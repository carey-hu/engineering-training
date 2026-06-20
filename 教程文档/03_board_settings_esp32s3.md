# 03 ESP32-S3 板卡参数设置

## 1. 通用板型

大多数普通 ESP32-S3 开发板可先选择：

```text
Tools → Board → esp32 → ESP32S3 Dev Module
```

如果你的板子厂家在菜单中提供了专用型号，优先选择厂家专用型号。

---

## 2. 推荐参数

```text
Board: ESP32S3 Dev Module
Port: 实际 COM 口
Upload Speed: 460800，失败时改 115200
CPU Frequency: 240MHz
Flash Mode: QIO 或 DIO
Flash Size: 按实际板子选择，常见 8MB 或 16MB
Partition Scheme: Default
PSRAM: 有则 Enabled，无则 Disabled
USB CDC On Boot: Enabled
```

如果使用的是本教程对应的 ESP32-S3 N16R8 开发板，资料页说明该核心板带 8MB PSRAM。Arduino IDE 中通常需要把 PSRAM 设为 `OPI PSRAM` 或对应的 Enabled 选项。菜单名称会随 Arduino-ESP32 版本略有不同，实操时以 IDE 中显示的选项为准。

---

## 3. 原生 USB 与串口芯片区别

### 原生 USB

特点：ESP32-S3 芯片自己提供 USB 功能。常见设备名可能是：

```text
USB JTAG/serial debug unit
USB Serial Device
```

建议：

```text
USB CDC On Boot: Enabled
```

这样 `Serial.print()` 才更容易从 USB 串口输出。

### 外置 USB 转串口芯片

常见芯片：

```text
CH340 / CH343 / CP2102
```

这种板子通常会出现普通 COM 口，上传比较接近传统 ESP32 开发板。

ESP32 开发板资料页提示，UART0 的 GPIO43/GPIO44 用于调试和下载，不建议再接外部模块。本教程的 RS485 示例默认使用 GPIO17/GPIO18 连接课程配套 RS485/供电转接板或外部 RS485 模块。

---

## 4. BOOT 与 EN 按键

- `BOOT`：进入下载模式。
- `EN` / `RST`：复位。

上传失败时常用操作：

```text
按住 BOOT → 点击上传 → 出现 Connecting... → 松开 BOOT
```

如果程序运行异常，按一下 `EN` 复位。
