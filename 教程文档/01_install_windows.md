# 01 Windows 安装教程

## 1. 安装 Arduino IDE

1. 下载 Arduino IDE 2.x。
2. 双击安装包。
3. 安装目录建议使用英文路径，例如：

```text
D:\ArduinoIDE
```

首次打开可能会提示安装一些内置组件，等待完成即可。

---

## 2. 添加 ESP32 国内板卡包地址

打开：

```text
File → Preferences
```

在：

```text
Additional Boards Manager URLs
```

填入：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

如果已经有其他地址，用英文逗号隔开。

---

## 3. 安装 ESP32 板卡包

打开：

```text
Tools → Board → Boards Manager
```

搜索：

```text
esp32
```

选择：

```text
esp32 by Espressif Systems
```

选择带 `-cn` 后缀的版本安装。

安装完成后重启 Arduino IDE。

---

## 4. 安装 USB 串口驱动

如果设备管理器里没有 COM 口，按板子芯片安装对应驱动：

- CH340/CH341：安装 CH341SER
- CH343/CH342：安装 CH343SER
- 原生 USB 的 ESP32-S3：多数情况下 Windows 10/11 可以自动识别

---

## 5. 连接开发板

打开设备管理器，查看：

```text
端口（COM 和 LPT）
```

插拔开发板，观察哪个 COM 口新增。

---

## 6. 上传第一个程序

设置：

```text
Board: ESP32S3 Dev Module
Port: COMx
Upload Speed: 460800
USB CDC On Boot: Enabled
```

上传 `examples/01_blink_external_led`。

若失败：

1. 按住 BOOT 再点上传；
2. 出现 `Connecting...` 后松开 BOOT；
3. 把 Upload Speed 改为 115200；
4. 换 USB 数据线。
