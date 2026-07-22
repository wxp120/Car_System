#include "video.h"
#include "ui_video.h"
#include <QDir>
#include <QMediaPlaylist>
#include <QTime>
#include <QTimer>

Video::Video(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Video)
{
    ui->setupUi(this);
    mediaPlayerInit();
    scanVideoFiles();

    connect(videoPlayer, &QMediaPlayer::stateChanged, this, &Video::Statechange);
    connect(ui->videolist, &QListWidget::currentRowChanged, this, &Video::Rowchanged);
    connect(videoPlayer, &QMediaPlayer::durationChanged, this, &Video::GetTotalTime);
    connect(videoPlayer, &QMediaPlayer::positionChanged, this, &Video::GetNowTime);

    // --- 新增：初始化自动隐藏定时器 ---
    hideTimer = new QTimer(this);
    hideTimer->setInterval(10000); // 设置 10 秒后隐藏
    connect(hideTimer, &QTimer::timeout, this, &Video::handleHideTimer);

    // 给 videowidget 安装事件过滤器，用来捕获点击动作
    ui->videowidget->installEventFilter(this);

    // 启动初始计时
    hideTimer->start();
}

bool Video::eventFilter(QObject *obj, QEvent *event) {
    // 捕获在视频显示区域的鼠标点击或触摸动作
    if (obj == ui->videowidget && event->type() == QEvent::MouseButtonPress) {
        // 如果当前是隐藏状态，则显示出来
        if (ui->widget_2->isHidden()) {
            ui->widget_2->show();
            ui->videolist->show(); // 如果你希望列表也一起出来
            hideTimer->start();    // 重新开始 5 秒倒计时
        } else {
            // 如果已经是显示状态，点击一下可以立即触发隐藏（可选）
            //handleHideTimer();
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void Video::handleHideTimer() {
    ui->widget_2->hide();
    ui->videolist->hide();
    hideTimer->stop(); // 停止计时，直到下次被唤醒
}

Video::~Video()
{
    delete ui;
}

void Video::mediaPlayerInit()
{
    videoPlayer = new QMediaPlayer(this);
    mediaPlaylist = new QMediaPlaylist(this);
    // 强制视频填充整个 videowidget 区域，不留黑边
    ui->videowidget->setAspectRatioMode(Qt::IgnoreAspectRatio);
    /* 确保列表是空的 */
    mediaPlaylist->clear();
    /* 设置视频播放器的列表为mediaPlaylist */
    videoPlayer->setPlaylist(mediaPlaylist);
    /* 设置视频输出窗口 */
    videoPlayer->setVideoOutput(ui->videowidget);
    /* 设置播放模式，Loop是列循环 */
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
    /* 设置默认软件音量为50% */
    videoPlayer->setVolume(50);

    ui->volumeSlider->setRange(0, 100); // 确保范围是 0-100
    ui->volumeSlider->setValue(50);     // 设置初始位置

}

void Video::scanVideoFiles()
{
#ifdef IS_BOARD_ENV
    QDir dir("/home/root/res/video");
#else
    QDir dir("/home/tong/桌面/work/车载系统/res/video");
#endif
    if (dir.exists()) {
        QStringList filter;
        filter << "*.mp4" << "*.mkv" << "*.wmv" << "*.avi";

        // 获取所有文件信息
        QFileInfoList files = dir.entryInfoList(filter, QDir::Files);

        ui->videolist->clear();    // 清空界面显示
        mediaPlaylist->clear();    // 清空播放器内部列表

        for (int i = 0; i < files.count(); i++) {
            MediaObjectInfo info;
            // 直接获取文件名和绝对路径，Qt 自动处理编码
            info.fileName = files.at(i).fileName();
            info.filePath = files.at(i).absoluteFilePath();

            // 3. 将媒体添加到播放列表
            if (mediaPlaylist->addMedia(QUrl::fromLocalFile(info.filePath))) {
                // 5. 显示在 UI 列表上
                ui->videolist->addItem(info.fileName);
            } else {
                qDebug() << "Add Media Error:" << mediaPlaylist->errorString();
            }
        }

        // 6. 如果扫描到了视频，默认选中第一行，方便用户直接点击播放
        if (ui->videolist->count() > 0) {
            ui->videolist->setCurrentRow(0);
        }
    } else {
        qDebug() << "视频目录不存在！请检查路径：" << dir.path();
    }
}


void Video::on_paly_btn_clicked()
{
    QMediaPlayer::State state = videoPlayer->state();
    hideTimer->start();
    if (state == QMediaPlayer::PlayingState) {
        // 如果正在放，那就暂停
        videoPlayer->pause();
    }
    else {
        // 如果是停止或暂停状态，准备播放
        int currentRow = ui->videolist->currentRow(); // 获取当前光标所在的行

        // 核心逻辑：如果光标行和播放列表当前行不一致，强制切换
        if (currentRow != -1 && mediaPlaylist->currentIndex() != currentRow) {
            mediaPlaylist->setCurrentIndex(currentRow);
        }

        videoPlayer->play();
    }
}

void Video::on_next_btn_clicked()
{
    hideTimer->start();
    // 1. 播放列表切换到下一个
    mediaPlaylist->next();
    videoPlayer->play();

    // 2. 同步 UI 列表的光标位置
    int currentIndex = mediaPlaylist->currentIndex();
    if (currentIndex != -1) {
        ui->videolist->setCurrentRow(currentIndex);
    }
}


void Video::Statechange(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState) {
        // 播放状态：切换为暂停图标（或者是你命名的 playing.png）
        ui->paly_btn->setStyleSheet("border-image: url(:/img/stop_button_white.png);");
    } else {
        // 非播放状态（暂停/停止）：切换为播放图标
        ui->paly_btn->setStyleSheet("border-image: url(:/img/play_button_white.png);");
    }
}

void Video::Rowchanged(int row)
{
    // 如果播放器现在是停止或者暂停状态，光标一动，我们就把播放列表切过去
    if (row != -1 && videoPlayer->state() != QMediaPlayer::PlayingState) {
        mediaPlaylist->setCurrentIndex(row);
    }
}

void Video::GetTotalTime(qint64 duration)
{
    ui->videoslider->setRange(0, duration); // 设置进度条最大值为视频总毫秒数

    // 转换格式并显示总时间 00:00
    QTime totalTime(0, 0);
    totalTime = totalTime.addMSecs(duration);
    ui->totaltime->setText("/ " + totalTime.toString("mm:ss"));
}

void Video::GetNowTime(qint64 pos)
{
    if (!ui->videoslider->isSliderDown()) { // 只有在用户没在手动拖动时，才随视频自动移动
        ui->videoslider->setValue(pos);
    }

    // 更新当前时间标签
    QTime currentTime(0, 0);
    currentTime = currentTime.addMSecs(pos);
    ui->curtime->setText(currentTime.toString("mm:ss"));
}

void Video::on_videolist_doubleClicked(const QModelIndex &index)
{
    int row = index.row();
    mediaPlaylist->setCurrentIndex(row);
    videoPlayer->play();
}


void Video::on_volumeSlider_sliderReleased()
{
    int val = ui->volumeSlider->value();
    videoPlayer->setVolume(val);
}

void Video::on_videoslider_sliderReleased()
{
    qint64 pos = ui->videoslider->value();
    videoPlayer->setPosition(pos);
}

void Video::on_largewidget_btn_clicked()
{
    hideTimer->start();
    // 1. 获取右侧列表当前的显示状态
    bool isVisible = ui->videolist->isVisible();

    // 2. 取反设置：如果可见就隐藏，如果隐藏就显示
    ui->videolist->setVisible(!isVisible);

}
