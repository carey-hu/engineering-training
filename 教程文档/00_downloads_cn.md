# 00 国内下载链接汇总

> 目标：让学生在国内网络环境下尽量少踩坑。教师课前最好统一下载并分发离线包。

## 1. Arduino IDE

### 国内备用下载页

```text
https://arduino.me/download
```

说明：这是中文社区下载页，优点是国内访问方便；缺点是可能不是官方最新版。课堂教学以“可稳定安装”为第一目标时可用作备用。

### 官方下载页

```text
https://www.arduino.cc/en/software
```

说明：官方最新版来源。若学生网络能正常访问，优先使用官方版本。

---

## 2. ESP32 Arduino 板卡包：国内镜像

在 Arduino IDE 的“其他开发板管理器地址”中填入：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

注意：

1. 这是 Espressif 官方文档给国内用户推荐的 Jihulab 镜像。
2. Boards Manager 里要安装带 `-cn` 后缀的版本。
3. 国内镜像包通常不支持 IDE 自动更新，后续升级需要手动选择新的 `-cn` 版本。

---

## 3. USB 串口驱动

### CH340 / CH341 驱动

```text
https://www.wch.cn/downloads/CH341SER_EXE.html
```

适用：很多低成本 ESP32、Arduino、USB-TTL 模块。

### CH343 / CH342 / CH344 / WCHLinkSER 驱动

```text
https://www.wch.cn/downloads/CH343SER_EXE.html
```

适用：较新的 WCH USB CDC 串口芯片。

### macOS CH34x 驱动

```text
https://www.wch.cn/downloads/ch34xser_mac_zip.html
```

说明：新版本 macOS 有时需要在“系统设置 → 隐私与安全性”里允许驱动扩展。

---

## 4. 建议教师统一打包的资源

```text
Arduino IDE 安装包
CH341SER_EXE.exe
CH343SER_EXE.exe
本仓库 ZIP
ESP32-S3 引脚图 PDF/PNG
课堂示例代码
```

建议命名：

```text
ESP32S3_Arduino_课堂环境包_2026.zip
```
