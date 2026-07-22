QT       += core gui network multimedia multimediawidgets serialport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    camera/camera.cpp \
    environment/environment.cpp \
    main.cpp \
    map/gps.cpp \
    setting/setting.cpp \
    app/speedmeter.cpp \
    widget.cpp \
    weather/weather.cpp \
    weather/day.cpp \
    music/musicplayer.cpp \
    music/rotatablelabel.cpp \
    video/video.cpp \
    map/map.cpp \
    time/analogclock.cpp \
    time/networktimemanager.cpp \
    wifi/wifi.cpp

HEADERS += \
    camera/camera.h \
    environment/environment.h \
    map/gps.h \
    setting/setting.h \
    app/speedmeter.h \
    widget.h \
    weather/weather.h \
    weather/day.h \
    music/musicplayer.h \
    music/rotatablelabel.h \
    video/video.h \
    map/map.h \
    time/analogclock.h \
    time/networktimemanager.h \
    wifi/wifi.h

FORMS += \
    camera/camera.ui \
    environment/environment.ui \
    setting/setting.ui \
    app/speedmeter.ui \
    widget.ui \
    weather/weather.ui \
    music/musicplayer.ui \
    video/video.ui \
    map/map.ui \
    wifi/wifi.ui

# 添加头文件搜索路径
INCLUDEPATH += $$PWD/weather \
               $$PWD/music \
               $$PWD/video \
               $$PWD/map \
               $$PWD/time \
               $$PWD/wifi \
               $$PWD/app \
               $$PWD/environment \
               $$PWD/camera \
               $$PWD/setting

# 如果检测到是 ARM 架构（开发板编译环境）
contains(QT_ARCH, arm) {
    DEFINES += IS_BOARD_ENV
} else {
    DEFINES += IS_PC_ENV
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ../res/MainWidget/MainWidget.qrc \
    ../res/res.qrc
