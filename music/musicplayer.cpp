#include "musicplayer.h"
#include "ui_musicplayer.h"
#include <QDirIterator>
#include <QUrl>
#include <QDebug>
#include <QMediaPlaylist>
#include <QMediaPlayer>
#include <QTimer>

musicplayer::musicplayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::musicplayer)
{
    ui->setupUi(this);

    // --- 1. 基础 UI 初始化 ---
    ui->label_disk->setFixedSize(350, 350);
    mediaPlayerInit(); // 初始化播放器及核心组件
    scanSongs();       // 扫描本地音乐文件

    // --- 2. 信号与槽连接 ---

    // 播放器状态改变（同步按钮图标、旋转动画、外部状态通知）
    connect(musicPlayer, &QMediaPlayer::stateChanged, this, &musicplayer::updatePlayButtonIcon);

    // 进度条与时间显示同步
    connect(musicPlayer, &QMediaPlayer::durationChanged, this, &musicplayer::GetDuration);
    connect(musicPlayer, &QMediaPlayer::positionChanged, this, &musicplayer::GetCurPosition);

    // 用户手动拖动进度条逻辑
    connect(ui->durationSlider, &QSlider::sliderReleased, this, &musicplayer::on_sliderReleased);
    connect(ui->durationSlider, &QSlider::sliderMoved, this, &musicplayer::SliderMove);

    // 播放列表索引改变（自动同步列表高亮）
    connect(mediaPlaylist, &QMediaPlaylist::currentIndexChanged, this, &musicplayer::on_playlistIndexChanged);

    // 音量调节（更新标签显示并设置音量）
    connect(volumeSlider, &QSlider::valueChanged, [this](int val){
        volumeLabel->setText(QString("%1%").arg(val));
        musicPlayer->setVolume(val);
    });
}

musicplayer::~musicplayer()
{
    delete ui;
}

/**
 * @brief 初始化媒体播放器、播放列表、音量条及光盘动画
 */
void musicplayer::mediaPlayerInit()
{
    // 初始化光盘封面
    ui->label_disk->setPixmap(QPixmap(":/img/cd.png"));

    musicPlayer = new QMediaPlayer(this);
    mediaPlaylist = new QMediaPlaylist(this);

    // --- 音量百分比标签初始化 ---
    volumeLabel = new QLabel(this);
    volumeLabel->setAlignment(Qt::AlignCenter);
    volumeLabel->setStyleSheet("color: white; background: rgba(0,0,0,150); border-radius: 4px; font-weight: bold;");
    volumeLabel->setFixedSize(40, 20);
    volumeLabel->hide();

    // --- 音量条 (QSlider) 初始化与车载样式设置 ---
    volumeSlider = new QSlider(Qt::Vertical, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(musicPlayer->volume());
    volumeSlider->setFixedSize(40, 150);
    volumeSlider->hide();
    volumeSlider->setStyleSheet(
                "QSlider::groove:vertical { background: #222; width: 12px; border-radius: 6px; }"
                "QSlider::add-page:vertical { background: #3498db; border-radius: 6px; }"
                "QSlider::handle:vertical { background: #ffffff; height: 20px; width: 20px; margin: 0 -4px; border-radius: 10px; border: 2px solid #3498db; }"
                );

    // --- 播放策略配置 ---
    mediaPlaylist->clear();
    musicPlayer->setPlaylist(mediaPlaylist);
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop); // 默认列表循环

    // --- 光盘旋转动画配置 ---
    diskAnimation = new QPropertyAnimation(ui->label_disk, "rotation", this);
    diskAnimation->setDuration(15000);   // 旋转一圈 15 秒
    diskAnimation->setStartValue(0);     // 起始角度
    diskAnimation->setEndValue(360);     // 结束角度
    diskAnimation->setLoopCount(-1);     // 无限循环
    diskAnimation->setEasingCurve(QEasingCurve::Linear); // 匀速

    // 隐藏垂直和水平滚动条
    ui->listWidget_lrc->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_lrc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

/**
 * @brief 扫描指定目录下的歌曲文件并添加到列表
 */
void musicplayer::scanSongs()
{
    QString targetPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../res/music");
    QDir dir(targetPath);
    if (!dir.exists()) {
        qDebug() << "警告：找不到目录！" << targetPath;
        return;
    }

    QStringList filter;
    filter << "*.mp3" << "*.wav";
    QFileInfoList files = dir.entryInfoList(filter, QDir::Files);

    for (int i = 0; i < files.count(); i++) {
        MusicObjectInfo info;
        QString baseName = files.at(i).completeBaseName();

        // 解析文件名 (格式要求: 歌手 - 歌名)
        QStringList nameParts = baseName.split("-");
        if (nameParts.size() >= 2) {
            QString artist = nameParts.at(0).trimmed();
            QString songName = nameParts.at(1).trimmed();
            info.fileName = songName + "\n" + artist;
        } else {
            info.fileName = baseName + "\n未知歌手";
        }

        info.filePath = files.at(i).absoluteFilePath();

        // 添加到播放列表及 UI 展示
        if (mediaPlaylist->addMedia(QUrl::fromLocalFile(info.filePath))) {
            mediaObjectInfo.append(info);
            ui->listWidget->addItem(info.fileName);
        }
    }
}

// --- 时间与进度条处理 ---

void musicplayer::GetDuration(qint64 duration)
{
    ui->durationSlider->setRange(0, duration);
    int seconds = (duration / 1000) % 60;
    int minutes = (duration / 60000);
    ui->label_3->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
    emit musicPositionChanged(musicPlayer->position(), duration);
}

void musicplayer::GetCurPosition(qint64 position)
{
    if (!ui->durationSlider->isSliderDown()) { // 用户未拖动时自动同步
        ui->durationSlider->setValue(position);
        int seconds = (position / 1000) % 60;
        int minutes = (position / 60000);
        ui->label_2->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
    }
    //歌词切换逻辑
    if (m_lrcMap.isEmpty()) return;
    // 1. 寻找当前时间对应的歌词
    // lowerBound 找的是第一个 >= pos 的位置
    auto it = m_lrcMap.lowerBound(position);
    int index = 0;
    if (it == m_lrcMap.end()) {
        index = m_lrcMap.size() - 1;
    } else if (it != m_lrcMap.begin()) {
        // 退回一步，拿到当前正在唱的那句
        index = std::distance(m_lrcMap.begin(), it) - 1;
    }
    // 2. 只有当行号改变时才刷新 UI，避免频繁重绘
    if (ui->listWidget_lrc->currentRow() != index) {
        ui->listWidget_lrc->setCurrentRow(index);

        // 3. 让当前行始终保持在 ListWidget 的中心
        ui->listWidget_lrc->scrollToItem(ui->listWidget_lrc->currentItem(),
                                         QAbstractItemView::PositionAtCenter);
    }
    emit musicPositionChanged(position, musicPlayer->duration());
}

void musicplayer::SliderMove(int value)
{
    int seconds = (value / 1000) % 60;
    int minutes = (value / 60000);
    ui->label_2->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
}

void musicplayer::on_sliderReleased()
{
    musicPlayer->setPosition(ui->durationSlider->value());
}

void musicplayer::parseLrc(const QString &lrcPath)
{

    m_lrcMap.clear();
    ui->listWidget_lrc->clear(); // 清空上一首

    QFile file(lrcPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->listWidget_lrc->addItem("暂无歌词");
        return;
    }

    QTextStream in(&file);
    // 正则表达式匹配 [00:00.00]
    QRegularExpression reg("\\[(\\d+):(\\d+)\\.(\\d+)\\](.*)");

    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch match = reg.match(line);
        if (match.hasMatch()) {
            // 1. 时间转毫秒
            qint64 m = match.captured(1).toInt();
            qint64 s = match.captured(2).toInt();
            qint64 ms = match.captured(3).toInt();
            if (match.captured(3).length() == 2) ms *= 10;
            qint64 totalTime = m * 60000 + s * 1000 + ms;

            // 2. 提取文本
            QString text = match.captured(4).trimmed();
            if (!text.isEmpty()) {
                m_lrcMap.insert(totalTime, text);

                // 3. 填充列表控件
                QListWidgetItem *item = new QListWidgetItem(text);
                item->setTextAlignment(Qt::AlignCenter); // 文字居中
                ui->listWidget_lrc->addItem(item);
            }
        }
    }
    file.close();

}

// --- 播放控制逻辑 ---

void musicplayer::on_listWidget_currentRowChanged(int currentRow)
{
    selectIndex = currentRow;
}

void musicplayer::on_btn_play_clicked()
{
    int state = musicPlayer->state();

    if (state == QMediaPlayer::PlayingState) {
        musicPlayer->pause();
    } else {
        // --- 修复：如果未选中任何歌曲，默认选中第一首 ---
        if (selectIndex == -1) {
            if (mediaPlaylist->mediaCount() > 0) {
                selectIndex = 0; // 默认选中第一首
            } else {
                return; // 列表为空，直接返回，防止后续越界
            }
        }

        // 如果是停止状态或用户在列表选择了新歌，则切换索引
        if (state == QMediaPlayer::StoppedState || mediaPlaylist->currentIndex() != selectIndex) {
            mediaPlaylist->setCurrentIndex(selectIndex);
        }
        musicPlayer->play();
    }
}

void musicplayer::on_playlistIndexChanged(int index)
{
    if (index != -1) {
        ui->listWidget->setCurrentRow(index); // 同步高亮选中行
        selectIndex = index;

        QString musicPath = mediaObjectInfo[index].filePath;
        QString lrcPath = musicPath;
        lrcPath.replace(QRegularExpression("\\.(mp3|wav|flac)$"), ".lrc");
        parseLrc(lrcPath);

        emit musicStatusUpdated(mediaObjectInfo[index].fileName, true);//播放完歌曲通知主页面
    }
}

void musicplayer::on_listWidget_doubleClicked(const QModelIndex &index)
{
    selectIndex = index.row();
    if (selectIndex != -1) {
        mediaPlaylist->setCurrentIndex(selectIndex);
        musicPlayer->play();
    }
}

/**
 * @brief 根据播放状态更新按钮图标及光盘旋转动画
 */
void musicplayer::updatePlayButtonIcon(QMediaPlayer::State state)
{
    bool isPlaying = (state == QMediaPlayer::PlayingState);

    // 更新播放按钮 QSS
    if (isPlaying) {
        ui->btn_play->setStyleSheet("border-image: url(:/img/btn_pause1.png);");
        ui->play_statuslabel->setText("暂停");
        if (diskAnimation->state() != QAbstractAnimation::Running) diskAnimation->start();
    } else {
        ui->btn_play->setStyleSheet("border-image: url(:/img/btn_play1.png);");
        ui->play_statuslabel->setText("播放");
        diskAnimation->stop();
    }

    emit musicStatusUpdated(mediaObjectInfo[selectIndex].fileName, isPlaying);//发送给主页面变化信息
}

void musicplayer::on_btn_previous_clicked()
{
    int count = mediaPlaylist->mediaCount();
    if (count == 0) return;
    selectIndex = (selectIndex - 1 < 0) ? count - 1 : selectIndex - 1;
    mediaPlaylist->setCurrentIndex(selectIndex);
    ui->listWidget->setCurrentRow(selectIndex);
    musicPlayer->play();
}

void musicplayer::on_btn_next_clicked()
{
    int count = mediaPlaylist->mediaCount();
    if (count == 0) return;
    selectIndex = (selectIndex + 1 >= count) ? 0 : selectIndex + 1;
    mediaPlaylist->setCurrentIndex(selectIndex);
    ui->listWidget->setCurrentRow(selectIndex);
    musicPlayer->play();
}

// --- 附加功能 (音量、收藏) ---

void musicplayer::on_btn_volume_clicked()
{
    if (volumeSlider->isHidden()) {
        // 计算音量条及标签的弹出位置
        QPoint btnPos = ui->btn_volume->mapTo(this, QPoint(0, 0));
        int x = btnPos.x() + (ui->btn_volume->width() - volumeSlider->width()) / 2;
        int y = btnPos.y() - volumeSlider->height() - 10;

        volumeLabel->move(x, y - 25);
        volumeLabel->setText(QString("%1%").arg(volumeSlider->value()));
        volumeSlider->move(x, y);

        volumeSlider->show();
        volumeLabel->show();
        volumeSlider->raise();
        volumeLabel->raise();

        // 3秒后自动隐藏
        QTimer::singleShot(3000, [this](){
            volumeSlider->hide();
            volumeLabel->hide();
        });
    } else {
        volumeSlider->hide();
        volumeLabel->hide();
    }
}

void musicplayer::on_btn_favorite_clicked()
{
    isFavorite = !isFavorite;
    if (isFavorite) {
        ui->btn_favorite->setStyleSheet("border-image: url(:/img/btn_favorite_yes.png);");
    } else {
        ui->btn_favorite->setStyleSheet("border-image: url(:/img/btn_favorite_no.png);");
    }
}

void musicplayer::on_btn_mode_clicked()
{
    QMediaPlaylist::PlaybackMode curMode = mediaPlaylist->playbackMode();
    if (curMode == QMediaPlaylist::Loop) {
        // 切换到单曲循环
        mediaPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
        ui->btn_mode->setStyleSheet("border-image: url(:/img/singlecycle.png);");
        ui->mode_label->setText("单曲循环");
    }
    else if (curMode == QMediaPlaylist::CurrentItemInLoop) {
        // 切换到随机播放
        mediaPlaylist->setPlaybackMode(QMediaPlaylist::Random);
        ui->btn_mode->setStyleSheet("border-image: url(:/img/shuffleplay.png);");
        ui->mode_label->setText("随机播放");
    }
    else if(curMode ==QMediaPlaylist::Random){
        // 切换到顺序播放
        mediaPlaylist->setPlaybackMode(QMediaPlaylist::Sequential);
        ui->btn_mode->setStyleSheet("border-image: url(:/img/playinorder.png);");
        ui->mode_label->setText("顺序播放");
    }
    else
    {
        mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
        ui->btn_mode->setStyleSheet("border-image: url(:/img/Listloop.png);");
        ui->mode_label->setText("列表循环");
    }
}
