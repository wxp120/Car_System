#include "environment.h"
#include "ui_environment.h"
#include <QShowEvent>
#include <QHideEvent>
#include <QVBoxLayout>

// ★ 嵌入式调试：注释掉下面这行来排除 Qt Charts 是否导致堆损坏
#define DISABLE_CHART

#ifndef DISABLE_CHART
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
QT_CHARTS_USE_NAMESPACE
#endif

Environment::Environment(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Environment),
    lightSeries(nullptr),
    lightChart(nullptr),
    lightChartView(nullptr),
    lightDataCount(0),
    mq135Scale(0.0),
    maxVoltage(3.3),
    lightTimer(nullptr),
    sensorTimer(nullptr),
    mq135Timer(nullptr)
{
    ui->setupUi(this);
    qDebug() << "Environment: setupUi done";

    readADCScale();

#ifndef DISABLE_CHART
    setupLightChart();

    if (ui->widget_chart && lightChartView) {
        QVBoxLayout *layout = new QVBoxLayout(ui->widget_chart);
        layout->addWidget(lightChartView);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->widget_chart->setLayout(layout);
    }
#endif

    // 创建定时器
#ifndef DISABLE_CHART
    lightTimer = new QTimer(this);
    connect(lightTimer, &QTimer::timeout, this, &Environment::updateLightData);
#endif

    sensorTimer = new QTimer(this);
    connect(sensorTimer, &QTimer::timeout, this, &Environment::updateSensorData);

    mq135Timer = new QTimer(this);
    connect(mq135Timer, &QTimer::timeout, this, &Environment::updateMQ135Data);

    qDebug() << "Environment widget initialized";
}

Environment::~Environment()
{
    stopMonitoring();
    delete ui;
}

void Environment::startMonitoring()
{
    qDebug() << "Environment: 开始监测";

#ifndef DISABLE_CHART
    if (lightSeries) {
        lightSeries->clear();
    }
    lightDataCount = 0;

    if (lightChart && lightChart->axisY()) {
        lightChart->axisY()->setRange(0, 1000);
    }
#endif

    updateSensorData();
#ifndef DISABLE_CHART
    updateLightData();
#endif
    updateMQ135Data();

#ifndef DISABLE_CHART
    if (lightTimer) lightTimer->start(3000);
#endif
    if (sensorTimer) sensorTimer->start(3000);
    if (mq135Timer) mq135Timer->start(2000);
}

void Environment::stopMonitoring()
{
    qDebug() << "Environment: 停止监测";

#ifndef DISABLE_CHART
    if (lightTimer && lightTimer->isActive()) lightTimer->stop();
#endif
    if (sensorTimer && sensorTimer->isActive()) sensorTimer->stop();
    if (mq135Timer && mq135Timer->isActive()) mq135Timer->stop();
}

void Environment::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    startMonitoring();
}

void Environment::hideEvent(QHideEvent *event)
{
    stopMonitoring();
    QWidget::hideEvent(event);
}

// ==================== ADC Scale 读取 ====================
void Environment::readADCScale()
{
    QFile scaleFile("/sys/bus/iio/devices/iio:device0/in_voltage_scale");

    if (scaleFile.open(QIODevice::ReadOnly)) {
        char buf[32] = {0};
        qint64 len = scaleFile.read(buf, sizeof(buf) - 1);
        scaleFile.close();

        if (len > 0) {
            bool ok;
            mq135Scale = QByteArray(buf, len).trimmed().toFloat(&ok);
            if (ok && mq135Scale > 0) {
                qDebug() << "ADC scale:" << mq135Scale;
                return;
            }
        }
        qDebug() << "Invalid ADC scale, using default";
        mq135Scale = 0.805664062;
    } else {
        qDebug() << "Failed to read ADC scale:" << scaleFile.errorString();
        mq135Scale = 0.805664062;
    }
}

// ==================== MQ-135 空气质量传感器 ====================
void Environment::updateMQ135Data()
{
    int rawValue = 0;
    float voltage = 0.0;
    int percentage = 0;

    // 安全读取
    if (!readMQ135RawValue(rawValue)) {
        qDebug() << "Failed to read MQ-135 raw value";
        return;
    }

    readMQ135Voltage(voltage);
    percentage = calculateMQ135Percentage(voltage);
    updateMQ135Display(percentage, rawValue, voltage);

    qDebug() << QString("MQ-135 - 原始值: %1, 电压: %2 V, 百分比: %3%")
                .arg(rawValue)
                .arg(voltage, 0, 'f', 2)
                .arg(percentage);
}

bool Environment::readMQ135RawValue(int &rawValue)
{
    QFile rawFile("/sys/bus/iio/devices/iio:device0/in_voltage1_raw");

    if (!rawFile.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开 MQ-135 原始值文件:" << rawFile.errorString();
        rawValue = 0;
        return false;
    }

    char buf[16] = {0};
    qint64 len = rawFile.read(buf, sizeof(buf) - 1);
    rawFile.close();

    if (len > 0) {
        bool ok;
        rawValue = QByteArray(buf, len).trimmed().toInt(&ok);
        if (ok) {
            return true;
        }
    }

    qDebug() << "Failed to parse raw value";
    rawValue = 0;
    return false;
}

void Environment::readMQ135Voltage(float &voltage)
{
    voltage = 0.0f;  // 默认值
    int rawValue = 0;

    if (readMQ135RawValue(rawValue)) {
        voltage = rawValue * mq135Scale / 1000.0f;

        if (voltage > maxVoltage) voltage = maxVoltage;
        if (voltage < 0) voltage = 0;
    }
}

int Environment::calculateMQ135Percentage(float voltage)
{
    if (maxVoltage <= 0) return 0;

    int percentage = (int)((voltage / maxVoltage) * 100);
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;
    return percentage;
}

void Environment::updateMQ135Display(int percentage, int rawValue, float voltage)
{
    if (!ui->label_air) {
        qDebug() << "ERROR: label_air not found";
        return;
    }

    // 只显示百分比，不做复杂操作
    ui->label_air->setText(QString::number(percentage) + " %");

    // 根据百分比改变颜色（安全，不设置 tooltip）
    if (percentage >= 70) {
        ui->label_air->setStyleSheet("color: red;");
    } else if (percentage >= 40) {
        ui->label_air->setStyleSheet("color: orange;");
    } else {
        ui->label_air->setStyleSheet("color: green; ");
    }
}


// ==================== 温湿度传感器 ====================
void Environment::updateSensorData()
{
    float temperature = 0.0;
    float humidity = 0.0;

    readDHT11Data(temperature, humidity);

    if (ui->label_temp) {
        ui->label_temp->setText(QString::number(temperature, 'f', 1) + " ℃");

        if (temperature > 30.0) {
            ui->label_temp->setStyleSheet("color: red; font-size: 40px;");
        } else if (temperature < 15.0) {
            ui->label_temp->setStyleSheet("color: blue; font-size: 40px;");
        } else {
            ui->label_temp->setStyleSheet("color: white; font-size: 40px;");
        }
    }

    if (ui->label_humity) {
        ui->label_humity->setText(QString::number(humidity, 'f', 1) + " %");

        if (humidity > 70.0) {
            ui->label_humity->setStyleSheet("color: red; font-size: 40px;");
        } else if (humidity < 30.0) {
            ui->label_humity->setStyleSheet("color: blue; font-size: 40px;");
        } else {
            ui->label_humity->setStyleSheet("color: white; font-size: 40px;");
        }
    }

    qDebug() << QString("温湿度 - 温度: %1 ℃, 湿度: %2 %").arg(temperature).arg(humidity);
}


void Environment::readDHT11Data(float &temp, float &humidity)
{
    char buf[16] = {0};
    QFile file("/dev/dht11");

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "打开 /dev/dht11 失败:" << file.errorString();
        temp = 0.0;
        humidity = 0.0;
        return;
    }

    qint64 len = file.read(buf, 5);
    file.close();

    qDebug() << "DHT11 read len:" << len;

    if (len == 5) {
        unsigned char *data = (unsigned char *)buf;
        humidity = data[0] + data[1] / 10.0f;
        temp     = data[2] + data[3] / 10.0f;
    } else {
        qDebug() << "DHT11 read failed, expected 5 bytes, got" << len;
        temp = 0.0;
        humidity = 0.0;
    }
}

// ==================== 光强度传感器 ====================
#ifndef DISABLE_CHART
void Environment::updateLightData()
{
    float lightValue = 0.0;
    readLightData(lightValue);

    if (!lightSeries) {
        qDebug() << "lightSeries is null";
        return;
    }

    if (lightSeries->count() < MAX_DATA_POINTS) {
        lightSeries->append(lightSeries->count(), lightValue);
    } else {
        QVector<QPointF> oldPoints = lightSeries->pointsVector();
        lightSeries->clear();

        for (int i = 1; i < oldPoints.size(); i++) {
            lightSeries->append(i - 1, oldPoints[i].y());
        }
        lightSeries->append(MAX_DATA_POINTS - 1, lightValue);
    }

    if (lightChart && lightChart->axisY() && lightSeries->count() > 0) {
        double maxY = 0;
        for (const QPointF &point : lightSeries->pointsVector()) {
            if (point.y() > maxY) maxY = point.y();
        }
        double yMax = qMax(maxY * 1.1, 10.0);
        lightChart->axisY()->setRange(0, yMax);
    }

    if (lightChart && lightChart->axisX()) {
        lightChart->axisX()->setRange(0, MAX_DATA_POINTS);
    }

    qDebug() << QString("光强度: %1 lux, 数据点数: %2").arg(lightValue).arg(lightSeries->count());
}
#endif

void Environment::readLightData(float &light)
{
    QFile file("/sys/class/misc/ap3216c/als");

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开光强度文件:" << file.errorString();
        light = 0.0;
        return;
    }

    char buf[16] = {0};
    qint64 len = file.read(buf, sizeof(buf) - 1);
    file.close();

    qDebug() << "光强度原始数据:" << QByteArray(buf, len);
    light = QByteArray(buf, len).trimmed().toFloat();
}

#ifndef DISABLE_CHART
void Environment::setupLightChart()
{
    lightSeries = new QLineSeries();
    lightSeries->setName("光强度 (lux)");
    lightSeries->setColor(Qt::green);

    lightChart = new QChart();
    lightChart->addSeries(lightSeries);
    lightChart->setTitle("环境光强度实时曲线");
    lightChart->setAnimationOptions(QChart::NoAnimation);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("时间 (采样点)");
    axisX->setRange(0, MAX_DATA_POINTS);
    axisX->setLabelFormat("%d");
    axisX->setTickCount(6);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("光强度 (lux)");
    axisY->setRange(0, 65535);
    axisY->setLabelFormat("%.0f");

    lightChart->addAxis(axisX, Qt::AlignBottom);
    lightChart->addAxis(axisY, Qt::AlignLeft);
    lightSeries->attachAxis(axisX);
    lightSeries->attachAxis(axisY);

    lightChartView = new QChartView(lightChart);
}
#endif
