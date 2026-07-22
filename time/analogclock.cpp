#include "analogclock.h"
#include <QPainter>
#include <QTime>
#include <QTimer>

analogclock::analogclock(QWidget *parent) : QWidget(parent)
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&analogclock::update));
    timer->start(1000);

    // 设置默认大小
    setMinimumSize(120, 120);

}

void analogclock::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QTime time = QTime::currentTime();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // --- 重点修改 1：把 side 的占比从 0.5 调大到 0.9 ---
    // 这样表盘会几乎占满这 120 像素的宽度，视觉上就“变大”了
    int side = qMin(width(), height()) * 0.9;

    // --- 重点修改 2：中心位置稍微下移 ---
    // height() * 0.5 是几何中心。如果想离顶部远点，可以设为 0.5
    painter.translate(width() / 2, height() * 0.5);

    // 缩放坐标系
    painter.scale(side / 200.0, side / 200.0);

    // --- 1. 绘制刻度线 (稍微加长，让表盘更有存在感) ---
    QPen pen;
    pen.setColor(Qt::white);
    pen.setWidth(4);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    for (int i = 0; i < 12; ++i) {
        painter.drawLine(0, -95, 0, -80); // 刻度外移并加长
        painter.rotate(30.0);
    }

    // --- 2. 绘制指针 (保持原有比例，但稍微加粗) ---
    // 时针 (红色)
    painter.save();
    pen.setColor(QColor(255, 0, 0));
    pen.setWidth(10); // 加粗到 10，让小尺寸下更清晰
    painter.setPen(pen);
    painter.rotate(30.0 * ((time.hour() % 12) + time.minute() / 60.0));
    painter.drawLine(0, 5, 0, -50);
    painter.restore();

    // 分针 (蓝色)
    painter.save();
    pen.setColor(QColor(0, 0, 255));
    pen.setWidth(6); // 加粗到 6
    painter.setPen(pen);
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.drawLine(0, 8, 0, -75);
    painter.restore();

    // 秒针 (白色)
    painter.save();
    pen.setColor(Qt::white);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.rotate(6.0 * time.second());
    painter.drawLine(0, 10, 0, -90);
    painter.restore();

    // --- 3. 中心白点 ---
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPoint(0, 0), 6, 6);
}
