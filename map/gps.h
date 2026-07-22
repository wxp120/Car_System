#ifndef GPS_H
#define GPS_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>

class GPS : public QObject
{
    Q_OBJECT
public:
    explicit GPS(QObject *parent = nullptr);
    bool openSerial(const QString &portName);

signals:
    // 解析完成后发出的信号，包含经纬度
    void locationUpdated(double latitude, double longitude);

private slots:
    void handleReadyRead();

private:
    QSerialPort *serial;
    QByteArray buffer;
    double convertToDegrees(const QString &nmea);
};

#endif // GPS_H
