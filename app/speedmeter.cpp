#include "speedmeter.h"
#include "ui_speedmeter.h"
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QtMath>

// 构造函数
Speedmeter::Speedmeter(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Speedmeter)
{
    ui->setupUi(this);
    qDebug() << "Speedmeter: setupUi done";
    setFixedSize(944,600);
    startAngle = 150;
    // 创建定时器对象
    timer = new QTimer(this);
    // 绑定定时器信号与槽函数
    connect(timer, &QTimer::timeout, this, &Speedmeter::timeready);
    // 设置定时器间隔 50ms
    timer->setInterval(50);
    qDebug() << "Speedmeter: constructor done";
}

// 析构函数
Speedmeter::~Speedmeter()
{
    stopRun();
    delete ui;
}

// 定时器槽函数：数值自动增减
void Speedmeter::timeready()
{
    if (!flag)
    {
        // 递增模式：数值+1
        CurrentValue++;
        if (CurrentValue >= 60)
        {
            // 达到最大值，切换为递减模式
            flag = 1;
        }
    }
    else
    {
        // 递减模式：数值-1
        CurrentValue--;
        if (CurrentValue <= 0)
        {
            // 达到最小值，切换为递增模式
            flag = 0;
        }
    }
    // 刷新界面，重绘绘图事件
    update();
}

// 绘图事件：绘制仪表盘所有内容
void Speedmeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 创建画家对象
    QPainter painter(this);
    // 初始化画布
    initCanvas(painter);
    // 取消画刷填充
    painter.setBrush(Qt::NoBrush);
    // 画小圆
    drawMiddleCircle(painter, 60);
    // ===================== 绘制刻度 =====================
    drawScale(painter, height() / 2);
    // ===================== 绘制刻度文字 =====================
    drawScaleText(painter, height() / 2);
    // ===================== 绘制指针 =====================
    drawPointLine(painter);
    // ===================== 绘制扇形进度条 =====================
    drawSpeedPie(painter, height() / 2 + 20);
    // 绘制渐变色内圆
    drawEllipseInnerShine(painter, 110);
    // 绘制黑色内圆
    drawEllipseInnerBlack(painter, 80);
    // 画当前值
    drawCurrentSpeed(painter);
    // 画外围光圈
    drawEllipseoutShine(painter, height() / 2 + 20);
    // 绘制logo
    drawLogo(painter, height() / 2);
}

void Speedmeter::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    startRun();
}

void Speedmeter::hideEvent(QHideEvent *event)
{
    stopRun();
    QWidget::hideEvent(event);
}

void Speedmeter::initCanvas(QPainter &painter)
{
    // 开启抗锯齿
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 设置黑色背景画刷
    QBrush brush(Qt::black);
    painter.setBrush(brush);
    // 绘制全屏黑色矩形背景
    painter.drawRect(rect());

    // 将坐标原点移到窗口中心
    painter.translate(rect().center().x(), rect().center().y() + yOffset);
}

void Speedmeter::drawMiddleCircle(QPainter &painter, int raduis)
{
    painter.setPen(QPen(Qt::white, 3));
    // 绘制中心白色小圆
    painter.drawEllipse(QPoint(0, 0), raduis, raduis);
}

void Speedmeter::drawCurrentSpeed(QPainter &painter)
{
    painter.setPen(Qt::white);
    // 设置字体
    QFont font("Arial", 30);
    font.setBold(true);
    painter.setFont(font);
    // 在中心小圆内绘制当前数值（居中显示）
    painter.drawText(QRect(-60, -60, 120, 70), Qt::AlignCenter, QString::number(CurrentValue * 4));
    painter.setFont(QFont("Arial", 18));
    painter.drawText(QRect(-60, -60, 120, 160), Qt::AlignCenter, "KM/h");
}

void Speedmeter::drawScale(QPainter &painter, int radius)
{
    // 保存当前坐标系状态
    painter.save();
    // 刻度起始角度：旋转135度
    painter.rotate(startAngle);
    // 设置刻度字体
    QFont font("华文宋体", 15);
    painter.setFont(font);

    // 循环绘制61个刻度（0到60）
    for (int i = 0; i <= 60; i++)
    {
        if (i >= 40)
        {
            painter.setPen(QPen(Qt::red, 5));
        }
        else
        {
            painter.setPen(QPen(Qt::white, 3));
        }
        if (i % 5 == 0)
        {
            // 主刻度：长线
            painter.drawLine(radius - 20, 0, radius - 3, 0);
        }
        else
        {
            // 小刻度：短线
            painter.drawLine(radius - 8, 0, radius - 3, 0);
        }
        // 每画一个刻度，旋转一格角度
        painter.rotate(angle);
    }
    // 恢复坐标系
    painter.restore();
}

void Speedmeter::drawScaleText(QPainter &painter, int radius)
{
    int r = radius - 50;
    for (int i = 0; i <= 60; i++)
    {
        if (i % 5 == 0)
        {
            painter.save();
            // 平移点
            int delX = qCos(qDegreesToRadians(210 - angle * i)) * r;
            int delY = qSin(qDegreesToRadians(210 - angle * i)) * r;
            // 坐标系
            painter.translate(QPoint(delX, -delY));
            painter.rotate(-120 + angle * i);
            // 文字
            QFont font("Arial", 18);
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(-25, -25, 50, 30, Qt::AlignCenter, QString::number(i * 4));
            painter.restore();
        }
    }
}

void Speedmeter::drawPointLine(QPainter &painter)
{
    painter.save();
    // 根据当前值旋转指针角度
    painter.rotate(startAngle + angle * CurrentValue);
    // 绘制指针线条
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    static const QPointF points[4] = {
        QPointF(0, 0.0),
        QPointF(0, 15.0),
        QPointF(200.0, -1.2),
        QPointF(200, 1.2),
    };
    painter.drawPolygon(points, 3);
    // 恢复坐标系
    painter.restore();
}

void Speedmeter::drawSpeedPie(QPainter &painter, int radius)
{
    QRectF rectangle(QPoint(-radius, radius), QPoint(radius, -radius));
    // 取消画笔（无边框）
    painter.setPen(Qt::NoPen);
    // 设置扇形颜色与透明度
    painter.setBrush(QColor(255, 0, 0, 100));
    // 绘制扇形进度区域
    painter.drawPie(rectangle, -startAngle * 16, -CurrentValue * angle * 16);
}

void Speedmeter::drawEllipseInnerBlack(QPainter &painter, int radius)
{
    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(0, 0), radius, radius);
}

void Speedmeter::drawEllipseInnerShine(QPainter &painter, int radius)
{
    QRadialGradient radiaGradient(0, 0, radius);
    radiaGradient.setColorAt(0.6, QColor(255, 0, 0, 200));
    radiaGradient.setColorAt(1.0, QColor(0, 0, 0, 100));
    painter.setBrush(radiaGradient);
    painter.drawEllipse(QPoint(0, 0), radius, radius);
}

void Speedmeter::drawEllipseoutShine(QPainter &painter, int radius)
{
    painter.save();

    QRectF rectangle(-radius, -radius, radius * 2, radius * 2);
    painter.setPen(Qt::NoPen);

    QRadialGradient radiaGradient(0, 0, radius);
    radiaGradient.setColorAt(1.0, QColor(255, 0, 0, 200));   // 最外圈：红色半透明
    radiaGradient.setColorAt(0.95, QColor(255, 0, 0, 120));   // 0.95位置：红色更透明
    radiaGradient.setColorAt(0.9, QColor(0, 0, 0, 0));       // 0.9位置以内：完全透明
    radiaGradient.setColorAt(0.0, QColor(0, 0, 0, 0));       // 圆心：完全透明

    painter.setBrush(radiaGradient);

    // 绘制扇形（360度完整圆环）
    // 起始角度：(360-150)*16 = 210*16，覆盖角度：-angle*61*16（负数表示顺时针）
    painter.drawPie(rectangle, (360 - 150) * 16, -angle * 61 * 16);

    painter.restore();
}

void Speedmeter::drawLogo(QPainter &painter, int radius)
{
    QRect rectangle(-65, radius * 0.35, 130, 50);
    painter.drawPixmap(rectangle, QPixmap(":/img/icon.png"));
}

void Speedmeter::startRun()
{
    if (timer && !timer->isActive()) {
        timer->start();
    }
}

void Speedmeter::stopRun()
{
    if (timer && timer->isActive()) {
        timer->stop();
    }
}
