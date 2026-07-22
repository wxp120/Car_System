#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class Environment;
}

class Environment : public QWidget
{
    Q_OBJECT

public:
    explicit Environment(QWidget *parent = nullptr);
    ~Environment();

    void startMonitoring();
    void stopMonitoring();

private slots:
    void updateSensorData();
    void updateLightData();
    void updateMQ135Data();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    Ui::Environment *ui;

    // 温湿度相关
    QTimer *sensorTimer;
    void readDHT11Data(float &temp, float &humidity);

    // 光强度相关
    QTimer *lightTimer;
    QLineSeries *lightSeries;
    QChart *lightChart;
    QChartView *lightChartView;
    int lightDataCount;
    static const int MAX_DATA_POINTS = 100;
    void readLightData(float &light);
    void setupLightChart();

    // MQ-135 相关
    QTimer *mq135Timer;
    void readADCScale();
    bool readMQ135RawValue(int &rawValue);  // 改为 bool
    void readMQ135Voltage(float &voltage);
    int calculateMQ135Percentage(float voltage);
    void updateMQ135Display(int percentage, int rawValue, float voltage);

    float mq135Scale;
    float maxVoltage;
};

#endif // ENVIRONMENT_H
