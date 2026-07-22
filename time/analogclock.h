#ifndef ANALOGCLOCK_H
#define ANALOGCLOCK_H

#include <QWidget>

class analogclock : public QWidget
{
    Q_OBJECT
public:
    explicit analogclock(QWidget *parent = nullptr);

signals:
protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // ANALOGCLOCK_H
