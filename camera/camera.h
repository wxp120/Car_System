// camera.h
#ifndef CAMERA_H
#define CAMERA_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QThread>
#include <QPaintEvent>
#include <QAtomicInt>
#include <QMutex>
#include <linux/videodev2.h>

#define VIDEO_BUFFER_COUNT 4
#define VIDEO_DEVICE "/dev/video2"  // 根据您的设备修改

struct buffer_info {
    void *start;
    unsigned long length;
};

/* ==================== 超声波测距线程 ==================== */
class HCSR04Thread : public QThread
{
    Q_OBJECT
public:
    explicit HCSR04Thread(QObject *parent = nullptr);
    ~HCSR04Thread();

    void startThread() { startFlag.store(true); start(); }
    void stopThread() { startFlag.store(false); wait(); }

signals:
    void distanceReady(int distance);  // 发送距离值（单位：厘米）

protected:
    void run() override;

private:
    std::atomic<bool> startFlag{false};
};

// 采集线程类
class CaptureThread : public QThread
{
    Q_OBJECT
public:
    explicit CaptureThread(QObject *parent = nullptr);
    ~CaptureThread();

    void setLocalDisplay(bool enable) { startLocalDisplay = enable; }
    void startThread() { startFlag = 1; this->start(); }
    void stopThread() { startFlag = 0; this->wait(); }

signals:
    void imageReady(const QImage &image);  // 使用引用传递，避免拷贝

protected:
    void run() override;

private:
    QAtomicInt startFlag = 0;  // 使用原子操作
    bool startLocalDisplay = true;//控制是否将采集到的画面发送到界面显示。
};

namespace Ui {
class Camera;
}

class Camera : public QWidget
{
    Q_OBJECT

public:
    explicit Camera(QWidget *parent = nullptr);
    ~Camera();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    // 页面显示/隐藏事件
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_btn_camera_on_clicked();
    void showImage(const QImage &image);
    void startCamera();
    void stopCamera();
    void updateDistanceDisplay(int distance);  // 更新距离显示

    void on_pushButton_clicked();

private:
    Ui::Camera *ui;
    CaptureThread *captureThread;
    void startDistanceMeasurement();
    void stopDistanceMeasurement();
    QImage m_currentImage;
    QImage m_scaledImage;  // 缓存缩放后的图像
    QMutex m_imageMutex;   // 图像数据互斥锁
    bool isCameraRunning = false;
    QTimer *fpsTimer;      // FPS显示定时器
    int frameCount = 0;
    HCSR04Thread *hcsr04Thread;
    bool isDistanceMeasuring = false;  // 添加初始化

    void buzzerOn();      // 打开蜂鸣器
    void buzzerOff();     // 关闭蜂鸣器
    void checkAndAlert(int distance);  // 检查距离并报警
    bool isBuzzerEnabled = false;  // 蜂鸣器是否启用
    QTimer *buzzerTimer;           // 用于定时关闭蜂鸣器
};

#endif // CAMERA_H
