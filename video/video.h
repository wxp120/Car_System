#ifndef VIDEO_H
#define VIDEO_H

#include <QMediaPlayer>
#include <QWidget>

namespace Ui {
class Video;
}

/* 媒体信息结构体 */
struct MediaObjectInfo {
    /* 用于保存视频文件名 */
    QString fileName;
    /* 用于保存视频文件路径 */
    QString filePath;
};

class Video : public QWidget
{
    Q_OBJECT

public:
    explicit Video(QWidget *parent = nullptr);
    ~Video();

private slots:
    void on_paly_btn_clicked();

    void on_next_btn_clicked();

    void Statechange(QMediaPlayer::State state);
    void Rowchanged(int row);
    void GetTotalTime(qint64 duration);
    void GetNowTime(qint64 pos);

    void on_videolist_doubleClicked(const QModelIndex &index);

    void on_volumeSlider_sliderReleased();

    void on_videoslider_sliderReleased();

    void on_largewidget_btn_clicked();
    void handleHideTimer(); // 定时器超时处理函数

private:
    Ui::Video *ui;
    /* 媒体播放器，用于播放视频 */
    QMediaPlayer *videoPlayer;
    /* 媒体列表 */
    QMediaPlaylist *mediaPlaylist;

    void mediaPlayerInit();
    void scanVideoFiles();

    QTimer *hideTimer;      // 声明定时器
    bool eventFilter(QObject *obj, QEvent *event); // 声明事件过滤器
};

#endif // VIDEO_H
