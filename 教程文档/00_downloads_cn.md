# 00 国内下载链接汇总

> [!TIP]
> 本篇是 [学习路线](00_learning_path.md) 的第 1 步。目标是在国内网络环境下尽量少踩坑，先把 Arduino IDE、ESP32 板卡包和串口驱动准备好。

## 下载清单

| 名称 | 用途 | 系统 | 链接 |
|------|------|------|------|
| Arduino IDE | 编程、编译、上传 | Windows/macOS/Linux | [官方下载](https://www.arduino.cc/en/software) / [国内镜像](https://arduino.me/download) |
| ESP32 板卡包镜像地址 | 安装 ESP32 支持 | Arduino IDE 内使用 | `https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json` |
| CH340/CH341 串口驱动 | 识别 ESP32 的 USB 串口 | Windows | [CH341SER.EXE](https://www.wch.cn/downloads/CH341SER_EXE.html) |
| CH343/CH342 串口驱动 | 较新的 WCH USB 串口芯片 | Windows | [CH343SER.EXE](https://www.wch.cn/downloads/CH343SER_EXE.html) |
| CH34x 驱动（macOS） | macOS 串口驱动 | macOS | [ch34xser_mac.zip](https://www.wch.cn/downloads/ch34xser_mac_zip.html) |
| ESP32-S3 开发板资料页 | 原理图、引脚图、固件 | — | [新录官网资料页](https://www.xinlucity.com/?s=resourcedetail/index/id/61.html) |

---

## 1. Arduino IDE

### 官方下载页（推荐）

```text
https://www.arduino.cc/en/software
```

官方最新版来源。如果网络能正常访问，优先使用官方版本。

### 国内备用下载页

```text
https://arduino.me/download
```

> [!TIP]
> 这是中文社区下载页，优点是国内访问方便；缺点是可能不是官方最新版。安装失败时可作为备用下载入口。

---

## 2. ESP32 Arduino 板卡包：国内镜像

在 Arduino IDE 的"其他开发板管理器地址"中填入：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

> [!IMPORTANT]
> 注意事项：
> 1. 这是 Espressif 官方文档给国内用户推荐的 Jihulab 镜像。
> 2. Boards Manager 里要安装带 `-cn` 后缀的版本。
> 3. 国内镜像包通常不支持 IDE 自动更新，后续升级需要手动选择新的 `-cn` 版本。

---

## 3. USB 串口驱动

### CH340 / CH341 驱动（最常用）

```text
https://www.wch.cn/downloads/CH341SER_EXE.html
```

适用：很多低成本 ESP32、Arduino、USB-TTL 模块。

### CH343 / CH342 / CH344 驱动

```text
https://www.wch.cn/downloads/CH343SER_EXE.html
```

适用：较新的 WCH USB CDC 串口芯片。

### macOS CH34x 驱动

```text
https://www.wch.cn/downloads/ch34xser_mac_zip.html
```

> [!NOTE]
> 新版本 macOS 有时需要在"系统设置 → 隐私与安全性"里允许驱动扩展。

---

## 4. 开发板资料页

本教程使用的 ESP32-S3 开发板资料页：

```text
https://www.xinlucity.com/?s=resourcedetail/index/id/61.html
```

资料页包含原理图、参考例程、软件下载、引脚定义、出厂固件和注意事项。遇到开发板引脚、板载灯、5V-IN、PSRAM 或烧录问题时，可以先查这个页面。

---

## 5. 建议提前准备的资源

如果需要给学生准备离线安装包，建议打包以下内容：

```text
Arduino IDE 安装包
CH341SER_EXE.exe
CH343SER_EXE.exe
本仓库 ZIP
ESP32-S3 引脚图 PDF/PNG
示例代码
```

建议命名：

```text
ESP32S3_Arduino_学习资料包_2026.zip
```

---

**[← 上一篇：学习路线](00_learning_path.md)** ·
[返回学习路线](00_learning_path.md) ·
**[下一篇：Windows 安装 →](01_install_windows.md)**
