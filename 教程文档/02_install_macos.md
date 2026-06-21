# 02 macOS 安装教程

> [!TIP]
> 本篇是 [学习路线](00_learning_path.md) 的第 3 步。建议先完成 [下载链接](00_downloads_cn.md) 准备再开始。

## 本节目标

- 在 macOS 上安装 Arduino IDE 2.x
- 添加 ESP32 板卡包国内镜像
- 安装 USB 串口驱动（如需要）
- 成功上传第一个程序到 ESP32-S3

## 需要的硬件

| 物品 | 用途 |
|------|------|
| ESP32-S3 开发板 | 主控 |
| USB 数据线（能传数据） | 连接电脑和开发板 |

## 系统要求

- **macOS 版本**: macOS 10.14 (Mojave) 或更高版本
- **推荐**: macOS 12 (Monterey) 或更高版本
- **磁盘空间**: 至少 2GB 可用空间

---

## 步骤

### 1. 安装 Arduino IDE

1. 下载 Arduino IDE 2.x（见 [00_downloads_cn.md](00_downloads_cn.md)），选择 `.dmg` 文件（约 150-200MB）。

2. 双击下载的 `.dmg` 文件，会显示：
   ```
   [Arduino IDE 图标]  →  [Applications 文件夹图标]
   ```

3. 拖动 Arduino IDE 到 Applications 文件夹。

4. 弹出磁盘映像（在 Finder 侧边栏右键点击映像，选择"推出"）。

5. 打开 Launchpad 或 Applications 文件夹，点击 Arduino IDE。

> [!NOTE]
> 首次打开可能会出现安全提示"无法验证开发者"。解决方法：
> - 打开 **系统设置 → 隐私与安全性**
> - 找到底部提示，点击 **仍要打开（Open Anyway）**
> - 确认 **打开（Open）**

---

### 2. 添加 ESP32 国内板卡包

1. 打开 Arduino IDE，进入 `Arduino IDE → Settings`（或按 `Cmd + ,`）。

2. 在 `Additional Boards Manager URLs` 中，点击右侧的小窗口图标 📋。

3. 新增一行，粘贴：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

4. 点击 OK 确认。

5. 打开 **Tools → Board → Boards Manager**，搜索 `esp32`。

6. 找到 `esp32 by Espressif Systems`，**选择带 `-cn` 后缀的版本安装**。

7. 等待安装完成（通常需要 3-10 分钟）。

8. 验证：打开 **Tools → Board → esp32**，应该能看到 `ESP32S3 Dev Module`。

---

### 3. 安装 USB 串口驱动

1. 用 USB 数据线连接 ESP32 到 Mac。

2. 打开 **Tools → Port**。

3. **情况 1：能看到串口（如 `/dev/cu.usbserial-0001`）** → 无需安装驱动，跳到步骤 4。

4. **情况 2：Port 菜单是灰色或为空** → 需要安装驱动。

#### 安装 CH340/CH341 驱动

1. 下载驱动：https://www.wch.cn/downloads/CH34XSER_MAC_ZIP.html

2. 解压后双击 `CH34xVCPDriver.pkg`。

3. 如果出现安全提示，打开 **系统设置 → 隐私与安全性**，点击 **仍要打开**。

4. 按照安装向导完成安装。

5. **重要：重启 Mac**。

> [!WARNING]
> macOS Ventura/Sonoma 用户：重启后可能需要在"系统设置 → 隐私与安全性"中**允许内核扩展**，然后再次重启。

#### 验证驱动

重新连接 ESP32，打开 **Tools → Port**，应该能看到 `/dev/cu.usbserial-xxx`。

---

### 4. 配置 ESP32-S3 板卡参数

选择：

```text
Board: ESP32S3 Dev Module
Port: /dev/cu.usbserial-xxx
Upload Speed: 460800
USB CDC On Boot: Enabled
CPU Frequency: 240MHz
Flash Mode: QIO 或 DIO
Partition Scheme: Default
```

> [!IMPORTANT]
> **USB CDC On Boot** 必须设为 **Enabled**，否则 `Serial.print()` 无法输出。

---

### 5. 上传第一个程序

1. 打开示例代码：

[`示例代码/01_LED闪烁/01_blink_onboard_led.ino`](../示例代码/01_LED闪烁/01_blink_onboard_led.ino)

2. 点击 ✓（验证）按钮，确保代码无错误。

3. 点击 →（上传）按钮。

4. 观察底部输出，应该显示：
```
Connecting........__
Chip is ESP32-S3
Writing at 0x00010000...
Done uploading.
```

5. 上传成功后，**板载 LED 应该每秒闪烁一次**。

---

## 完成标志

- [x] Arduino IDE 能看到 `ESP32S3 Dev Module`
- [x] Tools → Port 能看到串口设备
- [x] 上传成功后板载 LED 开始闪烁
- [x] 串口监视器（`Cmd + Shift + M`）能正常显示输出

---

## 常见错误

| 现象 | 处理 |
|------|------|
| 找不到串口（Port 为空） | 1. 换 USB 数据线（不能是充电线）<br>2. 重新安装驱动并重启<br>3. 终端运行 `ls /dev/cu.*` 检查设备 |
| 上传卡在 `Connecting...` | 1. 点上传后，看到 `Connecting` 时按住 BOOT 键<br>2. 出现 `Writing at...` 后松开<br>3. 或降低 Upload Speed 到 `115200` |
| 串口输出乱码 | 1. 检查代码中的波特率（如 `Serial.begin(115200)`）<br>2. 串口监视器右下角改为对应波特率<br>3. 按开发板的 EN 键复位 |
| LED 不闪烁 | 1. 确认代码中 `LED_PIN = 2`<br>2. 尝试交换 `LED_ON` 和 `LED_OFF` 的值<br>3. 按 EN 键复位 |
| macOS 安全性阻止驱动 | 系统设置 → 隐私与安全性 → 允许来自"江苏沁恒股份"的软件 → 重启 |

> [!TIP]
> macOS 快捷键：`Cmd + R` 验证，`Cmd + U` 上传，`Cmd + Shift + M` 串口监视器。

---

**[← 上一篇：Windows 安装](01_install_windows.md)** ·
[返回学习路线](00_learning_path.md) ·
**[下一篇：板卡参数设置 →](03_board_settings_esp32s3.md)**
