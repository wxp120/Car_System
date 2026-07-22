#include "rotatablelabel.h"
#include <QDebug>

RotatableLabel::RotatableLabel(QWidget *parent) : QLabel(parent), m_rotation(0.0)
{
    // 构造函数：初始化旋转角度为 0
}

/**
 * @brief 动画引擎会自动调用这个 Setter 函数来改变角度
 * @param rotation 动画计算出的当前绝对角度（0.0 ~ 360.0）
 */
void RotatableLabel::setRotation(qreal rotation)
{
    m_rotation = rotation;
    update(); // 关键：请求 Qt 调度一次重绘事件，如果不调用，界面就不会动
}

/**
 * @brief Getter 函数，供动画引擎读取当前角度状态
 */
qreal RotatableLabel::rotation() const
{
    return m_rotation;
}

/**
 * @brief 核心绘图逻辑：每一帧动画都会执行一次该函数
 */
void RotatableLabel::paintEvent(QPaintEvent *event)
{
    // 如果没有设置图片或者图片为空，则执行父类默认的绘图逻辑并返回
    if (!pixmap() || pixmap()->isNull()) {
        QLabel::paintEvent(event);
        return;
    }

    // 初始化画笔，注意：每次进入时 painter 的坐标系都是重置状态（左上角 0,0，角度 0）
    QPainter painter(this);

    // 开启抗锯齿和丝滑缩放，防止图片在旋转时出现毛刺或像素锯齿
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // --- 步骤 1: 变换坐标原点 ---
    // 将绘图原点从左上角 (0,0) 移动到控件的正中心
    QPointF center = rect().center();
    painter.translate(center);

    // --- 步骤 2: 轴心微调 (车载 UI 适配) ---
    // 在中心点的基础上进行垂直偏移。负数向上移，正数向下移
    // 这里的 -10 是为了让旋转轴心在视觉上更符合 CD 盘机的位置
    int offsetY = -10;
    painter.translate(0, offsetY);

    // --- 步骤 3: 旋转坐标系 ---
    // 旋转整个画布。注意：这是绕着当前移动后的原点 (center + offsetY) 旋转的
    painter.rotate(m_rotation);

    // --- 步骤 4: 计算绘图区域，防止切边 ---
    // 取宽和高的最小值作为直径，确保图片是正方形
    int side = qMin(width(), height());

    // 预留 10 像素的安全边距。
    // 因为矩形旋转时角会伸出原有的边界，缩小 drawSide 可以保证图片永远在控件范围内
    int drawSide = side - 10;

    // --- 步骤 5: 绘制图片 ---
    // 由于坐标原点已经在中心，我们需要将图片的左上角定在相对中心的 (-r, -r) 处
    // 这样图片的正中心就会刚好重合在坐标原点上，从而实现“原地自转”
    QRectF targetRect(-drawSide / 2.0, -drawSide / 2.0, drawSide, drawSide);

    // 绘制图片：将原始 pixmap 缩放并绘制到计算好的 targetRect 区域中
    painter.drawPixmap(targetRect, *pixmap(), pixmap()->rect());
}
