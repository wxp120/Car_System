#include "weather.h"
#include "ui_weather.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>
#include <QNetworkReply>
#include <QScroller>

weather::weather(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::weather)
{
    ui->setupUi(this);

    // 1. 系统与数据初始化
    City_listInit();    // 初始化浮动城市列表控件
    City_init();        // 加载省市数据树
    WeatherMapInit();   // 初始化天气图标映射

    // 2. 网络管理器初始化
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &weather::readytoread);

    // 3. UI 控件组映射 (便于循环操作)
    mWeekList << ui->labelday0 << ui->labelday1 << ui->labelday2 << ui->labelday3 << ui->labelday4;
    mDatelist << ui->labelDate0 << ui->labelDate1 << ui->labelDate2 << ui->labelDate3 << ui->labelDate4;
    mIconList << ui->labelWeatherIcon0 << ui->labelWeatherIcon1 << ui->labelWeatherIcon2 << ui->labelWeatherIcon3 << ui->labelWeatherIcon4;
    mWeatherTypeList << ui->lbweatherTypeDate0 << ui->lbweatherTypeDate1 << ui->lbweatherTypeDate2 << ui->lbweatherTypeDate3 << ui->lbweatherTypeDate4;
    mAirqList << ui->labelaiq0 << ui->labelaiq1 << ui->labelaiq2 << ui->labelaiq3 << ui->labelaiq4;

    // 4. 事件过滤器注册 (用于绘图)
    ui->widget0404->installEventFilter(this);

    // 5. 初始执行：默认查询北京天气
    qDebug() << "weather: constructor done, querying Beijing...";
    queryWeather(QString("青岛"));  // 暂时禁用，排查崩溃
}

weather::~weather()
{
    delete ui;
}

// --- 网络与搜索逻辑 ---

void weather::queryWeather(const QString &cityName)
{
    // 构造请求 URL 并发送 Get 请求
    QString url = QString("http://pddfps.tianqiapi.com/api?unescape=1&version=v9&appid=27566186&appsecret=XkWkYO4l&city=%1").arg(cityName);
    manager->get(QNetworkRequest(QUrl(url)));
}

void weather::readytoread(QNetworkReply *reply)
{
    int resCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError && resCode == 200) {
        ParseWeatherJsonDataNew(reply->readAll());
    } else {
        // 网络故障提示
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("网络故障");
        msgBox.setText(QString("错误: %1\n状态码: %2").arg(reply->errorString()).arg(resCode));
        msgBox.setStyleSheet("QMessageBox { background-color: #444; min-width: 200px; } QLabel { color: white; }");
        msgBox.exec();
    }
    reply->deleteLater();
}

// --- 城市选择逻辑封装 ---

void weather::City_listInit()
{
    // 创建手动漂浮在表面的列表控件
    cityListWidget = new QListWidget(this);
    cityListWidget->setGeometry(0, 0, 180, 272);
    cityListWidget->hide();

    // 设置平滑滚动与手势支持 (针对触摸屏优化)
    cityListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(cityListWidget, QScroller::LeftMouseButtonGesture);

    // QSS 样式表：深色车载风格与宽大滚动条
    cityListWidget->setStyleSheet(
                "QListWidget { background-color: rgb(30, 30, 30); color: white; border: none; font-size: 18px; outline: 0px; }"
                "QListWidget::item { height: 54px; border-bottom: 1px solid #444; padding-left: 15px; }"
                "QListWidget::item:selected { background-color: #0078d7; }"
                "QScrollBar:vertical { width: 25px; background: #222; margin: 0px; }"
                "QScrollBar::handle:vertical { background: #666; min-height: 40px; border-radius: 4px; margin: 2px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                );

    connect(cityListWidget, &QListWidget::itemClicked, this, &weather::onCityItemClicked);
}

void weather::on_search_btn_clicked()
{
    // 点击触发按钮：显示省份列表
    cityListWidget->clear();
    cityListWidget->addItems(mCityTree.keys());
    isProvinceLevel = true;

    cityListWidget->show();
    cityListWidget->raise(); // 确保在最顶层
}

void weather::onCityItemClicked(QListWidgetItem *item)
{
    QString name = item->text();

    if (isProvinceLevel) {
        // 进入二级城市列表
        cityListWidget->clear();
        cityListWidget->addItem("< 返回上级");
        cityListWidget->addItems(mCityTree[name]);
        isProvinceLevel = false;
    }
    else {
        // 处理返回逻辑或最终城市确认
        if (name == "< 返回上级") {
            on_search_btn_clicked();
            return;
        }
        queryWeather(name);    // 执行查询
        cityListWidget->hide(); // 隐藏列表
    }
}

// --- 数据处理与 UI 更新 ---
void weather::ParseWeatherJsonDataNew(QByteArray rawData)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);
    if(jsonDoc.isNull() || !jsonDoc.isObject()) return;

    QJsonObject jsonObj = jsonDoc.object();
    if(jsonObj.contains("errcode")) return;

    days[0].city = jsonObj["city"].toString();
    days[0].PM25 = jsonObj["aqi"].toObject()["pm25"].toString();

    QJsonArray dataArray = jsonObj["data"].toArray();
    for (int i = 0; i < dataArray.size() && i < 5; ++i)
    {
        QJsonObject obj = dataArray[i].toObject();
        days[i].date = obj["date"].toString();
        days[i].week = obj["week"].toString();

        // 截取天气类型 (例如 "多云转晴" -> "多云")
        QString rawWea = obj["wea"].toString();
        days[i].weatherType = rawWea.split("转").at(0).split("/").at(0);
        days[i].Temp = obj["tem"].toString();
        days[i].TempHigh = obj["tem1"].toString();
        days[i].TempLow = obj["tem2"].toString();
        days[i].humidty = obj["humidity"].toString();
        days[i].air_q = obj["air_level"].toString();
        days[i].tips = obj["air_tips"].toString();
        QJsonValue winVal = obj["win"];
        days[i].Fx = winVal.isArray() ? winVal.toArray().at(0).toString() : winVal.toString();
        // 截取风力等级
        QString rawWinSpeed = obj["win_speed"].toString();
        days[i].Fl = rawWinSpeed.split("转").at(0).split("/").at(0);
        if(obj.contains("index")&&obj["index"].isArray())
        {
            QJsonArray indexArray = obj["index"].toArray();
            for (const QJsonValue &value : indexArray) {
                if((value.toObject())["title"].toString()=="紫外线指数")
                    days[i].UV_tips = (value.toObject())["desc"].toString();
                if((value.toObject())["title"].toString()=="洗车指数")
                    days[i].Car_WashTips = (value.toObject())["desc"].toString();
                if((value.toObject())["title"].toString()=="穿衣指数")
                    days[i].Dressing_Tips = (value.toObject())["desc"].toString();
            }
        }
    }
    UpdateUi();
    emit weatherDataUpdated(days[0], mTypeMap[days[0].weatherType]);
}

void weather::UpdateUi()
{
    // 主界面核心信息更新
    ui->labelCity->setText(days[0].city);
    ui->labelCurrentDate->setText(days[0].date + " " + days[0].week);
    ui->labelTmp->setText(days[0].Temp);
    ui->labelTempRange->setText(days[0].TempLow + "~" + days[0].TempHigh + "℃");
    ui->labelweatherType->setText(days[0].weatherType);
    ui->labelWeatherIcon->setPixmap(QPixmap(mTypeMap[days[0].weatherType]).scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->labelFXType->setText(days[0].Fx);
    ui->labelFXType_2->setText(days[0].Fl);
    ui->labelPM25Data->setText(days[0].PM25);
    ui->labelShiduData->setText(days[0].humidty);
    ui->labelairData->setText(days[0].air_q);
    ui->Tips_label->setText(days[0].tips);
    ui->dressing_label->setText(days[0].Dressing_Tips);
    ui->washingCar_label->setText(days[0].Car_WashTips);
    ui->UV_label->setText(days[0].UV_tips);

    // 预报列表更新
    for(int i = 0; i < 5; i++)
    {
        QStringList dateParts = days[i].date.split("-");
        if(dateParts.size() >= 3) {
            mDatelist[i]->setText(dateParts.at(1) + "/" + dateParts.at(2));
        }
        mWeekList[i]->setText(i == 0 ? "今天" : days[i].week);
        mWeatherTypeList[i]->setText(days[i].weatherType);
        mAirqList[i]->setText(days[i].air_q);
        mIconList[i]->setPixmap(QPixmap(mTypeMap[days[i].weatherType]).scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    ui->widget0404->update();
}

// --- 绘图逻辑 --
bool weather::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->widget0404 && event->type() == QEvent::Paint) {
        drawTemperatureCurves();
        return true;
    }
    return QWidget::eventFilter(watched, event);//如果不加其他事件将会实失效，但是这里widget没有其他事件，无所谓
}

void weather::drawTemperatureCurves()
{
    if (!ui->widget0404 || ui->widget0404->width() <= 0) return;

    QPainter painter(ui->widget0404);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int W = ui->widget0404->width();
    int H = ui->widget0404->height();
    int margin = 25;

    int maxT = -999, minT = 999;
    for(int i = 0; i < 5; i++) {
        if(days[i].TempHigh.toInt() > maxT) maxT = days[i].TempHigh.toInt();
        if(days[i].TempLow.toInt() < minT)  minT = days[i].TempLow.toInt();
    }

    float tempDiff = (maxT - minT) == 0 ? 1.0f : (maxT - minT);
    float offsetStep = (float)(H - 2 * margin) / tempDiff;

    QPoint pointsHigh[5];
    QPoint pointsLow[5];

    for(int i = 0; i < 5; i++)
    {
        int x = (W / 5) * i + (W / 10);
        int yHigh = margin + (maxT - days[i].TempHigh.toInt()) * offsetStep;
        int yLow  = margin + (maxT - days[i].TempLow.toInt()) * offsetStep;

        pointsHigh[i] = QPoint(x, yHigh);
        pointsLow[i]  = QPoint(x, yLow);
    }

    // 绘制最高温曲线 (黄色)
    painter.setPen(QPen(Qt::yellow, 2));
    painter.setBrush(Qt::yellow);
    for(int i = 0; i < 5; i++) {
        painter.drawEllipse(pointsHigh[i], 3, 3);
        painter.drawText(pointsHigh[i].x() - 10, pointsHigh[i].y() - 10, days[i].TempHigh + "°");
        if(i < 4) painter.drawLine(pointsHigh[i], pointsHigh[i+1]);
    }

    // 绘制最低温曲线 (蓝色)
    painter.setPen(QPen(QColor(85, 170, 255), 2));
    painter.setBrush(QColor(85, 170, 255));
    for(int i = 0; i < 5; i++) {
        painter.drawEllipse(pointsLow[i], 3, 3);
        painter.drawText(pointsLow[i].x() - 10, pointsLow[i].y() + 20, days[i].TempLow + "°");
        if(i < 4) painter.drawLine(pointsLow[i], pointsLow[i+1]);
    }
}

/**
 * @brief 初始化城市选择数据树
 */
void weather::City_init()
{
    mCityTree.clear();

    // 直辖市
    mCityTree["直辖市"] << "北京" << "上海" << "天津" << "重庆";

    // 华北地区
    mCityTree["河北"]   << "石家庄" << "保定" << "邯郸" << "唐山" << "秦皇岛" << "廊坊" << "邢台";
    mCityTree["山西"]   << "太原" << "大同" << "临汾" << "运城" << "晋中";
    mCityTree["内蒙古"] << "呼和浩特" << "包头" << "赤峰" << "鄂尔多斯";

    // 华东地区
    mCityTree["山东"] << "济南" << "青岛" << "烟台" << "潍坊" << "临沂" << "济宁" << "淄博" << "威海";
    mCityTree["江苏"] << "南京" << "苏州" << "无锡" << "徐州" << "南通" << "常州" << "扬州" << "泰州";
    mCityTree["浙江"] << "杭州" << "宁波" << "温州" << "嘉兴" << "绍兴" << "金华" << "台州" << "湖州";
    mCityTree["安徽"] << "合肥" << "芜湖" << "蚌埠" << "阜阳" << "安庆";
    mCityTree["福建"] << "福州" << "厦门" << "泉州" << "漳州" << "莆田";
    mCityTree["江西"] << "南昌" << "赣州" << "上饶" << "九江" << "宜春";

    // 华中地区
    mCityTree["河南"] << "郑州" << "洛阳" << "新乡" << "南阳" << "许昌" << "信阳" << "安阳" << "商丘";
    mCityTree["湖北"] << "武汉" << "宜昌" << "襄阳" << "荆州" << "十堰" << "黄石" << "孝感";
    mCityTree["湖南"] << "长沙" << "株洲" << "湘潭" << "衡阳" << "岳阳" << "邵阳" << "常德";

    // 华南地区
    mCityTree["广东"] << "广州" << "深圳" << "珠海" << "佛山" << "东莞" << "中山" << "江门" << "惠州" << "汕头";
    mCityTree["广西"] << "南宁" << "柳州" << "桂林" << "玉林" << "北海";
    mCityTree["海南"] << "海口" << "三亚";

    // 西南地区
    mCityTree["四川"] << "成都" << "绵阳" << "宜宾" << "南充" << "泸州" << "达州" << "乐山";
    mCityTree["贵州"] << "贵阳" << "遵义" << "毕节";
    mCityTree["云南"] << "昆明" << "曲靖" << "大理" << "丽江";

    // 西北地区
    mCityTree["陕西"] << "西安" << "咸阳" << "宝鸡" << "渭南" << "延安" << "汉中";
    mCityTree["甘肃"] << "兰州" << "天水" << "白银";
    mCityTree["宁夏"] << "银川";
    mCityTree["新疆"] << "乌鲁木齐" << "库尔勒" << "伊宁";

    // 东北地区
    mCityTree["辽宁"]   << "沈阳" << "大连" << "鞍山" << "抚顺" << "本溪";
    mCityTree["吉林"]   << "长春" << "吉林" << "四平";
    mCityTree["黑龙江"] << "哈尔滨" << "大庆" << "齐齐哈尔" << "牡丹江";
}

/**
 * @brief 初始化天气图标映射
 */
void weather::WeatherMapInit()
{
    mTypeMap.clear();
    mTypeMap.insert("晴",     ":/img/Qing.png");
    mTypeMap.insert("多云",   ":/img/DuoYun.png");
    mTypeMap.insert("阴",     ":/img/Yin.png");
    mTypeMap.insert("雨",     ":/img/Yu.png");
    mTypeMap.insert("小雨",   ":/img/XiaoYu.png");
    mTypeMap.insert("中雨",   ":/img/ZhongYu.png");
    mTypeMap.insert("大雨",   ":/img/DaYu.png");
    mTypeMap.insert("暴雨",   ":/img/DaYu.png");
    mTypeMap.insert("雷阵雨", ":/img/LeiZhenYu.png");
    mTypeMap.insert("雪",     ":/img/Xue.png");
    mTypeMap.insert("霾",     ":/img/haze.png");
    mTypeMap.insert("雾",     ":/img/smog.png");
}
