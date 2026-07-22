#ifndef MAP_H
#define MAP_H

#include <QWidget>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QTimer>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QtMath>
#include "gps.h"

namespace Ui {
class map;
}

class map : public QWidget
{
    Q_OBJECT

public:
    explicit map(QWidget *parent = nullptr);
    ~map();

private slots:
    // ========== 按钮槽函数 ==========
    void on_startNavBtn_clicked();   // 开始导航
    void on_zoomInBtn_clicked();     // 放大
    void on_zoomOutBtn_clicked();    // 缩小
    void on_resetBtn_clicked();      // 重置

    // ========== 网络响应处理 ==========
    void handleRouteReply(QNetworkReply *reply);

protected:
    // ========== 事件处理 ==========
    void paintEvent(QPaintEvent *event) override;        // 绘制地图
    void mousePressEvent(QMouseEvent *event) override;   // 鼠标按下
    void mouseReleaseEvent(QMouseEvent *event) override; // 鼠标释放

private:
    // ========== UI组件 ==========
    Ui::map *ui;

    // ========== 网络相关 ==========
    QNetworkAccessManager *routeManager;  // 网络管理器

    // ========== 路径规划数据 ==========
    QString currentPathPoints;  // 当前路径点集合
    QString startLatLon;        // 起点经纬度
    QString endLatLon;          // 终点经纬度

    // ========== 地图状态 ==========
    enum NavState { STATE_LOCATING, STATE_NAVIGATING };  // 当前模式
    NavState currentState = STATE_LOCATING;

    // ========== 地图显示参数 ==========
    int currentZoom;            // 当前缩放级别 (3-19)
    double currentCenterLat;    // 当前地图中心纬度
    double currentCenterLon;    // 当前地图中心经度
    QString currentMarkerPos;   // 当前标记位置

    // ========== GPS数据 ==========
    double currentGpsLat = 0.0;     // 当前GPS纬度
    double currentGpsLon = 0.0;     // 当前GPS经度
    double lastRequestedLat = 0.0;  // 上次请求的纬度
    double lastRequestedLon = 0.0;  // 上次请求的经度
    QTimer *gpsTimer;               // GPS超时定时器
    bool gpsInitialized;            // GPS是否初始化成功

    // ========== 鼠标拖拽交互 ==========
    QPoint lastMousePos;        // 上一次鼠标位置
    bool isDragging;            // 是否正在拖拽
    double dragStartLat;        // 拖拽起点纬度
    double dragStartLon;        // 拖拽起点经度
    QPoint dragOffset;          // 拖拽偏移量
    QPixmap cachedPixmap;       // 缓存当前地图图片

    // ========== 核心功能函数 ==========
    void refreshMap();                              // 刷新地图
    void requestCoordConv(double lat, double lon); // 坐标纠偏转换
    void requestMapImageForCoords(double bdLat, double bdLon);  // 请求定位地图
    void requestMapImage();                       // 请求导航地图
    void requestRoutePlan(QString start, QString end);  // 请求路径规划
    void requestGeocoding(QString location, bool isStart); // 地理编码请求
    void parseRouteJson(const QByteArray &data);    // 解析路径规划结果
    void onGpsTimeout();                            // GPS超时处理
};

#endif // MAP_H
