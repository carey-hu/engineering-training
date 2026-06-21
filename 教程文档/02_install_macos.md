# 02 macOS 安装教程

> 本教程帮助 macOS 用户从零开始安装 Arduino IDE、配置 ESP32 板卡包、安装驱动，并完成第一次程序上传。

---

## 系统要求

- **macOS 版本**: macOS 10.14 (Mojave) 或更高版本
- **推荐**: macOS 12 (Monterey) 或更高版本
- **磁盘空间**: 至少 2GB 可用空间
- **网络**: 需要联网下载板卡包

---

## 1. 安装 Arduino IDE

### 1.1 下载 Arduino IDE

**官方下载页：**
```
https://www.arduino.cc/en/software
```

**国内镜像（备用）：**
```
https://arduino.me/download
```

**选择版本：**
- 下载 **Arduino IDE 2.x** 版本（推荐）
- 文件格式：`.dmg`（约 150-200MB）

### 1.2 安装步骤

1. **双击下载的 `.dmg` 文件**
   
   会打开一个安装窗口，显示：
   ```
   [Arduino IDE 图标]  →  [Applications 文件夹图标]
   ```

2. **拖动 Arduino IDE 到 Applications 文件夹**
   
   按住 Arduino IDE 图标，拖到右侧的 Applications 文件夹上。

3. **等待复制完成**
   
   系统会自动复制文件到 `/Applications/Arduino IDE.app`

4. **弹出磁盘映像**
   
   在 Finder 侧边栏中，右键点击 Arduino IDE 磁盘映像，选择"推出"。

### 1.3 首次打开

1. **打开 Launchpad** 或 **Applications 文件夹**

2. **找到并点击 Arduino IDE**

3. **可能会出现安全提示：**

   ```
   "Arduino IDE" 无法打开，因为无法验证开发者。
   ```

   **解决方法：**
   
   - 打开 **系统设置（System Settings / System Preferences）**
   - 进入 **隐私与安全性（Privacy & Security）**
   - 找到底部提示：`"Arduino IDE" 已被阻止使用，因为它来自身份不明的开发者`
   - 点击 **仍要打开（Open Anyway）**
   - 确认 **打开（Open）**

4. **Arduino IDE 启动成功**

   首次启动可能需要几秒钟，会自动安装内置组件。

---

## 2. 添加 ESP32 国内板卡包

### 2.1 打开首选项

**方式一（推荐）：**
```
Arduino IDE → Settings (或 Preferences)
```

快捷键：`Cmd + ,`（逗号）

**方式二：**
```
菜单栏 → File → Preferences
```

### 2.2 添加板卡管理器 URL

1. 在 Preferences 窗口中，找到：
   ```
   Additional Boards Manager URLs
   ```

2. 点击右侧的 **小窗口图标** 📋

3. 在弹出的窗口中，**新增一行**，粘贴：

   ```
   https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
   ```

4. 如果已经有其他 URL，**不要删除**，保留原有内容，新 URL 另起一行。

5. 点击 **OK** 确认。

6. 再次点击 **OK** 关闭 Preferences 窗口。

### 2.3 安装 ESP32 板卡包

1. **打开 Boards Manager（开发板管理器）**

   方式一：点击左侧工具栏的 **芯片图标** 🔧
   
   方式二：菜单栏 → Tools → Board → Boards Manager

2. **搜索 ESP32**

   在搜索框中输入：`esp32`

3. **找到正确的板卡包**

   找到名为：
   ```
   esp32 by Espressif Systems
   ```

   **重要：** 选择带 **`-cn`** 后缀的版本，例如：
   ```
   3.0.7-cn
   2.0.17-cn
   ```

4. **点击 INSTALL（安装）**

   下载时间取决于网速，通常需要 3-10 分钟。

5. **等待安装完成**

   底部状态栏会显示进度：
   ```
   Downloading esp32-tools...
   Installing esp32-tools...
   ```

6. **安装完成后关闭 Boards Manager**

### 2.4 验证安装

1. 打开 **Tools → Board → esp32**

2. 应该能看到很多 ESP32 开发板型号，例如：
   ```
   ESP32 Dev Module
   ESP32S3 Dev Module
   ESP32-C3 Dev Module
   ...
   ```

3. 选择 **ESP32S3 Dev Module**

---

## 3. 安装 USB 串口驱动

### 3.1 判断是否需要驱动

**步骤：**

1. 用 **USB 数据线**（不是充电线）连接 ESP32 开发板到 Mac

2. 打开 **Tools → Port**

3. **情况 1：能看到串口设备（不需要驱动）**

   ```
   /dev/cu.usbserial-0001
   /dev/cu.usbmodem12345
   /dev/cu.wchusbserial1234
   ```

   **说明：** 系统已自动识别，无需安装驱动。

4. **情况 2：Port 菜单是灰色或为空（需要驱动）**

   说明系统未识别串口芯片，需要安装驱动。

### 3.2 确认串口芯片型号

**方法一：查看开发板标注**

在开发板上找到串口芯片，通常标注为：
- `CH340G` 或 `CH340C`
- `CH343P` 或 `CH342F`
- `CP2102` 或 `CP2104`

**方法二：查看系统报告**

1. 点击左上角 **Apple 图标 **
2. 选择 **关于本机（About This Mac）**
3. 点击 **更多信息（More Info）** 或 **系统报告（System Report）**
4. 左侧找到 **USB**
5. 查看连接的设备信息

### 3.3 安装 CH340/CH341 驱动

**适用芯片：** CH340G、CH340C、CH341

**官方下载：**
```
https://www.wch.cn/downloads/CH34XSER_MAC_ZIP.html
```

**安装步骤：**

1. 下载 `CH34xSER_MAC.ZIP` 并解压

2. 双击 `CH34xVCPDriver.pkg` 安装包

3. 点击 **继续（Continue）**

4. **可能会出现安全提示：**

   ```
   "CH34xVCPDriver.pkg" 无法打开，因为它来自身份不明的开发者。
   ```

   **解决方法：**
   
   - 打开 **系统设置 → 隐私与安全性**
   - 找到底部提示：`"CH34xVCPDriver.pkg" 已被阻止...`
   - 点击 **仍要打开（Open Anyway）**
   - 输入密码确认

5. 按照安装向导完成安装

6. **重要：重启 Mac**

   安装驱动后必须重启，否则不生效。

### 3.4 安装 CH343/CH342 驱动

**适用芯片：** CH343P、CH342F、CH344

**官方下载：**
```
https://www.wch.cn/downloads/CH343SER_MAC_ZIP.html
```

**安装步骤同 CH340 驱动。**

### 3.5 macOS 权限设置（Ventura 13.0+ / Sonoma 14.0+）

**如果重启后仍无法识别串口，可能需要允许内核扩展：**

1. 打开 **系统设置（System Settings）**

2. 进入 **隐私与安全性（Privacy & Security）**

3. 向下滚动到 **安全性（Security）** 区域

4. 找到类似提示：
   ```
   系统软件来自"江苏沁恒股份有限公司"已被阻止加载
   ```

5. 点击 **允许（Allow）**

6. 输入密码确认

7. **再次重启 Mac**

### 3.6 验证驱动安装

1. **重新连接 ESP32 开发板**

2. **打开 Arduino IDE → Tools → Port**

3. **应该能看到串口：**

   ```
   /dev/cu.usbserial-0001
   /dev/cu.wchusbserial1234
   ```

4. **选择该串口**

---

## 4. 配置 ESP32-S3 板卡参数

### 4.1 选择开发板型号

```
Tools → Board → esp32 → ESP32S3 Dev Module
```

### 4.2 推荐参数设置

| 选项 | 推荐值 | 说明 |
|------|--------|------|
| **Port** | `/dev/cu.usbserial-xxx` | 选择实际串口 |
| **Upload Speed** | `460800` | 上传失败时改为 `115200` |
| **CPU Frequency** | `240MHz` | 最高性能 |
| **Flash Mode** | `QIO` 或 `DIO` | 默认即可 |
| **Flash Size** | `8MB` 或 `16MB` | 按实际板卡选择 |
| **Partition Scheme** | `Default` | 默认分区 |
| **PSRAM** | `OPI PSRAM` 或 `Enabled` | N16R8 型号需启用 |
| **USB CDC On Boot** | `Enabled` | **重要：必须启用** |

**关键设置说明：**

- **USB CDC On Boot: Enabled** - 让 `Serial.print()` 通过 USB 输出，必须启用
- **Upload Speed: 460800** - 默认上传速度，失败时改为 115200
- **PSRAM** - 如果开发板带 PSRAM（如 N16R8），需启用对应选项

---

## 5. 上传第一个程序

### 5.1 打开示例代码

1. **在 Finder 中打开示例代码**

   ```
   工程目录/示例代码/01_LED闪烁/01_blink_onboard_led.ino
   ```

2. **双击 `.ino` 文件**，Arduino IDE 会自动打开

### 5.2 验证代码

点击左上角的 **✓（验证/编译）** 按钮，确保代码无错误。

**底部输出窗口应该显示：**
```
Sketch uses 267876 bytes (20%) of program storage space.
Global variables use 19856 bytes (6%) of dynamic memory.
Done compiling
```

### 5.3 上传代码

1. **确认板卡和串口已选择**

   ```
   Board: ESP32S3 Dev Module
   Port: /dev/cu.usbserial-0001
   ```

2. **点击 →（上传）按钮**

3. **观察底部输出窗口**

   ```
   Connecting........__
   Chip is ESP32-S3 (revision v0.1)
   Features: WiFi, BLE
   Writing at 0x00010000... (10%)
   Writing at 0x00020000... (20%)
   ...
   Wrote 267876 bytes at 0x00010000 in 6.2 seconds
   Leaving...
   Hard resetting via RTS pin...
   ```

4. **上传成功标志：**

   ```
   Done uploading.
   ```

### 5.4 观察结果

上传成功后，**板载 LED 应该每秒闪烁一次**。

---

## 6. 常见问题排查

### 6.1 找不到串口（Port 为空）

**可能原因：**

1. **USB 线是充电线，不是数据线**
   - 解决：换一根 USB 数据线

2. **驱动未安装或未生效**
   - 解决：重新安装驱动并重启 Mac

3. **USB-C 转接器有问题**
   - 解决：换一个转接器或直接插 USB-A 口

4. **开发板未通电**
   - 解决：检查开发板电源指示灯是否亮

**调试步骤：**

1. 打开 **终端（Terminal）**

2. 输入命令：

   ```bash
   ls /dev/cu.*
   ```

3. **应该能看到串口设备：**

   ```
   /dev/cu.Bluetooth-Incoming-Port
   /dev/cu.usbserial-0001          ← ESP32 串口
   ```

4. **如果看不到 `usbserial` 或 `usbmodem`：**
   - 重新安装驱动
   - 检查系统设置 → 隐私与安全性 → 允许内核扩展

### 6.2 上传卡在 "Connecting........"

**现象：**
```
Connecting........_____....._____....._____.....
```

**解决方法 1：按住 BOOT 键上传**

1. 点击 Arduino IDE 的 **上传** 按钮
2. 当底部出现 `Connecting` 时，**按住开发板上的 BOOT 键**
3. 看到 `Writing at...` 开始上传后，**松开 BOOT 键**
4. 等待上传完成

**解决方法 2：降低上传速度**

```
Tools → Upload Speed → 115200
```

然后重新上传。

**解决方法 3：检查 USB CDC 设置**

确认 `Tools → USB CDC On Boot → Enabled`

### 6.3 上传成功但串口乱码

**现象：**

打开串口监视器（Tools → Serial Monitor），显示：
```
������ÿ����������ÿ��
```

**原因：** 波特率不匹配

**解决方法：**

1. 检查代码中的波特率：

   ```cpp
   Serial.begin(115200);  // 代码设置为 115200
   ```

2. 检查串口监视器右下角的波特率设置，改为 **115200**

3. 按一下开发板的 **EN（复位）** 键，重启程序

### 6.4 上传后 LED 不闪烁

**检查项：**

1. **确认 LED 引脚**

   代码中应该是：
   ```cpp
   const int LED_PIN = 2;  // GPIO2 是板载 LED
   ```

2. **LED 可能是反向点亮**

   尝试修改代码：
   ```cpp
   const int LED_ON = LOW;   // 改为 LOW
   const int LED_OFF = HIGH; // 改为 HIGH
   ```

3. **按 EN 键复位**

   上传后按一下 EN 键，让程序重新运行。

### 6.5 macOS Ventura/Sonoma 权限问题

**现象：** 安装驱动后仍无法识别串口

**解决步骤：**

1. **完全卸载旧驱动**

   打开终端，输入：
   ```bash
   sudo rm -rf /Library/Extensions/usbserial.kext
   sudo rm -rf /System/Library/Extensions/usb.kext
   ```

2. **重启 Mac**

3. **重新安装驱动**

4. **允许内核扩展**（见 3.5 节）

5. **再次重启 Mac**

---

## 7. 串口监视器使用

### 7.1 打开串口监视器

**方式一：** 点击右上角 **🔍（放大镜）** 图标

**方式二：** 菜单栏 → Tools → Serial Monitor

**快捷键：** `Cmd + Shift + M`

### 7.2 配置串口监视器

右下角设置：

- **波特率（Baud Rate）**：选择 `115200`（与代码一致）
- **换行符（Line Ending）**：选择 `Newline` 或 `Both NL & CR`

### 7.3 查看串口输出

上传示例代码 `02_串口调试/02_serial_print.ino`，串口监视器应该显示：

```
millis: 1000
millis: 2000
millis: 3000
...
```

---

## 8. 进阶提示

### 8.1 使用快捷键

| 快捷键 | 功能 |
|--------|------|
| `Cmd + R` | 验证（编译） |
| `Cmd + U` | 上传 |
| `Cmd + Shift + M` | 打开串口监视器 |
| `Cmd + T` | 自动格式化代码 |
| `Cmd + F` | 查找 |
| `Cmd + ,` | 打开设置 |

### 8.2 安装其他开发板

除了 ESP32，还可以安装其他开发板：

- **STM32**: https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
- **RP2040**: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

### 8.3 保留旧版本板卡包

如果某些库不兼容新版本，可以保留旧版本：

1. Boards Manager → 找到已安装的 ESP32
2. 点击版本号下拉菜单
3. 选择其他版本安装（旧版本不会自动删除）

---

## 9. 总结

完成本教程后，你应该已经：

✅ 安装了 Arduino IDE 2.x  
✅ 添加了 ESP32 国内板卡包  
✅ 安装了 USB 串口驱动（如需要）  
✅ 配置了 ESP32-S3 板卡参数  
✅ 成功上传了第一个程序  
✅ 学会了使用串口监视器  

---

## 10. 下一步学习

- **[03 板卡参数设置](03_board_settings_esp32s3.md)** - 深入理解 ESP32-S3 各项参数
- **[00 学习路线](00_learning_path.md)** - 开始系统学习 ESP32 控制舵机和电机
- **[FAQ 常见问题](FAQ_troubleshooting.md)** - 遇到问题时查阅

---

## 11. 参考资料

- **Arduino IDE 官方文档**: https://docs.arduino.cc/software/ide-v2
- **ESP32 Arduino 官方文档**: https://docs.espressif.com/projects/arduino-esp32/
- **WCH 驱动下载页**: https://www.wch.cn/downloads/
- **本教程开发板资料页**: https://www.xinlucity.com/?s=resourcedetail/index/id/61.html

---

**编写日期**：2026-06-21  
**适用系统**：macOS 10.14+ (Mojave, Big Sur, Monterey, Ventura, Sonoma)  
**测试环境**：macOS 14.0 (Sonoma) + Arduino IDE 2.3.2
