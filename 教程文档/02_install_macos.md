# 02 macOS 安装提示

## 1. 安装 Arduino IDE

下载 `.dmg` 文件，打开后把 Arduino IDE 拖入 Applications。

如果提示“无法验证开发者”，可在：

```text
系统设置 → 隐私与安全性
```

允许打开。

---

## 2. 添加 ESP32 国内板卡包

Arduino IDE：

```text
Arduino IDE → Settings / Preferences
```

添加：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

安装带 `-cn` 后缀的 `esp32 by Espressif Systems`。

---

## 3. 串口识别

macOS 下常见串口名称：

```text
/dev/cu.usbserial-xxxx
/dev/cu.wchusbserial-xxxx
/dev/cu.usbmodemxxxx
```

如果没有串口：

1. 换数据线；
2. 换 USB-C 转接器；
3. 安装 WCH macOS 驱动；
4. 系统设置中允许驱动扩展。
