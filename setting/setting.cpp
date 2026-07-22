#include "setting.h"
#include "ui_setting.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QLabel>

// 开发板亮度文件路径
#ifdef IS_BOARD_ENV
#define BRIGHTNESS_PATH "/sys/class/backlight/backlight/brightness"
#define MAX_BRIGHTNESS_PATH "/sys/devices/platform/backlight/backlight/backlight/max_brightness"
#define ACTUAL_BRIGHTNESS_PATH "/sys/devices/platform/backlight/backlight/backlight/actual_brightness"
#endif

Setting::Setting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Setting)
{
    ui->setupUi(this);

#ifdef IS_BOARD_ENV
    initBrightnessControl();
#else
    // 非开发板环境，可以禁用亮度控件或隐藏
    if (ui->LightBar) {
        ui->LightBar->setEnabled(false);
        ui->LightBar->setVisible(false);
    }
#endif

    // 设置label字体大小为30px（样式表方式）
    this->setStyleSheet("QLabel { font-size: 30px; }");
}

Setting::~Setting()
{
    delete ui;
}

// 槽函数实现：放在条件编译外部
void Setting::onLightBarValueChanged(int value)
{
#ifdef IS_BOARD_ENV
    setBrightness(value);
    qDebug() << "Brightness changed to:" << value;
#else
    Q_UNUSED(value);
    // 非开发板环境，仅做调试输出
    qDebug() << "Brightness would be set to:" << value << "(not in board env)";
#endif
}

#ifdef IS_BOARD_ENV
void Setting::initBrightnessControl()
{
    if (!ui->LightBar) {
        qDebug() << "LightBar not found!";
        return;
    }

    // 获取最大亮度值并设置滑动条范围
    int maxBrightness = getMaxBrightness();
    if (maxBrightness > 0) {
        ui->LightBar->setRange(0, maxBrightness);
    } else {
        // 如果无法获取最大亮度，使用默认范围 0-255
        ui->LightBar->setRange(0, 255);
        maxBrightness = 255;
    }

    // 获取当前亮度值并设置滑动条位置
    int currentBrightness = getCurrentBrightness();
    if (currentBrightness >= 0 && currentBrightness <= maxBrightness) {
        ui->LightBar->setValue(currentBrightness);
    } else {
        ui->LightBar->setValue(maxBrightness / 2); // 默认设置为中等亮度
    }

    // 连接滑动条的valueChanged信号
    connect(ui->LightBar, &QSlider::valueChanged,
            this, &Setting::onLightBarValueChanged);

    qDebug() << "Brightness control initialized: max=" << maxBrightness
             << ", current=" << currentBrightness;
}

void Setting::setBrightness(int level)
{
    QFile brightnessFile(BRIGHTNESS_PATH);
    if (!brightnessFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open brightness file:" << BRIGHTNESS_PATH;
        return;
    }

    QTextStream out(&brightnessFile);
    out << level;
    brightnessFile.close();

    // 验证设置是否成功
    int actualBrightness = getCurrentBrightness();
    if (actualBrightness != level) {
        qDebug() << "Warning: Set brightness to" << level
                 << "but actual brightness is" << actualBrightness;
    }
}

int Setting::getCurrentBrightness()
{
    QFile brightnessFile(ACTUAL_BRIGHTNESS_PATH);
    if (!brightnessFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open actual brightness file:" << ACTUAL_BRIGHTNESS_PATH;
        return -1;
    }

    QTextStream in(&brightnessFile);
    int value = 0;
    in >> value;
    brightnessFile.close();

    return value;
}

int Setting::getMaxBrightness()
{
    QFile maxFile(MAX_BRIGHTNESS_PATH);
    if (!maxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open max brightness file:" << MAX_BRIGHTNESS_PATH;
        return -1;
    }

    QTextStream in(&maxFile);
    int value = 0;
    in >> value;
    maxFile.close();

    return value;
}
#endif // IS_BOARD_ENV
