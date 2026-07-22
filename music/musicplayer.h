#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPropertyAnimation>
#include <QSlider>
#include <QLabel>
namespace Ui {
class musicplayer;
}

/* 媒体信息结构体 */
struct MusicObjectInfo {
    /* 用于保存歌曲文件名 */
    QString fileName;
    /* 用于保存歌曲文件路径 */
    QString filePath;
};


class musicplayer : public QWidget
{
    Q_OBJECT

public:
    explicit musicplayer(QWidget *parent = nullptr);
    ~musicplayer();

private slots:

    void on_listWidget_currentRowChanged(int currentRow);
    void on_playlistIndexChanged(int index);
    void on_listWidget_doubleClicked(const QModelIndex &index);
    void updatePlayButtonIcon(QMediaPlayer::State state);
    void on_btn_volume_clicked();
    void on_btn_favorite_clicked();

    void on_btn_mode_clicked();

public slots:
    void on_btn_play_clicked();
    void on_btn_previous_clicked();
    void on_btn_next_clicked();
signals:
    void musicStatusUpdated(const QString &songName, bool isPlaying);
    void musicPositionChanged(qint64 position, qint64 duration);

private:
    Ui::musicplayer *ui;
    /* 媒体播放器，用于播放音乐 */
    QMediaPlayer *musicPlayer;
    /* 媒体列表 */
    QMediaPlaylist *mediaPlaylist;
    /* 媒体信息存储 */
    QVector<MusicObjectInfo> mediaObjectInfo;
    int selectIndex = -1; // 初始设为-1，表示还没选
    QPropertyAnimation *diskAnimation;
    QSlider *volumeSlider; // 动态创建的音量条
    QLabel *volumeLabel; // 用于显示百分比数值
    QMap<qint64, QString> m_lrcMap;//存储时间辍和歌词
    bool isFavorite = false;
    void mediaPlayerInit();
    void scanSongs();
    void GetDuration(qint64 duration);
    void GetCurPosition(qint64 position);
    void SliderMove(int value);
    void on_sliderReleased();
    void parseLrc(const QString &lrcPath); // 解析逻辑

};

#endif // MUSICPLAYER_H
