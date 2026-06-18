# ESP32-S3 Arduino 从零搭建开发环境

这是一套给零基础学生准备的 ESP32-S3 开发板入门教程，适合工程训练课、机器人课或者自学。

**系统要求**：Windows 10/11（推荐），macOS 也能用但课堂统一教学建议用 Windows。

**开发板选择**：ESP32-S3 开发板，尽量买 **原生 USB** 或者 **CH340/CH343 USB 转串口** 的板子，驱动好装。

---

## 这里有什么

```
esp32s3-arduino-zero-to-one-cn/
├── README.md              # 项目主页（你现在看的）
├── 教程文档/              # 安装教程、板卡设置、使用指南
│   ├── 00-05_*.md        # 安装教程、板卡设置、常见问题
│   ├── 06_hardware_info.md       # 硬件资料说明
│   ├── 07_servo_guide.md         # 舵机使用指南
│   └── 08_motor_guide.md         # MG4010 电机使用指南
├── 示例代码/              # 示例代码（从易到难）
│   ├── 01_LED闪烁/                # LED 闪烁（最简单）
│   ├── 02_串口调试/               # 串口输出
│   ├── 03_舵机基础控制/           # 舵机串口控制
│   ├── 04_WiFi网页控制LED/        # Wi-Fi 网页控制 LED（学网页交互）
│   ├── 05_舵机网页控制/           # 舵机网页控制（非阻塞）
│   ├── 06_电机基础控制/           # MG4010 厂家 RS485 协议基础
│   ├── 07_电机速度模式/           # MG4010 速度闭环控制
│   ├── 08_电机位置模式/           # MG4010 多圈位置闭环控制
│   └── 09_电机网页控制/           # MG4010 网页综合控制
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

## 第一个程序：让 LED 闪起来

课堂建议用外接 LED，因为不同 ESP32-S3 板子的板载 LED 引脚不一样，容易搞混。

**接线：**

```
ESP32-S3 的 GPIO2 → 220Ω电阻 → LED 长脚
LED 短脚 → GND
```

打开示例代码：

```
示例代码/01_LED闪烁/01_blink_external_led.ino
```

点上传。如果 LED 每秒闪一次，说明环境搭好了。

---

## 例程列表

所有例程都在 `示例代码/` 目录下，建议按顺序学：

1. **01_LED闪烁** - 外接 LED 闪烁  
   最简单的例程，验证环境搭好了没

2. **02_串口调试** - 串口输出  
   学会用 Serial.println 调试

3. **03_舵机基础控制** - 舵机基础控制  
   串口输入角度，舵机平滑转动

4. **04_WiFi网页控制LED** - Wi-Fi 网页控制 LED  
   手机连 ESP32 热点，网页控制 LED 开关，学习网页交互基础

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

**新手建议**：先把 01-05 跑通，电机例程（06-09）需要 MG4010-i10 RS485 电机、RS485 转换模块和独立电源。电机第一次测试时先用低速、小角度，不要直接高速运行。

---

## 硬件资料

`硬件资料/` 目录下有开发板和电机的技术资料：

- **技术文档/** - ESP32-S3 原理图、MG4010 电机手册和协议文档
- **驱动程序/** - CH340/CH341 USB 转串口驱动（Windows）
- **3D模型/** - MG4010 电机和 MG90S 舵机的 3D 模型（STEP 格式，设计机械结构用）

详细说明看 [`硬件资料/README.md`](硬件资料/README.md)

---

## 课堂怎么安排（2 学时参考）

- **认识开发板**（10 分钟）：USB 口在哪、EN 和 BOOT 键是干嘛的、GPIO/3.3V/GND 怎么接
- **装 IDE**（15 分钟）：Arduino IDE、加国内镜像地址
- **认串口**（10 分钟）：设备管理器看 COM 口、装驱动
- **第一个程序**（20 分钟）：外接 LED 闪烁
- **串口输出**（15 分钟）：Serial.begin、Serial.println
- **舵机或网页控制**（30 分钟）：二选一，看课程重点
- **排错时间**（20 分钟）：处理各种找不到端口、上传失败的问题

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

## 买什么板子比较好

优先选：

- ESP32-S3 + 原生 USB
- ESP32-S3 + CH340/CH343

别买引脚图不清楚、资料乱七八糟、驱动难装的杂牌板。

**课堂采购建议**：全班统一型号、统一引脚图、统一 USB 芯片，这样出问题好排查。

---

## 参考资料

- Arduino IDE 官方下载：https://www.arduino.cc/en/software
- Arduino IDE 安装说明：https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE
- Espressif Arduino-ESP32 安装文档：https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
- WCH CH340/CH341 驱动：https://www.wch.cn/downloads/CH341SER_EXE.html
- WCH CH343 驱动：https://www.wch.cn/downloads/CH343SER_EXE.html
