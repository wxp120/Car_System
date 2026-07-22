#include "gps.h"
#include <QDebug>
#include <QtMath>

GPS::GPS(QObject *parent) : QObject(parent)
{
    serial = new QSerialPort(this);
    connect(serial, &QSerialPort::readyRead, this, &GPS::handleReadyRead);
}

bool GPS::openSerial(const QString &portName)
{
    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud38400);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    return serial->open(QIODevice::ReadOnly);
}

void GPS::handleReadyRead()
{
    buffer.append(serial->readAll());
    while (buffer.contains('\n')) {
        int index = buffer.indexOf('\n');
        QByteArray frame = buffer.left(index);
        buffer.remove(0, index + 1);

        if (frame.startsWith("$GNRMC")) {
            QString str(frame);
            QStringList fields = str.split(',');
            if (fields.size() > 6 && fields[2] == "A") {
                double lat = convertToDegrees(fields[3]);
                double lon = convertToDegrees(fields[5]);

                if (fields[4] == "S") lat = -lat;
                if (fields[6] == "W") lon = -lon;

                emit locationUpdated(lat, lon);
            }
        }
    }
}

// NMEA 经纬度格式转换函数
// 输入：DDMM.MMMM 格式字符串（如 2318.1236 表示 23度18.1236分）
// 输出：十进制经纬度 DDD.DDDD（地图通用格式）
double GPS::convertToDegrees(const QString &nmea)
{
    double val = nmea.toDouble();
    int deg = (int)(val / 100);
    double min = val - (deg * 100);
    return deg + (min / 60.0);
}
