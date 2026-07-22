#ifndef ROTATABLELABEL_H
#define ROTATABLELABEL_H
#include <QLabel>
#include <QPainter>

class RotatableLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
public:
    explicit RotatableLabel(QWidget *parent = nullptr);
public:
    qreal m_rotation; // 存储当前角度
    void setRotation(qreal rotation);
    qreal rotation()const;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // ROTATABLELABEL_H
