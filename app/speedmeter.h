#ifndef SPEEDMETER_H
#define SPEEDMETER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Speedmeter; }
QT_END_NAMESPACE

class Speedmeter : public QWidget
{
    Q_OBJECT

public:
    explicit Speedmeter(QWidget *parent = nullptr);
    ~Speedmeter();

public slots:
    void timeready();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;    // 添加显示事件
    void hideEvent(QHideEvent *event) override;    // 添加隐藏事件
private:
    Ui::Speedmeter *ui;
    int CurrentValue = 0;
    QTimer *timer;
    int flag = 0;
    int startAngle;
    int yOffset = 40;  // 垂直偏移量
    // 每一格对应的角度
    double angle = 240.0 / 60;

    void initCanvas(QPainter &painter);
    void drawMiddleCircle(QPainter &painter, int raduis);
    void drawCurrentSpeed(QPainter &painter);
    void drawScale(QPainter &painter, int radius);
    void drawScaleText(QPainter &painter, int radius);
    void drawPointLine(QPainter &painter);
    void drawSpeedPie(QPainter &painter, int radius);
    void drawEllipseInnerBlack(QPainter &painter, int radius);
    void drawEllipseInnerShine(QPainter &painter, int radius);
    void drawEllipseoutShine(QPainter &painter, int radius);
    void drawLogo(QPainter &painter, int radius);

    void startRun();
    void stopRun();
};

#endif // SPEEDMETER_H
