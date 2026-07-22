# i.MX6ULL-Based In-Vehicle Intelligent Terminal System

基于 NXP i.MX6ULL 和嵌入式 Linux 开发的车载智能终端系统。项目采用 Qt 构建图形化交互界面，集成 USB 摄像头、超声波测距、网络天气查询、车辆状态显示及多媒体等功能。

## 1. Project Overview

本项目主要用于学习和实践嵌入式 Linux 应用开发、硬件外设控制、多线程编程、网络通信及音视频采集。

系统运行于 i.MX6ULL 开发板，通过 Qt 实现车载终端界面，并将摄像头、传感器和网络数据集成到统一的人机交互界面中。

![System Overview](docs/images/system-overview.png)

## 2. Main Features

* 基于 Qt 实现车载图形化交互界面
* 基于 V4L2 实现 USB 摄像头视频采集
* 基于 HC-SR04 实现超声波距离测量
* 使用多线程完成传感器数据采集与界面更新
* 通过 HTTP API 获取城市天气信息
* 支持车辆状态及环境信息显示
* 支持嵌入式 Linux 下的 NFS 和 SSH 调试
* 支持交叉编译及开发板部署

## 3. System Architecture

```text
┌──────────────────────────────────────┐
│              Qt GUI                  │
│                                      │
│  Vehicle   Camera   Distance Weather │
└───────────────┬──────────────────────┘
                │
┌───────────────▼──────────────────────┐
│        Application Service Layer     │
│                                      │
│  V4L2 Capture │ HTTP │ Sensor Thread │
└───────┬──────────┬──────────┬────────┘
        │          │          │
┌───────▼───┐ ┌────▼────┐ ┌───▼────────┐
│ USB Camera│ │ Network │ │  HC-SR04   │
└───────────┘ └─────────┘ └────────────┘
                │
┌───────────────▼──────────────────────┐
│      Embedded Linux / i.MX6ULL       │
└──────────────────────────────────────┘
```

## 4. Hardware Platform

* NXP i.MX6ULL development board
* USB UVC camera
* HC-SR04 ultrasonic sensor
* LCD display
* Ethernet interface
* Ubuntu development host

## 5. Software Environment

* Embedded Linux
* Qt 5
* C/C++
* V4L2
* POSIX Threads
* HTTP API
* ARM cross-compilation toolchain
* NFS
* SSH
* Git

## 6. Project Structure

```text
imx6ull-vehicle-system/
├── README.md
├── src/
│   ├── main.cpp
│   ├── camera/
│   ├── distance/
│   ├── weather/
│   └── ui/
├── include/
├── resources/
├── docs/
│   └── images/
│       ├── system-overview.png
│       ├── home-page.png
│       ├── camera-page.png
│       └── weather-page.png
├── scripts/
└── vehicle-system.pro
```

实际目录结构可根据项目代码进行调整。

## 7. Development Environment

开发板与 Ubuntu 主机通过局域网连接：

```text
Development Board: 192.168.10.50
Ubuntu Host:       192.168.10.100
```

开发板默认网关设置为 Ubuntu：

```bash
ip route replace default via 192.168.10.100 dev eth0
```

Ubuntu 需要开启 IPv4 转发和 NAT，使开发板能够访问互联网。

```bash
sudo sysctl -w net.ipv4.ip_forward=1

sudo iptables -t nat -A POSTROUTING \
-s 192.168.10.0/24 \
-o ens37 \
-j MASQUERADE

sudo iptables -A FORWARD \
-i ens33 \
-o ens37 \
-s 192.168.10.0/24 \
-j ACCEPT

sudo iptables -A FORWARD \
-i ens37 \
-o ens33 \
-d 192.168.10.0/24 \
-m conntrack --ctstate ESTABLISHED,RELATED \
-j ACCEPT
```

其中：

* `ens33` 为 Ubuntu 与开发板连接的网卡
* `ens37` 为 Ubuntu 访问互联网的网卡

使用前请根据实际网卡名称进行修改。

## 8. NFS Mount

在开发板中创建挂载目录：

```bash
mkdir -p /mnt/nfs
```

挂载 Ubuntu 共享目录：

```bash
mount -t nfs -o nolock,nfsvers=3 \
192.168.10.100:/home/user/nfs \
/mnt/nfs
```

检查挂载状态：

```bash
mount | grep nfs
```

## 9. Build

在 Ubuntu 主机中配置 ARM 交叉编译工具链和 Qt 编译环境。

使用 qmake：

```bash
qmake vehicle-system.pro
make -j$(nproc)
```

或者根据实际 Qt 工具链执行：

```bash
/path/to/arm-qmake vehicle-system.pro
make -j$(nproc)
```

编译完成后，将程序复制到 NFS 共享目录：

```bash
cp vehicle-system /home/user/nfs/
```

## 10. Run

在开发板中进入程序目录：

```bash
cd /mnt/nfs
```

添加执行权限：

```bash
chmod +x vehicle-system
```

运行程序：

```bash
./vehicle-system
```

如果需要指定 Qt 显示平台：

```bash
./vehicle-system -platform linuxfb
```

具体参数根据开发板 Qt 环境进行调整。

## 11. USB Camera

插入 USB 摄像头后检查设备：

```bash
lsusb
v4l2-ctl --list-devices
ls -l /dev/video*
```

正常情况下，USB 摄像头会生成新的 V4L2 设备节点，例如：

```text
/dev/video1
/dev/video2
```

注意：i.MX6ULL 系统中的 `/dev/video0` 可能属于 PXP 图像处理设备，并不一定是 USB 摄像头。

可使用以下命令查看摄像头支持的格式：

```bash
v4l2-ctl -d /dev/video2 --list-formats-ext
```

测试视频采集：

```bash
v4l2-ctl -d /dev/video2 \
--stream-mmap=3 \
--stream-count=10 \
--stream-to=test.raw
```

程序中的设备路径应根据实际生成的节点进行修改。

## 12. Weather API

系统通过 HTTP 请求获取天气数据。

请求格式示例：

```text
http://example.com/api?version=v9&city=Qingdao
```

建议不要将真实的 `appid` 和 `appsecret` 直接提交到公开仓库。

可以创建本地配置文件：

```text
config/weather.conf
```

配置示例：

```ini
APP_ID=your_app_id
APP_SECRET=your_app_secret
CITY=Qingdao
```

并将配置文件加入 `.gitignore`：

```gitignore
config/weather.conf
```

## 13. Screenshots

### Home Page

![Home Page](docs/images/home-page.png)

### Camera Page

![Camera Page](docs/images/camera-page.png)

### Distance Measurement

![Distance Measurement](docs/images/distance-page.png)

### Weather Page

![Weather Page](docs/images/weather-page.png)

## 14. Troubleshooting

### Camera device cannot be opened

错误信息：

```text
ERROR: failed to open video device /dev/video2
```

检查设备节点：

```bash
v4l2-ctl --list-devices
ls -l /dev/video*
```

如果没有识别到 USB 摄像头，检查：

```bash
lsusb
dmesg | tail -n 100
lsmod | grep uvc
```

尝试加载 UVC 驱动：

```bash
modprobe uvcvideo
```

### Development board cannot access the Internet

先测试公网 IP：

```bash
ping -c 3 223.5.5.5
```

如果开发板只能连接 Ubuntu，不能访问公网，需要检查：

* 默认网关是否为 Ubuntu
* Ubuntu 是否开启 IPv4 转发
* Ubuntu 是否配置 NAT
* DNS 是否正确配置

### DNS resolution failure

检查 DNS：

```bash
cat /etc/resolv.conf
```

测试域名解析：

```bash
nslookup www.baidu.com
```

### NFS or SSH connection is interrupted

不要随意修改开发板的 `eth0` 地址。开发板可在保持以下局域网地址的情况下，同时使用 NFS、SSH 和 Ubuntu NAT：

```text
Development Board: 192.168.10.50
Gateway:           192.168.10.100
```

## 15. Future Improvements

* 自动识别 USB 摄像头设备节点
* 增加摄像头断开后的自动重连机制
* 增加天气接口异常处理和数据缓存
* 优化多线程资源管理
* 增加倒车影像和距离告警功能
* 增加车辆 CAN 总线数据接入
* 完善系统日志及错误提示

## 16. License

This project is intended for learning and technical research purposes.
