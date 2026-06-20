# ESP32-S3 Arduino 从零搭建开发环境

这是一套面向零基础学习者的 ESP32-S3 开发板入门教程，适合工程训练项目、机器人项目或者自学。

**系统要求**：Windows 10/11（推荐），macOS 也可以使用。初学者建议先按 Windows 教程完成环境搭建。

**硬件说明**：本教程按已配发的 ESP32-S3 开发板、MG4010-i10 RS485 电机和 PWM 舵机组织内容，重点放在环境搭建、硬件认识、驱动控制和机器人项目整合。

**开发板资料**：本教程使用的 ESP32 开发板资料页：
https://www.xinlucity.com/?s=resourcedetail/index/id/61.html

**第一次学习先看**：[`教程文档/00_learning_path.md`](教程文档/00_learning_path.md)。这份路线按“环境 → 板载 RGB → 串口 → 舵机 → 电机 → 整合”的顺序安排，每一步都有完成标志和失败排查入口。

**教师审阅与维护参考**：[`教程文档/10_review_report.md`](教程文档/10_review_report.md)。里面记录了教程规范性评估、各例程正确性检查和后续改进建议。

**默认接线引脚**：已根据 ESP32-S3 开发板原理图核对。板载 RGB 不需要接线；下面用于外设的 GPIO 都从排针引出，适合本教程直接插线使用。

| 功能 | 默认 GPIO | 接线用途 |
|------|-----------|----------|
| 板载 RGB | GPIO48 | 开发板自带 WS2812 RGB 灯，不需要外接元件 |
| 外接 LED（扩展） | GPIO2 | GPIO2 → 220Ω 电阻 → LED 长脚，LED 短脚 → GND |
| RS485 TX | GPIO17 | 接 RS485 转接板/模块 DI |
| RS485 RX | GPIO18 | 接 RS485 转接板/模块 RO |
| RS485 DE/RE | GPIO16 | 接 RS485 转接板/模块 DE 和 RE，两个脚并在一起 |
| 舵机 1-4 | GPIO10、GPIO11、GPIO12、GPIO13 | 只接舵机信号线，舵机电源使用独立 5V |

---

## 这里有什么

```
esp32s3-arduino-zero-to-one-cn/
├── README.md              # 项目主页（你现在看的）
├── 教程文档/              # 安装教程、板卡设置、使用指南
│   ├── 00-05_*.md        # 安装教程、板卡设置、常见问题、项目自查
│   ├── 00_learning_path.md       # 学生从零开始的学习路线
│   ├── 06_hardware_info.md       # 硬件资料说明
│   ├── 07_servo_guide.md         # 舵机使用指南
│   ├── 08_motor_guide.md         # MG4010 电机使用指南
│   ├── 09_robot_integration.md   # 两电机四舵机整合说明
│   └── 10_review_report.md       # 教程规范性和例程审阅报告
├── 示例代码/              # 示例代码（从易到难）
│   ├── 01_LED闪烁/                # LED 闪烁（最简单）
│   ├── 02_串口调试/               # 串口输出
│   ├── 03_舵机基础控制/           # 舵机串口控制
│   ├── 04_WiFi网页控制LED/        # Wi-Fi 网页控制 LED（学网页交互）
│   ├── 05_舵机网页控制/           # 舵机网页控制（非阻塞）
│   ├── 06_电机基础控制/           # MG4010 厂家 RS485 协议基础
│   ├── 07_电机速度模式/           # MG4010 速度闭环控制
│   ├── 08_电机位置模式/           # MG4010 多圈位置闭环控制
│   ├── 09_电机网页控制/           # MG4010 网页综合控制
│   └── 10_机器人整合控制/         # 两个 RS485 电机 + 四个 PWM 舵机
└── 硬件资料/              # 硬件资料
    ├── 技术文档/          # 原理图、电机手册、协议文档
    ├── 驱动程序/          # CH340 驱动程序
    └── 3D模型/            # 3D 模型（STEP 格式）
        ├── 电机/          # MG4010 电机模型
        └── 舵机/          # MG90S 舵机和舵盘模型
```

---



**重点**：国内装 ESP32 板卡包别用 GitHub 地址，用 Espressif 官方给的国内镜像：

```
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

安装的时候选带 **-cn** 后缀的版本（比如 3.x.x-cn），这是国内镜像版本。后续更新也要手动选 **-cn** 版本，别点自动更新。

---

## 下载链接

详细的看 [`教程文档/00_downloads_cn.md`](教程文档/00_downloads_cn.md)，这里列几个必需的：

**Arduino IDE**  
- 官方下载：https://www.arduino.cc/en/software
- 国内备用：https://arduino.me/download

**ESP32 板卡包地址（国内镜像）**  
```
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

**USB 转串口驱动（WCH 芯片）**  
- CH340/CH341 驱动：https://www.wch.cn/downloads/CH341SER_EXE.html
- CH343/CH342/CH344 驱动：https://www.wch.cn/downloads/CH343SER_EXE.html

---

## Windows 快速上手（5 步搞定）

### 第 1 步：装 Arduino IDE

下载 Arduino IDE 2.x，双击安装。

**注意**：安装路径别用中文，比如：

- 可以：`D:\ArduinoIDE`  
- 不行：`C:\Users\张三\桌面\Arduino`

中文路径有时候会出问题。

---

### 第 2 步：加 ESP32 国内镜像地址

打开 Arduino IDE，找到：

```
文件 → 首选项（或者 File → Preferences）
```

在「其他开发板管理器地址」（Additional Boards Manager URLs）里填：

```
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

点 OK。

---

### 第 3 步：装 ESP32 板卡包

打开：

```
工具 → 开发板 → 开发板管理器（或者 Tools → Board → Boards Manager）
```

搜索：`esp32`

找到：`esp32 by Espressif Systems`

选带 `-cn` 后缀的版本装，比如：`3.x.x-cn`

装完重启 Arduino IDE。

---

### 第 4 步：连接 ESP32-S3 开发板

用 USB 线插上开发板，**注意要用数据线，很多充电线只能充电不能传数据**。

打开 Windows 设备管理器，看看有没有出现 COM 口：

```
端口（COM 和 LPT）
  USB-SERIAL CH340 (COM3)
  或者 USB-Enhanced-SERIAL CH343 (COM4)
  或者 USB JTAG/serial debug unit (COMx)
```

如果没有 COM 口：

1. 换根数据线试试
2. 换个 USB 口
3. 装 WCH 驱动（CH340/CH343）
4. 按住板子上的 BOOT 键再插 USB

---

### 第 5 步：选开发板和端口

在 Arduino IDE 里设置：

```
Tools → Board → esp32 → ESP32S3 Dev Module
Tools → Port → COMx（选你刚才看到的那个）
```

推荐参数（不懂的话照抄就行）：

```
Board: ESP32S3 Dev Module
Upload Speed: 460800
USB CDC On Boot: Enabled
CPU Frequency: 240MHz
Flash Mode: QIO 或 DIO
Flash Size: 看你板子实际大小，一般 8MB 或 16MB
PSRAM: 有的话选 Enabled，没有就 Disabled
Partition Scheme: Default
```

上传失败的话，把 Upload Speed 改成 `115200` 试试。

---

## 第一个程序：让板载 RGB 闪起来

第一步建议先用板载 RGB，不需要面包板、电阻和 LED。这样学生能先确认“IDE、板卡、端口、上传”全部正常，再学习外接 GPIO。

本教程使用的 ESP32-S3 开发板板载 RGB 是 WS2812，控制脚为 GPIO48。它不能用普通 `digitalWrite()` 直接点亮，示例使用 Arduino-ESP32 提供的 `neopixelWrite()`。

打开示例代码：

```
示例代码/01_LED闪烁/01_blink_onboard_rgb.ino
```

点上传。如果板载 RGB 每秒闪一次，说明环境搭好了。想练习普通 GPIO 输出时，再把外接 LED 接到 GPIO2：`GPIO2 → 220Ω 电阻 → LED 长脚，LED 短脚 → GND`。

---

## 例程列表

所有例程都在 `示例代码/` 目录下，建议按顺序学：

1. **01_LED闪烁** - 板载 RGB 闪烁  
   不需要接线，先验证环境、板卡和上传流程

2. **02_串口调试** - 串口输出  
   学会用 Serial.println 调试

3. **03_舵机基础控制** - 舵机基础控制  
   串口输入角度，舵机平滑转动

4. **04_WiFi网页控制LED** - Wi-Fi 网页控制板载 RGB  
   手机连 ESP32 热点，网页控制板载 RGB 开关，学习网页交互基础

5. **05_舵机网页控制** - 舵机网页控制  
   结合舵机和网页控制，学习非阻塞编程

6. **06_电机基础控制** - MG4010 电机基础  
   使用厂家 RS485 私有协议，学习运行、停止、清错、状态读取和角度读取

7. **07_电机速度模式** - MG4010 闭环速度  
   设定输出轴目标速度，代码按 i10 减速比换算为协议数据

8. **08_电机位置模式** - MG4010 闭环位置  
   设定输出轴目标角度，使用厂家多圈位置闭环命令控制

9. **09_电机网页控制** - MG4010 网页综合控制  
   网页切换速度/位置模式，显示速度、角度和温度

10. **10_机器人整合控制** - 最终工程整合基础
    同时控制两个 MG4010-i10 RS485 电机和四个 PWM 舵机，作为机器人项目主程序模板

**新手建议**：先把 01-05 跑通，再学习电机例程 06-09，最后阅读 10 的整合结构。电机第一次测试时先用低速、小角度，不要直接高速运行。

---

## 硬件资料

`硬件资料/` 目录下有开发板和电机的技术资料：

- [技术文档](硬件资料/技术文档) - ESP32-S3 原理图、MG4010 电机手册和协议文档
- [驱动程序](硬件资料/驱动程序) - CH340/CH341 USB 转串口驱动（Windows）
- [3D模型](硬件资料/3D模型) - MG4010 电机和 MG90S 舵机的 3D 模型（STEP 格式，设计机械结构用）
- [转接板原理图](转接板原理图.pdf) - 课程用 RS485 通信与电机供电转接板原理图

常用资源直达：

| 资源 | 链接 |
|------|------|
| ESP32 开发板资料页 | [资料页](https://www.xinlucity.com/?s=resourcedetail/index/id/61.html) |
| ESP32-S3 原理图 | [ESP32-S3原理图.pdf](硬件资料/技术文档/ESP32-S3原理图.pdf) |
| MG4010-i10 电机手册 | [MG4010-i10电机手册.pdf](硬件资料/技术文档/MG4010-i10电机手册.pdf) |
| MG4010-i10 RS485 协议 | [MG4010-i10-R485协议.pdf](硬件资料/技术文档/MG4010-i10-R485协议.pdf) |
| RS485/供电转接板原理图 | [转接板原理图.pdf](转接板原理图.pdf) |
| CH340/CH341 Windows 驱动 | [CH341SER.EXE](硬件资料/驱动程序/CH341SER.EXE) |
| MG4010-i10 电机 3D 模型 | [MG4010-i10电机.STEP](硬件资料/3D模型/电机/MG4010-i10电机.STEP) |

详细说明看 [`硬件资料/README.md`](硬件资料/README.md)

---

## 项目制课程建议安排

本教程不是一次性实验，而是面向机器人制作的项目制训练。建议按“先能用、再理解、最后整合”的顺序推进：

1. **开发板认识与环境验证**
   你需要知道 USB 口、EN/BOOT 键、GPIO、3.3V、5V、GND 的含义，完成 Arduino IDE、ESP32-S3 板卡包、串口驱动和板载 RGB 闪烁验证。

2. **基础调试能力训练**
   通过串口输出、串口输入和简单网页控制，掌握 `Serial.println()`、串口监视器、Wi-Fi AP 和浏览器访问 ESP32 的基本方法。

3. **PWM 舵机模块学习**
   从单个舵机开始，理解 50Hz PWM、脉宽与角度的关系、外部 5V 供电和共地要求，再扩展到多个舵机的非阻塞控制。

4. **RS485 电机模块学习**
   先完成单个 MG4010-i10 电机的运行、停止、清错、状态读取，再学习速度模式、位置模式和网页控制。推荐使用课程配套 RS485/供电转接板减少接线错误；重点理解厂家私有协议、ID、校验和、减速比换算和安全上电顺序。

5. **机器人底盘与机构整合**
   将两个 RS485 电机作为左右轮或双执行机构，将四个 PWM 舵机作为机器人机构关节，统一放入一个主循环中调度。最终工程参考 `示例代码/10_机器人整合控制/` 和 `教程文档/09_robot_integration.md`。

6. **项目迭代与自查**
   在能稳定控制硬件后，再加入传感器、结构件、运动逻辑和任务流程。检查项目时不要只看“能动”，还要看接线安全、程序结构、调试记录和异常处理。

---

## 遇到问题怎么办

### 找不到端口

先检查这几个：

- USB 线是不是数据线（充电线不行）
- 驱动装了没
- 开发板灯亮了吗
- 设备管理器有没有出现未知设备

### 上传卡在 Connecting...

试试这个：

```
按住 BOOT 键 → 点上传 → 看到 Connecting... → 松开 BOOT
```

还不行就：

- 把 Upload Speed 改成 115200
- 换根 USB 线
- 关掉串口监视器
- 检查是不是选错 COM 口了

### 装 ESP32 板卡包失败

确认两件事：

1. 用的是国内镜像地址（`package_esp32_index_cn.json`）
2. 装的是带 `-cn` 后缀的版本

别在国内直接用 GitHub 的地址。

### 编译路径报错

别把代码放在：

- 有中文的路径
- 有空格的路径
- OneDrive 同步目录

建议放：`D:\ArduinoProjects\esp32s3_class`

---

## 最终工程能力说明

ESP32-S3 具备足够资源完成本教程的最终工程：

- 两个 MG4010-i10 RS485 电机共用一条 UART + RS485 总线，通过不同电机 ID 区分，命令按顺序快速发送。
- 四个 PWM 舵机使用 ESP32-S3 的 LEDC PWM 输出，每个舵机占用一个 GPIO，不占用 UART。
- Wi-Fi 网页控制、串口调试、两电机控制和四舵机 PWM 可以在同一工程中共存，但主循环必须采用非阻塞写法，避免长时间 `delay()`。
- 电机和舵机必须分别使用外部电源，所有电源负极必须与 ESP32 GND 共地。

---

## 参考资料

- Arduino IDE 官方下载：https://www.arduino.cc/en/software
- Arduino IDE 安装说明：https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE
- Espressif Arduino-ESP32 安装文档：https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
- WCH CH340/CH341 驱动：https://www.wch.cn/downloads/CH341SER_EXE.html
- WCH CH343 驱动：https://www.wch.cn/downloads/CH343SER_EXE.html
