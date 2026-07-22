#ifndef SETTING_H
#define SETTING_H

#include <QWidget>
#include <QSlider>

namespace Ui {
class Setting;
}

class Setting : public QWidget
{
    Q_OBJECT

public:
    explicit Setting(QWidget *parent = nullptr);
    ~Setting();

private slots:
    void onLightBarValueChanged(int value);  // 始终声明，不要放在条件编译中

private:
    Ui::Setting *ui;

#ifdef IS_BOARD_ENV
    void initBrightnessControl();
    void setBrightness(int level);
    int getCurrentBrightness();
    int getMaxBrightness();
#endif
};

#endif // SETTING_H
