#ifndef WIFI_H
#define WIFI_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QFrame>
#include <QProcess>
#include <QGridLayout>

class Wifi : public QWidget
{
    Q_OBJECT

public:
    explicit Wifi(QWidget *parent = nullptr);
    ~Wifi();

private slots:
    void on_btn_scan_clicked();
    void on_btn_connect_clicked();
    void on_wifiList_itemClicked(QListWidgetItem *item);
    void on_btn_back_clicked();
    void on_key_clicked();       // 处理键盘按键点击

private:
    QVBoxLayout *mainLayout;
    QHBoxLayout *topLayout;
    QLabel *lab_title;
    QPushButton *btn_scan;
    QListWidget *wifiList;

    // 输入区域
    QFrame *inputGroup;
    QVBoxLayout *inputLayout;
    QLabel *lab_selected;
    QLineEdit *edit_pwd;

    // 键盘区域
    QFrame *keyBoardGroup;
    QGridLayout *gridLayout;

    QHBoxLayout *btnLayout;
    QPushButton *btn_connect;
    QPushButton *btn_back;

    QProcess *scanProcess;
    QString selectedSsid;

    void setupUI();
    void createKeyBoard(); // 创建虚拟键盘
};

#endif
