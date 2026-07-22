#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget), m_animCurr(nullptr)// 新增：初始化为nullptr
    , m_animNext(nullptr)// 新增：初始化为nullptr
    , m_group(nullptr)// 新增：初始化为nullptr
    , m_isAnimating(false)// 新增：初始化为false
{
    ui->setupUi(this);

    // --- 1. 窗口及基础组件初始化 ---
    initSettings();

    // --- 2. 实例化子页面与功能类 ---
    initSubPages();

    // --- 3. 信号与槽连接 ---
    initConnections();

    // --- 4. 堆栈窗口设置 ---
    setupStackedWidget();
}

Widget::~Widget()
{
    delete ui;
}

// 基础属性设置
void Widget::initSettings()
{
    this->setAttribute(Qt::WA_StyledBackground);

    ui->verticalLayout_2->setAlignment(Qt::AlignCenter);//居中

    // 实例化模拟时钟并放入布局
    clock = new analogclock(this);
    if (ui->card_clock->layout()) {
        // 插入到最顶端，位于数字时间之上
        static_cast<QVBoxLayout*>(ui->card_clock->layout())->insertWidget(0, clock);
    }
}

// 页面对象初始化
void Widget::initSubPages()
{
    WeatherPage = new weather(this);
    MusicPlayerPage = new musicplayer(this);
    VideoPage = new Video(this);
    NetTime = new NetworkTimeManager(this);
    WifiPage = new Wifi(this);
    Mappage = new map(this);
    SettingPage = new Setting(this);
    SpeedmeterPage = new Speedmeter(this);
    environemnt_Page = new Environment(this);
    cameraPage = new Camera(this);
}

// 统一管理信号连接
void Widget::initConnections()
{
    // 网络时间更新
    connect(NetTime, &NetworkTimeManager::networkTimeUpdated, this, &Widget::UpdateSystemTime);

    // 天气数据回调
    connect(WeatherPage, &weather::weatherDataUpdated, this, &Widget::GetTodayWeather);

    // 音乐播放器回调
    connect(MusicPlayerPage, &musicplayer::musicStatusUpdated, this, &Widget::GetMusicName);
    connect(MusicPlayerPage, &musicplayer::musicPositionChanged, this, &Widget::UpdateMainSlider);
}

// 堆栈窗口管理
void Widget::setupStackedWidget()
{
    ui->stackedWidget->addWidget(WeatherPage);
    ui->stackedWidget->addWidget(MusicPlayerPage);
    ui->stackedWidget->addWidget(VideoPage);
    ui->stackedWidget->addWidget(WifiPage);
    ui->stackedWidget->addWidget(Mappage);
    ui->stackedWidget->addWidget(SettingPage);
    ui->stackedWidget->addWidget(SpeedmeterPage);
    ui->stackedWidget->addWidget(environemnt_Page);
    ui->stackedWidget->addWidget(cameraPage);

    // 新增：优化堆栈窗口性能
    ui->stackedWidget->setAttribute(Qt::WA_OpaquePaintEvent, true);

    // 默认展示第一个页面
    ui->stackedWidget->setCurrentIndex(0);
}

void Widget::slideToPage(QWidget *nextPage, bool isNext)
{
    // 如果正在动画，直接返回，不处理新的滑动
    if (m_isAnimating) return;

    QWidget *currPage = ui->stackedWidget->currentWidget();
    if (currPage == nextPage) return;

    // 标记动画开始
    m_isAnimating = true;

    // 获取页面宽度
    int width = ui->stackedWidget->width();
    if (width <= 0) {
        width = this->width();
    }

    // 确定滑动方向
    int direction = isNext ? 1 : -1;

    // 设置目标页面位置
    nextPage->setGeometry(width * direction, 0, width, ui->stackedWidget->height());
    nextPage->show();

    // 复用动画对象，而不是每次都 new
    if (!m_animCurr) {
        m_animCurr = new QPropertyAnimation(currPage, "pos", this);
        m_animNext = new QPropertyAnimation(nextPage, "pos", this);
        m_group = new QParallelAnimationGroup(this);
        m_group->addAnimation(m_animCurr);
        m_group->addAnimation(m_animNext);
    } else {
        // 如果动画对象已存在，重新设置目标对象
        m_animCurr->setTargetObject(currPage);
        m_animNext->setTargetObject(nextPage);
    }

    // 断开之前的连接，避免重复执行
    disconnect(m_group, nullptr, this, nullptr);

    // 设置当前页动画
    m_animCurr->setDuration(350);
    m_animCurr->setEasingCurve(QEasingCurve::OutCubic);  // 缓动曲线，更平滑
    m_animCurr->setStartValue(QPoint(0, 0));
    m_animCurr->setEndValue(QPoint(-width * direction, 0));

    // 设置目标页动画
    m_animNext->setDuration(350);
    m_animNext->setEasingCurve(QEasingCurve::OutCubic);
    m_animNext->setStartValue(QPoint(width * direction, 0));
    m_animNext->setEndValue(QPoint(0, 0));

    // 动画完成后的清理工作
    connect(m_group, &QParallelAnimationGroup::finished, this, [=]() {
        // 确保最终位置正确
        currPage->move(0, 0);
        nextPage->move(0, 0);

        // 设置当前页面
        ui->stackedWidget->setCurrentWidget(nextPage);

        // 隐藏旧页面
        if (currPage != nextPage) {
            currPage->hide();
        }

        // 动画结束标志
        m_isAnimating = false;
    });

    // 开始播放动画
    m_group->start();
}

void Widget::slideToNext()
{
    int currentIndex = ui->stackedWidget->currentIndex();
    if (currentIndex > 2) return;  // 安全保护

    int nextIndex = (currentIndex + 1) % 3;  // 0->1, 1->2, 2->0
    slideToPage(ui->stackedWidget->widget(nextIndex), true);
}

void Widget::slideToPrev()
{
    int currentIndex = ui->stackedWidget->currentIndex();
    if (currentIndex > 2) return;  // 安全保护

    int prevIndex = (currentIndex - 1 + 3) % 3;  // 0->2, 1->0, 2->1
    slideToPage(ui->stackedWidget->widget(prevIndex), false);
}

void Widget::on_weather_btn_clicked() { ui->stackedWidget->setCurrentWidget(WeatherPage); }
void Widget::on_btn_music_clicked()   { ui->stackedWidget->setCurrentWidget(MusicPlayerPage); }
void Widget::on_btn_video_clicked()   { ui->stackedWidget->setCurrentWidget(VideoPage); }
void Widget::on_exit_btn_clicked()    { ui->stackedWidget->setCurrentIndex(0); }
void Widget::on_btn_nav_app_clicked() {ui->stackedWidget->setCurrentWidget(Mappage);}
void Widget::on_btn_settings_clicked(){ui->stackedWidget->setCurrentWidget(SettingPage);}
void Widget::on_wifi_btn_clicked(){ui->stackedWidget->setCurrentWidget(WifiPage);}
void Widget::on_locate_btn_clicked(){ui->stackedWidget->setCurrentWidget(Mappage);}
void Widget::on_btn_apps_clicked(){ui->stackedWidget->setCurrentWidget(SpeedmeterPage);}
void Widget::on_btn_environment_clicked(){ui->stackedWidget->setCurrentWidget(environemnt_Page);}
void Widget::on_btn_camera_clicked(){ui->stackedWidget->setCurrentWidget(cameraPage);}
void Widget::on_btn_home_clicked(){ui->stackedWidget->setCurrentIndex(0);}
void Widget::on_weather_btn_1_clicked(){ui->stackedWidget->setCurrentWidget(WeatherPage);}

void Widget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();  // 改用 m_lastPos
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    // 动画期间不处理滑动
    if (m_isAnimating) {
        return;
    }

    // 获取当前页面索引
    int currentIndex = ui->stackedWidget->currentIndex();

    // 只有索引 0、1、2 的页面才允许手势滑动
    if (currentIndex > 2) {
        return;  // 其他页面不允许滑动
    }

    int deltaX = event->pos().x() - m_lastPos.x();

    // 阈值50像素
    if (qAbs(deltaX) > 50) {
        if (deltaX > 0) {
            slideToPrev();  // 向右滑
        } else {
            slideToNext();  // 向左滑
        }
    }
}

// --- UI 更新函数 ---
void Widget::UpdateSystemTime(const QString &time, const QString &date)
{
    ui->timeLabel->setText(time);
    ui->dateLabel->setText(date);
}

void Widget::GetTodayWeather(const Day &today,const QString &iconPath)
{
    ui->lab_temp->setText(today.Temp + "℃");
    ui->lab_city->setText(today.city);
    ui->lab_weather_status->setText(today.weatherType);
    QString style = QString("QPushButton { border-image: url(%1); border: none; }").arg(iconPath);
    ui->weather_btn->setStyleSheet(style);
}

void Widget::GetMusicName(const QString &info, bool isPlaying)
{
    // 获取歌名（取第一行）
    ui->SongTabel->setText(info.split("\n").at(0));

    // 更新播放/暂停按钮样式
    QString iconPath = isPlaying ? ":/img/btn_playing.png" : ":/img/btn_pausing.png";
    ui->Play_btn->setStyleSheet(QString("border-image: url(%1);").arg(iconPath));
}

void Widget::UpdateMainSlider(qint64 position, qint64 duration)
{
    ui->SongSlider->setRange(0, duration);
    ui->SongSlider->setValue(position);

    // 格式化时间显示 (MM:SS)
    int seconds = (position / 1000) % 60;
    int minutes = (position / 60000);
    ui->songtabel->setText(QString("%1:%2")
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0')));
}

// --- 快捷控制槽函数 ---
void Widget::on_Play_btn_clicked() { MusicPlayerPage->on_btn_play_clicked(); }
void Widget::on_Next_btn_clicked() { MusicPlayerPage->on_btn_next_clicked(); }
void Widget::on_Last_btn_clicked() { MusicPlayerPage->on_btn_previous_clicked(); }

