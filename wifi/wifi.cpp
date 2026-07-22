#include "wifi.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>

Wifi::Wifi(QWidget *parent) : QWidget(parent) {
    // 强制窗口大小并设置纯黑底色
    this->setFixedSize(480, 272);
    this->setStyleSheet("background-color: black;");

    // --- 新增：进入 WiFi 模块时禁用以太网，避免路由冲突 ---
#ifdef IS_BOARD_ENV
    //system("ifconfig eth0 down");
#endif

    setupUI();
    scanProcess = new QProcess(this);
}

// --- 修改：析构函数，在退出程序/销毁窗口时恢复以太网 ---
Wifi::~Wifi() {
#ifdef IS_BOARD_ENV
    system("ifconfig eth0 up");
#endif
}

void Wifi::setupUI() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);

    // 1. 顶部栏 (初始隐藏标题)
    topLayout = new QHBoxLayout();
    lab_title = new QLabel("WiFi设置", this);
    lab_title->setStyleSheet("font-weight: bold; font-size: 16px; color: white;");
    lab_title->hide();

    // 2. 扫描按钮 (btn_scan) - 初始中心样式
    btn_scan = new QPushButton("扫描", this);
    btn_scan->setFixedSize(180, 60);
    btn_scan->setStyleSheet(
                "QPushButton {"
                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00AFFF, stop:1 #0078D4);"
                "color: white; font-size: 22px; font-weight: bold; border-radius: 12px; border: 2px solid #FFFFFF;"
                "}"
                "QPushButton:pressed { background: #005A9E; }"
                );

    topLayout->addWidget(lab_title);
    topLayout->addStretch();

    mainLayout->addStretch();
    mainLayout->addWidget(btn_scan, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // 3. WiFi列表 (初始隐藏)
    wifiList = new QListWidget(this);
    wifiList->setStyleSheet(
                "QListWidget { background: #111; color: white; border: 1px solid #444; font-size: 16px; }"
                "QListWidget::item { height: 45px; border-bottom: 1px solid #333; }"
                "QListWidget::item:selected { background: #0078D4; }"
                );
    wifiList->hide();
    mainLayout->addWidget(wifiList);

    // 4. 输入及键盘区域
    inputGroup = new QFrame(this);
    inputGroup->setStyleSheet("QFrame { background: #222; border: 1px solid #555; } QLabel { color: white; }");
    inputLayout = new QVBoxLayout(inputGroup);

    lab_selected = new QLabel(inputGroup);
    edit_pwd = new QLineEdit(inputGroup);
    edit_pwd->setPlaceholderText("输入密码...");
    edit_pwd->setEchoMode(QLineEdit::Password);
    edit_pwd->setFixedHeight(35);
    edit_pwd->setStyleSheet("background: white; color: black; font-size: 18px;");

    createKeyBoard();

    btnLayout = new QHBoxLayout();
    btn_back = new QPushButton("取消", inputGroup);
    btn_connect = new QPushButton("确认连接", inputGroup);
    btn_connect->setStyleSheet("background: #0078D4; color: white; font-weight: bold; height: 35px;");
    btn_back->setStyleSheet("background: #555; color: white; height: 35px;");

    btnLayout->addWidget(btn_back);
    btnLayout->addWidget(btn_connect);

    inputLayout->addWidget(lab_selected);
    inputLayout->addWidget(edit_pwd);
    inputLayout->addWidget(keyBoardGroup);
    inputLayout->addLayout(btnLayout);

    mainLayout->addWidget(inputGroup);
    inputGroup->hide();

    // 信号连接
    connect(btn_scan, &QPushButton::clicked, this, &Wifi::on_btn_scan_clicked);
    connect(btn_connect, &QPushButton::clicked, this, &Wifi::on_btn_connect_clicked);
    connect(btn_back, &QPushButton::clicked, this, &Wifi::on_btn_back_clicked);
    connect(wifiList, &QListWidget::itemClicked, this, &Wifi::on_wifiList_itemClicked);
}

void Wifi::createKeyBoard() {
    keyBoardGroup = new QFrame(this);
    gridLayout = new QGridLayout(keyBoardGroup);
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    QStringList keys = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
        "a", "s", "d", "f", "g", "h", "j", "k", "l", "<-"
    };

    int row = 0, col = 0;
    for (const QString &text : keys) {
        QPushButton *btn = new QPushButton(text, keyBoardGroup);
        btn->setFixedSize(44, 32);
        btn->setStyleSheet("background: #444; color: white; border-radius: 4px; font-weight: bold;");
        connect(btn, &QPushButton::clicked, this, &Wifi::on_key_clicked);
        gridLayout->addWidget(btn, row, col);
        col++;
        if (col > 9) { col = 0; row++; }
    }
}

void Wifi::on_key_clicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString val = btn->text();
    if (val == "<-") edit_pwd->backspace();
    else edit_pwd->insert(val);
}

void Wifi::on_btn_scan_clicked() {
    if (btn_scan->height() > 30) {
        btn_scan->setFixedSize(80, 30);
        btn_scan->setText("刷新");
        btn_scan->setStyleSheet("background: #444; color: white; border-radius: 5px; font-size: 14px;");

        for(int i=0; i < mainLayout->count(); ++i) {
            if (mainLayout->itemAt(i)->spacerItem()) {
                mainLayout->removeItem(mainLayout->itemAt(i));
            }
        }
        topLayout->addWidget(btn_scan);
        lab_title->show();
        wifiList->show();
    }

    btn_scan->setEnabled(false);
    wifiList->clear();
    scanProcess->start("iwlist", QStringList() << "wlan0" << "scan");

    connect(scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](){
        QString output = scanProcess->readAllStandardOutput();
        QStringList lines = output.split("\n");
        for (const QString &line : lines) {
            if (line.contains("ESSID:")) {
                QString ssid = line.split(":").last().trimmed().remove("\"");
                if (!ssid.isEmpty() && wifiList->findItems(ssid, Qt::MatchExactly).isEmpty())
                    wifiList->addItem(ssid);
            }
        }
        btn_scan->setEnabled(true);
        scanProcess->disconnect(this);
    });
}

void Wifi::on_wifiList_itemClicked(QListWidgetItem *item) {
    selectedSsid = item->text();
    lab_selected->setText("WiFi: " + selectedSsid);
    wifiList->hide();
    lab_title->hide();
    btn_scan->hide();
    inputGroup->show();
}

void Wifi::on_btn_back_clicked() {
    inputGroup->hide();
    wifiList->show();
    btn_scan->show();
    lab_title->show();
    edit_pwd->clear();
}

void Wifi::on_btn_connect_clicked() {
    QString pwd = edit_pwd->text();
    if (pwd.length() < 8) return;

    QFile file("/etc/wpa_supplicant.conf");
#ifndef IS_BOARD_ENV
    file.setFileName(QCoreApplication::applicationDirPath() + "/wpa_test.conf");
#endif

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "ctrl_interface=/var/run/wpa_supplicant\n"
            << "update_config=1\n"
            << "ap_scan=1\n"
            << "network={\n"
            << "  ssid=\"" << selectedSsid << "\"\n"
            << "  psk=\"" << pwd << "\"\n"
            << "}\n";
        file.close();
    }

#ifdef IS_BOARD_ENV
    system("ifconfig wlan0 up");
    system("wpa_supplicant -D wext -c /etc/wpa_supplicant.conf -i wlan0 -B");
    system("sleep 2 && udhcpc -i wlan0 &");
#endif

    on_btn_back_clicked();
}
