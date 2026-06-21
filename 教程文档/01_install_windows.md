# 01 Windows 安装教程

> [!TIP]
> 本篇是 [学习路线](00_learning_path.md) 的第 2 步。建议先完成 [下载链接](00_downloads_cn.md) 准备再开始。

## 本节目标

- 在 Windows 上安装 Arduino IDE 2.x
- 添加 ESP32 板卡包国内镜像
- 安装 USB 串口驱动
- 成功上传第一个程序到 ESP32-S3

## 需要的硬件

| 物品 | 用途 |
|------|------|
| ESP32-S3 开发板 | 主控 |
| USB 数据线（能传数据） | 连接电脑和开发板 |

---

## 步骤

### 1. 安装 Arduino IDE

1. 下载 Arduino IDE 2.x（见 [00_downloads_cn.md](00_downloads_cn.md)）。
2. 双击安装包。
3. 安装目录建议使用英文路径，例如：

```text
D:\ArduinoIDE
```

首次打开可能会提示安装一些内置组件，等待完成即可。

---

### 2. 添加 ESP32 国内板卡包地址

1. 打开 Arduino IDE，进入：

```text
File → Preferences
```

2. 在 `Additional Boards Manager URLs` 中填入：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

> [!NOTE]
> 如果已经有其他地址，用英文逗号隔开。

---

### 3. 安装 ESP32 板卡包

1. 打开：

```text
Tools → Board → Boards Manager
```

2. 搜索：

```text
esp32
```

3. 选择：

```text
esp32 by Espressif Systems
```

4. **选择带 `-cn` 后缀的版本安装**。

安装完成后重启 Arduino IDE。

---

### 4. 安装 USB 串口驱动

如果设备管理器里没有 COM 口，按板子芯片安装对应驱动：

| 芯片型号 | 驱动名称 | 下载链接 |
|----------|----------|----------|
| CH340/CH341 | CH341SER | [下载](https://www.wch.cn/downloads/CH341SER_EXE.html) |
| CH343/CH342 | CH343SER | [下载](https://www.wch.cn/downloads/CH343SER_EXE.html) |
| 原生 USB 的 ESP32-S3 | 通常自动识别 | Windows 10/11 自带 |

---

### 5. 连接开发板

1. 打开 Windows 设备管理器：

```text
Win + X → 设备管理器 → 端口（COM 和 LPT）
```

2. 插拔开发板，观察哪个 COM 口新增（如 COM3、COM5 等）。

---

### 6. 上传第一个程序

1. 在 Arduino IDE 中设置：

```text
Board: ESP32S3 Dev Module
Port: COMx（刚才看到的端口号）
Upload Speed: 460800
USB CDC On Boot: Enabled
```

2. 打开示例代码：

[`示例代码/01_LED闪烁/01_blink_onboard_led.ino`](../示例代码/01_LED闪烁/01_blink_onboard_led.ino)

3. 点击上传按钮（→）。

---

## 完成标志

- [x] Arduino IDE 能看到 `ESP32S3 Dev Module`
- [x] 设备管理器能看到 COM 口
- [x] 上传成功后板载 LED 开始闪烁

---

## 常见错误

| 现象 | 处理 |
|------|------|
| 找不到 COM 口 | 1. 换 USB 数据线（必须能传数据）<br>2. 换 USB 接口<br>3. 安装对应的串口驱动 |
| 上传失败，一直显示 `Connecting...` | 1. 按住 BOOT 按钮<br>2. 点击上传<br>3. 出现 `Connecting...` 后松开 BOOT |
| 上传卡住或报错 | 1. 把 Upload Speed 改为 `115200`<br>2. 重启开发板<br>3. 更换 USB 口 |
| 板卡包下载失败 | 1. 确认使用了 `-cn` 镜像地址<br>2. 检查网络连接<br>3. 尝试使用手机热点 |

> [!TIP]
> 上传失败时，优先尝试降低 Upload Speed 和按住 BOOT 上传。

---

**[← 上一篇：下载与驱动](00_downloads_cn.md)** ·
[返回学习路线](00_learning_path.md) ·
**[下一篇：macOS 安装 →](02_install_macos.md)**
