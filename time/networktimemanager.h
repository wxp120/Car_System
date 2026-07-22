#ifndef NETWORKTIMEMANAGER_H
#define NETWORKTIMEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>

class NetworkTimeManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkTimeManager(QObject *parent = nullptr);

    // 主动触发同步请求
    void syncTime();

signals:
    // 成功获取后发送信号：时间字符串、日期字符串
    void networkTimeUpdated(const QString &time, const QString &date);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    QTimer *refreshTimer;
    QTimer *localTimer;      // 1秒一次的本地累加定时器
    QDateTime currentDT;     // 存储当前内存中的时间对象
    void saveToSystemClock(const QString &dateTimeStr);
};

#endif // NETWORKTIMEMANAGER_H
