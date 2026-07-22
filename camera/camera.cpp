// camera.cpp
#include "camera.h"
#include "ui_camera.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <string.h>
#include <errno.h>
#include <QPainter>
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>

/* ==================== CaptureThread 实现 ==================== */
CaptureThread::CaptureThread(QObject *parent) : QThread(parent)
{
}

CaptureThread::~CaptureThread()
{
    stopThread();
}

void CaptureThread::run()
{
    int video_fd = -1;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req_bufs;
    struct v4l2_buffer buf;
    int n_buf;
    struct buffer_info bufs_info[VIDEO_BUFFER_COUNT];
    enum v4l2_buf_type type;
    fd_set fds;
    struct timeval tv;
    int ret;

    // 1. 打开设备
    video_fd = open(VIDEO_DEVICE, O_RDWR | O_NONBLOCK);
    if (0 > video_fd) {
        qDebug() << "ERROR: failed to open video device" << VIDEO_DEVICE;
        return;
    }

    // 2. 枚举USB摄像头支持的所有格式
    qDebug() << "=== Enumerating camera supported formats ===";
    struct v4l2_fmtdesc fmtdesc;
    unsigned int supported_fmts[16] = {0};
    int fmt_count = 0;

    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0 && fmt_count < 16) {
        qDebug() << "  [" << fmt_count << "]" << fmtdesc.description
                 << "pixelformat:" << fmtdesc.pixelformat;
        supported_fmts[fmt_count] = fmtdesc.pixelformat;
        fmt_count++;
        fmtdesc.index++;
    }
    qDebug() << "Total supported formats:" << fmt_count;

    // 3. 按优先级匹配: YUYV > RGB565 > MJPEG > RGB24
    unsigned int preferred[] = {
        V4L2_PIX_FMT_YUYV,
        V4L2_PIX_FMT_RGB565,
        V4L2_PIX_FMT_MJPEG,
        V4L2_PIX_FMT_RGB24
    };
    const char *fmt_names[] = {"YUYV", "RGB565", "MJPEG", "RGB24"};
    unsigned int chosen_fmt = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < fmt_count; j++) {
            if (supported_fmts[j] == preferred[i]) {
                chosen_fmt = preferred[i];
                qDebug() << "Matched format:" << fmt_names[i];
                break;
            }
        }
        if (chosen_fmt) break;
    }

    if (!chosen_fmt) {
        qDebug() << "ERROR: no supported format found";
        close(video_fd);
        return;
    }

    // 4. 设置选中的格式（不强制分辨率，驱动自动匹配）
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = chosen_fmt;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (0 > ioctl(video_fd, VIDIOC_S_FMT, &fmt)) {
        qDebug() << "ERROR: failed to set format:" << strerror(errno);
        close(video_fd);
        return;
    }

    qDebug() << "Format set:" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;

    int actual_width = fmt.fmt.pix.width;
    int actual_height = fmt.fmt.pix.height;

    // 3. 请求缓冲区 (使用固定数量)
    memset(&req_bufs, 0, sizeof(req_bufs));
    req_bufs.count = VIDEO_BUFFER_COUNT;
    req_bufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_bufs.memory = V4L2_MEMORY_MMAP;

    if (0 > ioctl(video_fd, VIDIOC_REQBUFS, &req_bufs)) {
        qDebug() << "ERROR: failed to VIDIOC_REQBUFS:" << strerror(errno);
        close(video_fd);
        return;
    }

    qDebug() << "Requested" << req_bufs.count << "buffers";

    // 4. 查询缓冲区并MMAP映射
    for (n_buf = 0; n_buf < req_bufs.count; n_buf++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buf;

        if (0 > ioctl(video_fd, VIDIOC_QUERYBUF, &buf)) {
            qDebug() << "ERROR: failed to VIDIOC_QUERYBUF for buffer" << n_buf << ":" << strerror(errno);
            close(video_fd);
            return;
        }

        bufs_info[n_buf].length = buf.length;
        bufs_info[n_buf].start = mmap(NULL, buf.length,
                                      PROT_READ | PROT_WRITE, MAP_SHARED,
                                      video_fd, buf.m.offset);
        if (MAP_FAILED == bufs_info[n_buf].start) {
            qDebug() << "ERROR: failed to mmap video buffer" << n_buf << ", size" << buf.length;
            close(video_fd);
            return;
        }

        qDebug() << "Buffer" << n_buf << "mmapped, size:" << buf.length;
    }

    // 5. 所有缓冲区入队
    for (n_buf = 0; n_buf < req_bufs.count; n_buf++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buf;

        if (0 > ioctl(video_fd, VIDIOC_QBUF, &buf)) {
            qDebug() << "ERROR: failed to VIDIOC_QBUF for buffer" << n_buf << ":" << strerror(errno);
            close(video_fd);
            return;
        }
    }

    // 6. 开启视频流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 > ioctl(video_fd, VIDIOC_STREAMON, &type)) {
        qDebug() << "ERROR: failed to VIDIOC_STREAMON:" << strerror(errno);
        close(video_fd);
        return;
    }

    qDebug() << "✅ Camera capture thread started successfully!";

    // 7. 采集循环
    int frame_count = 0;
    while (startFlag.load()) {
        FD_ZERO(&fds);
        FD_SET(video_fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 33000;  // 33ms超时

        ret = select(video_fd + 1, &fds, NULL, NULL, &tv);
        if (ret == 0) {
            continue;
        }
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // 出队获取一帧
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (0 > ioctl(video_fd, VIDIOC_DQBUF, &buf)) {
            if (errno == EAGAIN) {
                continue;
            }
            qDebug() << "ERROR: failed to VIDIOC_DQBUF:" << strerror(errno);
            break;
        }

        if (buf.index < req_bufs.count) {
            // 创建QImage
            QImage qImage;

            if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) {
                // MJPEG -> Qt解码JPEG帧
                unsigned char *data = (unsigned char*)bufs_info[buf.index].start;
                unsigned int len = buf.bytesused;
                if (data && len > 0 && len <= bufs_info[buf.index].length) {
                    qImage.loadFromData(data, len);
                }
            }
            else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
                // RGB565格式直接使用
                qImage = QImage((unsigned char*)bufs_info[buf.index].start,
                               actual_width, actual_height,
                               QImage::Format_RGB16);
            }
            else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
                unsigned char *src = (unsigned char*)bufs_info[buf.index].start;
                QImage temp(actual_width, actual_height, QImage::Format_RGB888);
                int bpl = temp.bytesPerLine();
                unsigned char *dst = temp.bits();

                for (int y = 0; y < actual_height; y++) {
                    unsigned char *row_dst = dst + y * bpl;
                    unsigned char *row_src = src + y * actual_width * 2;

                    for (int x = 0; x < actual_width / 2; x++) {
                        int y0 = row_src[x*4];
                        int u  = row_src[x*4 + 1] - 128;
                        int y1 = row_src[x*4 + 2];
                        int v  = row_src[x*4 + 3] - 128;

                        int r0 = (y0 * 256 + 359 * v) >> 8;
                        int g0 = (y0 * 256 - 88 * u - 183 * v) >> 8;
                        int b0 = (y0 * 256 + 454 * u) >> 8;

                        int r1 = (y1 * 256 + 359 * v) >> 8;
                        int g1 = (y1 * 256 - 88 * u - 183 * v) >> 8;
                        int b1 = (y1 * 256 + 454 * u) >> 8;

                        auto clamp = [](int v) -> unsigned char {
                            return v > 255 ? 255 : (v < 0 ? 0 : (unsigned char)v);
                        };
                        row_dst[x*6]     = clamp(r0);
                        row_dst[x*6 + 1] = clamp(g0);
                        row_dst[x*6 + 2] = clamp(b0);
                        row_dst[x*6 + 3] = clamp(r1);
                        row_dst[x*6 + 4] = clamp(g1);
                        row_dst[x*6 + 5] = clamp(b1);
                    }
                }
                qImage = temp;
            }
            else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
                // RGB24转RGB888 (Qt的Format_RGB888就是RGB24)
                qImage = QImage((unsigned char*)bufs_info[buf.index].start,
                               actual_width, actual_height,
                               QImage::Format_RGB888);
            }

            // 发送图像到主线程
            if (startLocalDisplay && !qImage.isNull()) {
                emit imageReady(qImage.copy());
            }

            frame_count++;
            if (frame_count % 30 == 0) {
                // qDebug() << "Captured" << frame_count << "frames";
            }
        }

        // 重新入队
        if (0 > ioctl(video_fd, VIDIOC_QBUF, &buf)) {
            qDebug() << "ERROR: failed to VIDIOC_QBUF:" << strerror(errno);
            break;
        }
    }

    // 8. 清理资源
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(video_fd, VIDIOC_STREAMOFF, &type);

    for (int i = 0; i < req_bufs.count; i++) {
        if (bufs_info[i].start != MAP_FAILED && bufs_info[i].start != NULL) {
            munmap(bufs_info[i].start, bufs_info[i].length);
        }
    }
    close(video_fd);

    qDebug() << "Camera capture thread stopped, total frames:" << frame_count;
}

#define CMD_TRIG  100

/* ==================== HCSR04Thread 实现 ==================== */
HCSR04Thread::HCSR04Thread(QObject *parent) : QThread(parent)
{
}

HCSR04Thread::~HCSR04Thread()
{
    stopThread();
}

void HCSR04Thread::run()
{
    int fd = -1;
    int val;

    qDebug() << "HCSR04: Thread started";

    // 打开设备节点（O_RDWR 因为要发 ioctl 触发测量）
    fd = open("/dev/sr04", O_RDWR);
    if (fd < 0) {
        qDebug() << "HCSR04: Failed to open /dev/sr04:" << strerror(errno);
        return;
    }

    while (startFlag.load()) {
        // 1. 发 ioctl 触发一次测量
        ioctl(fd, CMD_TRIG);

        // 2. 读 4 字节，得到原始时间值（纳秒）
        if (read(fd, &val, 4) == 4) {
            // 3. 换算成厘米
            int distance = (int)((unsigned int)val * 340LL / 2 / 1000000);
            emit distanceReady(distance);
        } else {
            emit distanceReady(-1);
        }

        // 等待 0.5 秒后再测
        for (int i = 0; i < 25 && startFlag.load(); i++) {
            msleep(20);
        }
    }

    close(fd);
    qDebug() << "HCSR04: Thread stopped";
}


/* ==================== Camera 主类实现 ==================== */
Camera::Camera(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Camera)
{
    ui->setupUi(this);

    // 安装事件过滤器到显示widget
    if (ui->camera_widget) {
        ui->camera_widget->installEventFilter(this);
        ui->camera_widget->setAttribute(Qt::WA_OpaquePaintEvent);
    }

    // 创建采集线程
    captureThread = new CaptureThread(this);
    connect(captureThread, &CaptureThread::imageReady, this, &Camera::showImage);

    // 创建超声波测距线程
    hcsr04Thread = new HCSR04Thread(this);
    connect(hcsr04Thread, &HCSR04Thread::distanceReady,this, &Camera::updateDistanceDisplay);

    // 创建蜂鸣器定时器
    buzzerTimer = new QTimer(this);
    buzzerTimer->setSingleShot(true);
    connect(buzzerTimer, &QTimer::timeout, this, &Camera::buzzerOff);

    // 设置按钮初始文本
    ui->btn_camera_on->setText("打开摄像头");
    isCameraRunning = false;

    qDebug() << "Camera widget initialized";
}

Camera::~Camera()
{
    stopCamera();
    stopDistanceMeasurement();
    delete ui;
}

void Camera::buzzerOn()
{
    if (!isBuzzerEnabled) return;

    QFile beepFile("/sys/class/leds/beep/brightness");
    if (beepFile.open(QIODevice::WriteOnly)) {
        beepFile.write("1");
        beepFile.close();
        qDebug() << "Buzzer ON - Distance too close!";
    }
}

void Camera::buzzerOff()
{
    QFile beepFile("/sys/class/leds/beep/brightness");
    if (beepFile.open(QIODevice::WriteOnly)) {
        beepFile.write("0");
        beepFile.close();
    }
}

void Camera::checkAndAlert(int distance)
{
    static int lastAlertDistance = -1;

    // 如果蜂鸣器未启用，不报警
    if (!isBuzzerEnabled) {
        return;
    }

    // 距离有效且在报警范围内（小于10cm）
    if (distance > 0 && distance < 10) {
        // 避免重复报警，距离变化时才触发
        if (lastAlertDistance != distance) {
            buzzerOn();
            // 200ms 后自动关闭蜂鸣器
            buzzerTimer->start(200);
            qDebug() << "⚠️ ALERT: Distance =" << distance << "cm, below 10cm!";
        }
    } else {
        // 距离安全，确保蜂鸣器关闭（但不清空正在进行的定时器）
        if (buzzerTimer->isActive()) {
            buzzerTimer->stop();
        }
        buzzerOff();
    }

    lastAlertDistance = distance;
}

/* 页面显示时自动开始测距 */
void Camera::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    qDebug() << "Camera page shown, starting distance measurement...";
    startDistanceMeasurement();
}

/* 页面隐藏时自动停止测距 */
void Camera::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    qDebug() << "Camera page hidden, stopping distance measurement...";
    stopDistanceMeasurement();
}

void Camera::startDistanceMeasurement()
{
    if (isDistanceMeasuring) {
        return;
    }

    isDistanceMeasuring = true;
    hcsr04Thread->startThread();
    qDebug() << "Distance measurement started (0.5s interval)";
}

void Camera::stopDistanceMeasurement()
{
    if (!isDistanceMeasuring) {
        return;
    }

    isDistanceMeasuring = false;
    hcsr04Thread->stopThread();
    // 不要重置文字，让最后一次显示保留
    qDebug() << "Distance measurement stopped";
}

void Camera::updateDistanceDisplay(int distance)
{
    if (!ui->label_distance) {
        qDebug() << "ERROR: ui->label_distance is NULL!";
        return;
    }
    checkAndAlert(distance);//检查是否需要报警
    QString text;
    QString color;

    if (distance < 0) {
        text = "错误/超时";
        color = "#ffffff";
    } else if (distance == 0) {
        text = "<1 cm";
        color = "#ffffff";
    } else if (distance < 100) {
        text = QString("%1 cm").arg(distance);
        if (distance < 30) {
            color = "#ff4444";
            // 警告文字放在这里，不要重复设置 color
            if (distance < 10 && ui->tip_warning) {
                ui->tip_warning->setText("⚠️ 危险！距离过近！请停车！");
            } else if (distance < 20 && ui->tip_warning) {
                ui->tip_warning->setText("⚠️ 警告！距离很近！");
            } else if (ui->tip_warning) {
                ui->tip_warning->setText("⚠️ 注意！距离较近");
            }
        } else {
            color = "#ffa500";
            if (ui->tip_warning) {
                ui->tip_warning->setText("距离较近，请注意");
            }
        }
    } else {
        double meters = distance / 100.0;
        text = QString("%1 m").arg(meters, 0, 'f', 1);
        color = "#00ff00";
        if (ui->tip_warning) {
            ui->tip_warning->setText("✓ 距离安全");
        }
    }

    ui->label_distance->setText(text);
    ui->label_distance->setStyleSheet(QString("color: %1;").arg(color));
}

void Camera::startCamera()
{
    if (isCameraRunning) {
        qDebug() << "Camera already running";
        return;
    }

    isCameraRunning = true;
    captureThread->setLocalDisplay(true);//确认要将画面发到界面显示
    captureThread->startThread();//开启线程
    ui->btn_camera_on->setText("关闭摄像头");
    ui->camera_status->setText("摄像头：已开启");
    qDebug() << "Camera started";
}

void Camera::stopCamera()
{
    if (!isCameraRunning) {
        return;
    }

    isCameraRunning = false;
    captureThread->stopThread();
    {
        QMutexLocker locker(&m_imageMutex);
        m_currentImage = QImage();// 赋值一个空的 QImage 对象
        m_scaledImage = QImage();// 赋值一个空的 QImage 对象
    }
    if (ui->camera_widget) {
        ui->camera_widget->update();
    }
    ui->btn_camera_on->setText("打开摄像头");
    ui->camera_status->setText("摄像头：未开启");
    qDebug() << "Camera stopped";
}

void Camera::on_btn_camera_on_clicked()
{
    if (isCameraRunning) {
        stopCamera();
    } else {
        startCamera();
    }
}

void Camera::showImage(const QImage &image)
{
    if (!isCameraRunning) return;

    {
        QMutexLocker locker(&m_imageMutex);//加锁
        // 如果图像尺寸与显示区域匹配，直接使用
        if (image.size() == ui->camera_widget->size()) {
            m_currentImage = image;
        } else {
            m_currentImage = image.scaled(ui->camera_widget->size(),
                                         Qt::IgnoreAspectRatio,
                                         Qt::FastTransformation);
        }
    }//解锁
    if (ui->camera_widget) {
        ui->camera_widget->update();
    }
}

bool Camera::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->camera_widget && event->type() == QEvent::Paint) {
        QPainter painter(ui->camera_widget);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.setRenderHint(QPainter::Antialiasing, false);

        QMutexLocker locker(&m_imageMutex);
        if (!m_currentImage.isNull()) {
            // 直接绘制，不再缩放！（因为已经是正确尺寸）
            painter.drawImage(ui->camera_widget->rect(), m_currentImage);
        } else {
            painter.fillRect(ui->camera_widget->rect(), Qt::black);
            painter.setPen(Qt::white);
            painter.drawText(ui->camera_widget->rect(), Qt::AlignCenter, "未获取到图像数据");
        }
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void Camera::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}


void Camera::on_pushButton_clicked()
{
    isBuzzerEnabled = !isBuzzerEnabled;

    if (isBuzzerEnabled) {
        ui->pushButton->setText("关闭蜂鸣器");
        if (ui->status_buzzer) {
            ui->status_buzzer->setText("蜂鸣器：已启用");
        }
        qDebug() << "✅ Buzzer alarm enabled (alert when distance < 10cm)";

        // 测试蜂鸣器（短响一声）
        buzzerOn();
        buzzerTimer->start(200);
    } else {
        ui->pushButton->setText("开启蜂鸣器");
        if (ui->status_buzzer) {
            ui->status_buzzer->setText("蜂鸣器：未启用");
        }
        buzzerOff();
        qDebug() << "❌ Buzzer alarm disabled";
    }
}
