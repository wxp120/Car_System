#ifndef WEATHER_H
#define WEATHER_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QLabel>
#include <QListWidget>
#include "day.h"

QT_BEGIN_NAMESPACE
namespace Ui { class weather; }
QT_END_NAMESPACE

class weather : public QWidget
{
    Q_OBJECT

public:
    explicit weather(QWidget *parent = nullptr);
    ~weather();

public slots:
    void readytoread(QNetworkReply *reply);

signals:
    void weatherDataUpdated(const Day &today,const QString &iconPath);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onCityItemClicked(QListWidgetItem *item);
    void on_search_btn_clicked();

private:
    Ui::weather *ui;

    // 网络与数据
    QNetworkAccessManager *manager;

    QMap<QString, QString> mTypeMap;

    // UI 控件列表映射
    QList<QLabel*> mDatelist;
    QList<QLabel*> mWeekList;
    QList<QLabel*> mIconList;
    QList<QLabel*> mWeatherTypeList;
    QList<QLabel*> mAirqList;
    QList<QLabel*> mFxList;
    QList<QLabel*> mFlList;
    Day days[7];
    //列表控件
    QListWidget *cityListWidget;
    QMap<QString, QStringList> mCityTree;
    bool isProvinceLevel;

    // 内部功能函数
    void WeatherMapInit();
    void ParseWeatherJsonDataNew(QByteArray rawData);
    void UpdateUi();
    void drawTemperatureCurves();
    void drawTempLineLow();
    void queryWeather(const QString &cityName);
    void City_listInit();
    void City_init();
};

#endif // WEATHER_H
