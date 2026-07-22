#include "networktimemanager.h"

#include <QProcess>

NetworkTimeManager::NetworkTimeManager(QObject *parent) : QObject(parent)
{
    // --- 1. 初始化：不管网络是否同步，先读本地系统时间 ---
    // 开发板上读取的是硬件 RTC，电脑上读取的是系统时间
    currentDT = QDateTime::currentDateTime();

    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &NetworkTimeManager::onReplyFinished);

    // --- 2. 界面显示定时器 ---
    localTimer = new QTimer(this);
    connect(localTimer, &QTimer::timeout, this, [this](){
        // 即使网络同步失败，这里依然以 currentDT 为基准每秒累加
        currentDT = currentDT.addSecs(1);
        emit networkTimeUpdated(currentDT.toString("HH : mm"),
                                currentDT.toString("yyyy年MM月dd日"));
    });
    localTimer->start(1000);

    // --- 3. 同步定时器 ---
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &NetworkTimeManager::syncTime);
    refreshTimer->start(3600000); // 1小时一次

    syncTime(); // 启动时尝试同步，网络失败时 currentDT 保持初值
}

void NetworkTimeManager::syncTime()
{
    // 这里就是执行网络请求的具体逻辑
    QUrl url("http://quan.suning.com/getSysTime.do");
    QNetworkRequest request(url);

    // 发起 GET 请求
    // 注意：确保你的 manager 已经在构造函数里初始化过了
    if (manager) {
        manager->get(request);
        qDebug() << "正在向服务器请求同步时间...";
    } else {
        qDebug() << "错误：NetworkAccessManager 未初始化！";
    }
}

void NetworkTimeManager::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (obj.contains("sysTime2")) {
            QString rawDT = obj.value("sysTime2").toString();
            currentDT = QDateTime::fromString(rawDT, "yyyy-MM-dd HH:mm:ss");

#ifdef IS_BOARD_ENV
            saveToSystemClock(rawDT);
#else
            qDebug() << "PC 环境：网络同步时间：" << rawDT;
#endif
            // 同步成功，立即更新界面
            emit networkTimeUpdated(currentDT.toString("HH : mm"),
                                    currentDT.toString("yyyy年MM月dd日"));
        }
    } else {
        // --- 核心修改：网络同步失败 ---
        qDebug() << "网络同步失败，正在回退至本地系统时间...";

        // 强制刷新：从系统/硬件重新读取一次当前时间，防止因为程序运行久了导致的内存偏差
        currentDT = QDateTime::currentDateTime();

        // 发出信号，界面会显示当前的本地/硬件时间
        emit networkTimeUpdated(currentDT.toString("HH : mm"),
                                currentDT.toString("yyyy年MM月dd日"));
    }
    reply->deleteLater();
}

void NetworkTimeManager::saveToSystemClock(const QString &dateTimeStr)
{
    // 1. 将接收到的北京时间字符串转为 QDateTime
    QDateTime netDT = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm:ss");

    // 2. 关键补偿：手动减去 8 小时
    // 原理：date -s 写入的是系统时间，如果系统会自动处理 UTC 转换，
    // 我们预先减去时差，写入后系统显示刚好就是你想要的北京时间
    QDateTime adjustedDT = netDT.addSecs(-8 * 3600);

    QString adjustedStr = adjustedDT.toString("yyyy-MM-dd HH:mm:ss");

    // 3. 执行设置命令
    QString dateCmd = QString("date -s \"%1\"").arg(adjustedStr);
    int res1 = QProcess::execute(dateCmd);

    // 4. 写入硬件 RTC
    // 如果系统还是强制把 UTC 写入 RTC，那么这里保持原样即可
    int res2 = QProcess::execute("hwclock -w");

    if(res1 == 0 && res2 == 0) {
        qDebug() << "RTC 同步成功，已进行 8 小时时区补偿！";
    } else {
        qDebug() << "RTC 同步失败，请检查 root 权限";
    }
}
