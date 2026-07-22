#include "map.h"
#include "ui_map.h"
#include <QTimer>

// ============================================================
// 构造函数与析构函数
// ============================================================

map::map(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::map)
{
    ui->setupUi(this);
    this->setFixedSize(944, 600);

    // ---------- 初始化网络管理器 ----------
    routeManager = new QNetworkAccessManager(this);
    connect(routeManager, &QNetworkAccessManager::finished, this, &map::handleRouteReply);

    // ---------- 初始化UI组件 ----------
    ui->startPosEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->startPosEdit->setClearButtonEnabled(true);
    ui->endPosEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->endPosEdit->setClearButtonEnabled(true);
    ui->verticalLayout_3->setAlignment(Qt::AlignCenter);

    // ---------- 初始化地图参数 ----------
    currentZoom = 14;
    currentCenterLat = 0;
    currentCenterLon = 0;
    isDragging = false;

    // ---------- 初始化GPS模块 ----------
    gpsInitialized = false;
    gpsTimer = new QTimer(this);
    gpsTimer->setSingleShot(true);
    connect(gpsTimer, &QTimer::timeout, this, &map::onGpsTimeout);
    gpsTimer->start(10000);  // 10秒超时，未收到GPS则使用默认位置

    GPS *myGps = new GPS(this);
    if (myGps->openSerial("/dev/ttymxc2")) {
        // 连接GPS信号
        connect(myGps, &GPS::locationUpdated, this, [this](double lat, double lon){
            // 收到GPS数据，停止超时定时器
            if (gpsTimer->isActive()) {
                gpsTimer->stop();
            }

            bool isFirstLoc = (this->currentGpsLat == 0.0);
            double distSq = (lat - lastRequestedLat)*(lat - lastRequestedLat) +
                            (lon - lastRequestedLon)*(lon - lastRequestedLon);

            this->currentGpsLat = lat;
            this->currentGpsLon = lon;

            // 定位模式下，位置变化超过阈值则更新地图
            if (currentState == STATE_LOCATING && (isFirstLoc || distSq > 0.000025)) {
                lastRequestedLat = lat;
                lastRequestedLon = lon;
                requestCoordConv(lat, lon);  // 触发坐标纠偏
            }
            this->update();
            gpsInitialized = true;
        });
    } else {
        // GPS模块打开失败，直接使用默认位置
        onGpsTimeout();
    }
}

map::~map()
{
    delete ui;
}

// ============================================================
// 鼠标交互事件
// ============================================================

void map::mousePressEvent(QMouseEvent *event)
{
    // 判断是否为鼠标左键按下
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();// 记录当前鼠标坐标，作为拖拽起始点
        setCursor(Qt::ClosedHandCursor);// 设置鼠标样式为“抓手”，提升交互体验
        event->accept();
    }
    QWidget::mousePressEvent(event);//其他事件默认父类处理
}

void map::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 计算拖拽偏移量
        int dx = event->pos().x() - lastMousePos.x();
        int dy = event->pos().y() - lastMousePos.y();

        // 只有在真正移动了的情况下才刷新地图
        if (dx != 0 || dy != 0) {
            QRect mapRect = ui->mapContainer->geometry();
            // 计算：1个像素 对应多少经纬度（根据缩放等级动态计算）
            double lonPerPixel = 360.0 / (256 * pow(2, currentZoom)) * (mapRect.width() / 256.0);
            double latPerPixel = 180.0 / (256 * pow(2, currentZoom)) * (mapRect.height() / 256.0);

            // 计算新的中心点
            double newLon = currentCenterLon - dx * lonPerPixel;//下拉往上
            double newLat = currentCenterLat + dy * latPerPixel;//右拉往左

            // 经纬度边界限制（避免地图拖出地球范围）
            newLat = qBound(-85.0, newLat, 85.0);
            if (newLon > 180) newLon -= 360;
            if (newLon < -180) newLon += 360;

            // 更新中心点
            currentCenterLat = newLat;
            currentCenterLon = newLon;

            // 定位模式下同步更新标记位置
            if (currentState == STATE_LOCATING) {
                currentMarkerPos = QString("%1,%2").arg(currentCenterLon, 0, 'f', 6)
                                                   .arg(currentCenterLat, 0, 'f', 6);
            }

            refreshMap();  // 重新请求地图
        }

        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    QWidget::mouseReleaseEvent(event);
}

// ============================================================
// 绘制事件
// ============================================================

void map::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QRect targetRect = ui->mapContainer->geometry();

    // 根据当前模式选择地图文件
    QString imgFile = (currentState == STATE_NAVIGATING) ? "nav_map.png" : "loc.png";

    // 非拖拽状态或缓存为空时加载图片
    if (!isDragging || cachedPixmap.isNull()) {
        QPixmap pixmap(imgFile);
        if (!pixmap.isNull()) {
            cachedPixmap = pixmap;
        }
    }

    // 绘制地图
    if (!cachedPixmap.isNull()) {
        if (isDragging) {
            painter.drawPixmap(targetRect.translated(dragOffset), cachedPixmap);
        } else {
            painter.drawPixmap(targetRect, cachedPixmap);
        }
    } else {
        painter.drawText(targetRect, Qt::AlignCenter, "正在加载地图...");
    }
}

// ============================================================
// 网络响应处理
// ============================================================

void map::handleRouteReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "网络通信错误:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QString urlStr = reply->url().toString();

    // 1. 坐标转换响应
    if (urlStr.contains("geoconv/v2")) {
        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (root["status"].toInt() == 0) {
            QJsonObject result = root["result"].toArray()[0].toObject();
            requestMapImageForCoords(result["y"].toDouble(), result["x"].toDouble());
        }
    }
    // 2. 地理编码响应
    else if (urlStr.contains("geocoding/v3")) {
        QString type = reply->request().attribute(QNetworkRequest::User).toString();
        QJsonObject root = QJsonDocument::fromJson(data).object();
        if (root["status"].toInt() == 0) {
            QJsonObject loc = root["result"].toObject()["location"].toObject();
            QString latLon = QString("%1,%2").arg(loc["lat"].toDouble(), 0, 'f', 6)
                                             .arg(loc["lng"].toDouble(), 0, 'f', 6);
            if (type == "START") {
                startLatLon = latLon;
                requestGeocoding(ui->endPosEdit->text(), false);
            } else if (type == "END") {
                endLatLon = latLon;
                requestRoutePlan(startLatLon, endLatLon);
            }
        }
    }
    // 3. 路径规划响应
    else if (urlStr.contains("directionlite")) {
        parseRouteJson(data);
    }
    // 4. 静态地图响应
    else if (urlStr.contains("staticimage/v2")) {
        QString fileName = (currentState == STATE_NAVIGATING) ? "nav_map.png" : "loc.png";
        if (data.size() > 200) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);// 将HTTP返回的图片二进制数据写入文件
                file.close();
                this->update();
            }
        }
    }
    reply->deleteLater();
}

// ============================================================
// 地图请求函数
// ============================================================

/**
 * @brief 刷新地图（根据当前模式）
 */
void map::refreshMap()
{
    if (currentState == STATE_NAVIGATING && !currentPathPoints.isEmpty()) {
        // 导航模式：刷新带路径的地图
        requestMapImage();
    } else if (currentState == STATE_LOCATING) {
        // 定位模式：使用当前中心点或GPS位置
        if (currentCenterLat != 0 && currentCenterLon != 0) {
            requestMapImageForCoords(currentCenterLat, currentCenterLon);
        } else if (currentGpsLat != 0 && currentGpsLon != 0) {
            requestCoordConv(currentGpsLat, currentGpsLon);
        }
    }
}

/**
 * @brief 请求坐标转换（WGS-84转百度坐标系）
 */
void map::requestCoordConv(double lat, double lon)
{
    // 百度地图坐标转换 API V2 （GPS → 百度坐标 BD09）
    // 对照手册：model=2 代表 gps to bd09ll
    QString urlStr = QString("https://api.map.baidu.com/geoconv/v2/?coords=%1,%2&model=2&ak=%3")
        .arg(lon, 0, 'f', 6)   // 经度（保留6位小数）
        .arg(lat, 0, 'f', 6)   // 纬度
        .arg("0tOED1V9eKbBPB8BOKBBvBTExsCPkYXK"); // 你的AK
    routeManager->get(QNetworkRequest(QUrl(urlStr)));
}

/**
 * @brief 请求静态地图（定位模式，带标记点）
 */
void map::requestMapImageForCoords(double bdLat, double bdLon)
{
    // 首次定位或重置时更新中心点
    if (currentCenterLat == 0 || currentCenterLon == 0) {
        currentCenterLat = bdLat;
        currentCenterLon = bdLon;
        currentMarkerPos = QString("%1,%2").arg(bdLon, 0, 'f', 6).arg(bdLat, 0, 'f', 6);
    }

    QString baseUrl = "http://api.map.baidu.com/staticimage/v2";
    QUrl url(baseUrl);
    QUrlQuery query;
    query.addQueryItem("ak", "dFvwjI1je60O4sCBDV2yy3ue736hid4R");
    query.addQueryItem("width", "744");
    query.addQueryItem("height", "600");
    query.addQueryItem("zoom", QString::number(currentZoom));
    query.addQueryItem("center", currentMarkerPos);
    query.addQueryItem("markers", currentMarkerPos);
    query.addQueryItem("markerStyles", "l,S,0xff0000");
    url.setQuery(query);
    routeManager->get(QNetworkRequest(url));
}

/**
 * @brief 请求静态地图（导航模式，带路径）
 */
void map::requestMapImage()
{
    // 无路径直接返回
        if (currentPathPoints.isEmpty())
            return;

        QUrl url("http://api.map.baidu.com/staticimage/v2");
        QUrlQuery query;
        query.addQueryItem("ak", "dFvwjI1je60O4sCBDV2yy3ue736hid4R");
        query.addQueryItem("width", "744");
        query.addQueryItem("height", "600");
        query.addQueryItem("zoom", QString::number(currentZoom));

        // 导航路线样式 + 路径点
        query.addQueryItem("pathStyles", "0x00ff00,3,1");
        query.addQueryItem("paths", currentPathPoints);

        // 设置地图中心点
        if (currentCenterLat != 0 && currentCenterLon != 0) {
            QString center = QString("%1,%2")
                .arg(currentCenterLon, 0, 'f', 6)
                .arg(currentCenterLat, 0, 'f', 6);
            query.addQueryItem("center", center);
        }

        url.setQuery(query);
        routeManager->get(QNetworkRequest(url));
}

/**
 * @brief 请求地理编码（地址转坐标）
 */
void map::requestGeocoding(QString location, bool isStart)
{
    QString baseUrl = "http://api.map.baidu.com/geocoding/v3/";
    QUrl url(baseUrl);
    QUrlQuery query;
    query.addQueryItem("address", location);
    query.addQueryItem("output", "json");
    query.addQueryItem("ak", "dFvwjI1je60O4sCBDV2yy3ue736hid4R");
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, isStart ? "START" : "END");
    routeManager->get(request);
}

/**
 * @brief 请求路径规划
 */
void map::requestRoutePlan(QString startLatLon, QString endLatLon)
{
    QString baseUrl = "http://api.map.baidu.com/directionlite/v1/driving";
    QUrl url(baseUrl);
    QUrlQuery query;
    query.addQueryItem("origin", startLatLon);
    query.addQueryItem("destination", endLatLon);
    query.addQueryItem("ak", "dFvwjI1je60O4sCBDV2yy3ue736hid4R");
    query.addQueryItem("output", "json");
    url.setQuery(query);
    routeManager->get(QNetworkRequest(url));
}

// ============================================================
// 数据处理函数
// ============================================================

/**
 * @brief 解析路径规划JSON数据
 */
void map::parseRouteJson(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    if (root["status"].toInt() == 0) {
        QJsonObject route = root["result"].toObject()["routes"].toArray()[0].toObject();

        // 显示距离和时间
        ui->distLabel->setText(QString("预计距离: %1 公里").arg(route["distance"].toInt() / 1000.0, 0, 'f', 1));
        ui->timeLabel->setText(QString("预计用时: %1 分钟").arg(route["duration"].toInt() / 60));

        // 提取路径点
        QStringList allPoints;
        for (const QJsonValue &value : route["steps"].toArray()) {
            QString pathStr = value.toObject()["path"].toString();
            QStringList points = pathStr.split(';');// 按 ; 分割，变成一个个坐标点："116.339,40.010"、"116.340,40.010" ...
            allPoints.append(points);// 把这些点全部加入总路线点集合
        }

        // 采样减少点数量（每5个点取1个）
        QStringList sampledPoints;
        for (int i = 0; i < allPoints.size(); i += 5) {
            sampledPoints.append(allPoints[i]);
        }
        currentPathPoints = sampledPoints.join(';');

        // 初始化中心点为路径起点
        if (!sampledPoints.isEmpty()) {
            QStringList coords = sampledPoints.first().split(',');
            if (coords.size() == 2) {
                currentCenterLon = coords[0].toDouble();
                currentCenterLat = coords[1].toDouble();
                currentMarkerPos = sampledPoints.first();
                startLatLon = currentMarkerPos;
            }
        }

        requestMapImage();
    }
}

// ============================================================
// 辅助函数
// ============================================================

/**
 * @brief GPS超时处理，使用默认位置
 */
void map::onGpsTimeout()
{
    if (!gpsInitialized) {
        qDebug() << "GPS未检测到数据，使用默认位置: 北京市中心";
        double defaultLat = 39.9042;
        double defaultLon = 116.4074;

        this->currentGpsLat = defaultLat;
        this->currentGpsLon = defaultLon;

        if (currentState == STATE_LOCATING) {
            lastRequestedLat = defaultLat;
            lastRequestedLon = defaultLon;
            requestCoordConv(defaultLat, defaultLon);
        }
        this->update();
    }
}

// ============================================================
// 按钮槽函数
// ============================================================

void map::on_startNavBtn_clicked()
{
    QString start = ui->startPosEdit->text();
    QString end = ui->endPosEdit->text();
    if (start.isEmpty() || end.isEmpty()) return;

    currentState = STATE_NAVIGATING;
    requestGeocoding(start, true);
}

void map::on_zoomInBtn_clicked()
{
    if (currentZoom < 19) {
        currentZoom++;
        refreshMap();
        qDebug() << "当前缩放级别:" << currentZoom;
    }
}

void map::on_zoomOutBtn_clicked()
{
    if (currentZoom > 3) {
        currentZoom--;
        refreshMap();
        qDebug() << "当前缩放级别:" << currentZoom;
    }
}
//重置目前有个bug原因尚未明白，关于重置失效的问题跟startLatLon有关
void map::on_resetBtn_clicked()
{
    if (currentState == STATE_NAVIGATING) {
           // 导航模式：重置到路径起点
           if (!startLatLon.isEmpty()) {
               QStringList parts = startLatLon.split(',');
               if (parts.size() == 2) {
                   currentCenterLat = parts[1].toDouble();
                   currentCenterLon = parts[0].toDouble();
                   currentZoom = 14;
                   currentMarkerPos = startLatLon;
                   refreshMap();
               }
           }
       } else if (currentState == STATE_LOCATING) {
           // 定位模式：重置到GPS位置或默认位置
           if (currentGpsLat != 0 && currentGpsLon != 0) {
               currentCenterLat = 0;
               currentCenterLon = 0;
               requestCoordConv(currentGpsLat, currentGpsLon);
           } else {
               currentZoom = 14;
               currentCenterLat = 0;
               currentCenterLon = 0;
               requestCoordConv(39.9042, 116.4074);
           }
       }

       qDebug() << "地图已重置，当前缩放级别:" << currentZoom;
}
