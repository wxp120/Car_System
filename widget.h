#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "weather.h"
#include "musicplayer.h"
#include "video.h"
#include "networktimemanager.h"
#include "analogclock.h"
#include "wifi.h"
#include "map.h"
#include "setting.h"
#include "speedmeter.h"
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "environment.h"
#include "camera.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:

    void on_weather_btn_clicked();

    void on_btn_music_clicked();

    void on_btn_video_clicked();

    void on_exit_btn_clicked();
    void GetTodayWeather(const Day &today,const QString &iconPath);
    void GetMusicName(const QString &info, bool isPlaying);
    void UpdateMainSlider(qint64 position, qint64 duration);
    void on_Play_btn_clicked();

    void on_Next_btn_clicked();

    void on_Last_btn_clicked();
    void UpdateSystemTime(const QString &time, const QString &date);

    void on_wifi_btn_clicked();


    void on_btn_nav_app_clicked();

    void on_btn_settings_clicked();

    void on_locate_btn_clicked();

    void on_btn_apps_clicked();


    void on_btn_environment_clicked();

    void on_btn_camera_clicked();

    void on_btn_home_clicked();

    void on_weather_btn_1_clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    Ui::Widget *ui;
private:
    weather *WeatherPage;
    musicplayer* MusicPlayerPage;
    Video *VideoPage;
    NetworkTimeManager *NetTime;
    analogclock *clock;
    Wifi *WifiPage; // 定义 WiFi 页面指针
    map *Mappage;
    Setting *SettingPage;
    Speedmeter *SpeedmeterPage;
    Environment *environemnt_Page;
    Camera *cameraPage;
    void initSettings();
    void initSubPages();
    void initConnections();
    void setupStackedWidget();
    void slideToPage(QWidget *nextPage, bool isNext);
    QPropertyAnimation *m_animCurr;
    QPropertyAnimation *m_animNext;
    QParallelAnimationGroup *m_group;
    QPoint m_lastPos;
    bool m_isAnimating;
    void slideToNext();
    void slideToPrev();
};
#endif // WIDGET_H
