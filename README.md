# ESP32-S3 Arduino 工程训练教程

面向零基础学习者的 ESP32-S3 Arduino 入门与机器人控制训练项目。内容覆盖环境搭建、板载 LED、串口调试、PWM 舵机、MG4010-i10 RS485 电机、网页控制，以及“两电机四舵机”的整合工程。

> 第一次学习不要从代码目录乱翻，先按 [00 学习路线](教程文档/00_learning_path.md) 走。

## 学生入口

| 你要做什么 | 从这里进入 |
|------------|------------|
| **第一次学习** | **[00 学习路线](教程文档/00_learning_path.md)** ⭐ |
| 下载 Arduino IDE 和驱动 | [00 下载链接](教程文档/00_downloads_cn.md) |
| Windows 环境搭建 | [01 Windows 安装教程](教程文档/01_install_windows.md) |
| macOS 环境搭建 | [02 macOS 安装教程](教程文档/02_install_macos.md) |
| ESP32-S3 板卡参数 | [03 板卡设置](教程文档/03_board_settings_esp32s3.md) |
| WiFi 和网页控制入门 | [04 WiFi AP 模式指南](教程文档/04_wifi_ap_guide.md) |
| 网页控制舵机进阶 | [05 网页控制舵机](教程文档/05_web_servo_guide.md) |
| 硬件资料说明 | [06 硬件资料说明](教程文档/06_hardware_info.md) |
| 舵机使用 | [07 舵机指南](教程文档/07_servo_guide.md) |
| 电机使用 | [08 电机指南](教程文档/08_motor_guide.md) |
| 机器人整合 | [09 机器人整合](教程文档/09_robot_integration.md) |
| **遇到问题看这里** | **[FAQ 常见问题](教程文档/FAQ_troubleshooting.md)** 🔧 |
| 项目交付模板 | [报告模板](示例代码/交付模板/项目报告模板.md) / [异常记录模板](示例代码/交付模板/异常记录模板.md) |

## 开发板说明

本教程使用 ESP32-S3 开发板作为主控制器，按 ESP32-S3-DevKitC-1 类开发板的使用方式组织。实际配发板可能在 USB 接口、丝印或外设数量上略有差异，接线时以板上丝印、[ESP32-S3开发板引脚图](硬件资料/接线图/ESP32-S3开发板引脚图.png) 和 [ESP32-S3原理图](硬件资料/技术文档/ESP32-S3原理图.pdf) 为准。

[![ESP32-S3开发板引脚图](硬件资料/接线图/ESP32-S3开发板引脚图.png)](硬件资料/接线图/ESP32-S3开发板引脚图.png)

参考 Espressif 官方 [ESP32-S3-DevKitC-1 v1.1 用户指南](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)，这类开发板的核心特点如下：

| 模块 | 说明 |
|------|------|
| 主控模组 | 搭载 ESP32-S3-WROOM-1、ESP32-S3-WROOM-1U 或 ESP32-S3-WROOM-2 系列 Wi-Fi + Bluetooth LE 模组 |
| 主芯片能力 | ESP32-S3 面向 AIoT 场景，具备丰富外设接口、神经网络运算能力和信号处理能力 |
| 排针 | 大部分可用 GPIO 引出到两侧排针，适合杜邦线、面包板和课程外设接线 |
| 供电 | 常用 USB 供电；也可通过 5V/GND 或 3V3/GND 排针供电，初学阶段优先使用 USB |
| 电源转换 | 板上 5V 转 3.3V LDO 为 ESP32-S3 模组供电 |
| 下载与调试 | USB-to-UART 接口可供电、烧录固件和串口通信；ESP32-S3 原生 USB 也可用于 USB 通信或 JTAG 调试 |
| 按键与指示灯 | 常见配置包括 BOOT、RESET、3.3V 电源指示灯，部分板卡带 GPIO38 驱动的 RGB LED |

主芯片和开发板资料：

| 资料 | 用途 |
|------|------|
| [ESP32-S3技术规格书.pdf](硬件资料/技术文档/ESP32-S3技术规格书.pdf) | 查询芯片外设、IO 复用、电气参数、工作条件 |
| [ESP32-S3原理图.pdf](硬件资料/技术文档/ESP32-S3原理图.pdf) | 核对开发板 USB、供电、按键、LED、排针连接 |
| [ESP32-S3开发板引脚图.png](硬件资料/接线图/ESP32-S3开发板引脚图.png) | 上机接线前快速核对 GPIO、5V、3.3V、GND 位置 |
| [Espressif 官方用户指南](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html) | 查看 ESP32-S3-DevKitC-1 的组件介绍、供电方式、排针说明和相关官方文档 |

注意：官方指南提到，部分 ESP32-S3-WROOM 模组会占用 GPIO35、GPIO36、GPIO37 作为内部 SPI flash/PSRAM 通信，外部不可使用。本教程默认使用的 GPIO2、GPIO10、GPIO11、GPIO12、GPIO13、GPIO16、GPIO17、GPIO18 已按课程资料核对。

## 接线图

[![电机转接板接线标注](硬件资料/接线图/电机转接板接线标注.png)](硬件资料/接线图/电机转接板接线标注.png)

常用硬件图：

| 图片 | 用途 |
|------|------|
| [电机转接板接线标注.png](硬件资料/接线图/电机转接板接线标注.png) | 上机接线时核对 24V、TTL-485、舵机、电机接口 |
| [电机转接板实物图.png](硬件资料/接线图/电机转接板实物图.png) | 核对实物接口位置、丝印和防呆方向 |
| [ESP32-S3开发板引脚图.png](硬件资料/接线图/ESP32-S3开发板引脚图.png) | 核对开发板 GPIO、供电和接口位置 |
| [电机转接板接线标注.svg](硬件资料/接线图/电机转接板接线标注.svg) | 可继续编辑的接线标注源文件 |

## 学习路线

| 阶段 | 目标 | 教程 | 示例代码 |
|------|------|------|----------|
| 1 | 装好 Arduino IDE 和 ESP32-S3 板卡 | [Windows](教程文档/01_install_windows.md) / [macOS](教程文档/02_install_macos.md) | - |
| 2 | 让板载 LED 闪烁，验证上传流程 | [学习路线第 2 节](教程文档/00_learning_path.md) | [01_LED闪烁](示例代码/01_LED闪烁/01_blink_onboard_led.ino) |
| 3 | 学会串口输出和调试 | [学习路线第 3 节](教程文档/00_learning_path.md) | [02_串口调试](示例代码/02_串口调试/02_serial_print.ino) |
| 4 | 控制单个 PWM 舵机 | [舵机指南](教程文档/07_servo_guide.md) | [03_舵机基础控制](示例代码/03_舵机基础控制/03_servo_basic.ino) |
| 5 | 用网页控制 LED | [学习路线](教程文档/00_learning_path.md) | [04_WiFi网页控制LED](示例代码/04_WiFi网页控制LED/04_wifi_web_led.ino) |
| 6 | 用网页控制舵机 | [舵机指南](教程文档/07_servo_guide.md) | [05_舵机网页控制](示例代码/05_舵机网页控制/05_servo_web_control.ino) |
| 7 | 跑通 MG4010-i10 基础命令 | [电机指南](教程文档/08_motor_guide.md) | [06_电机基础控制](示例代码/06_电机基础控制/06_mg4010_motor_basic.ino) |
| 8 | 电机速度闭环控制 | [电机指南](教程文档/08_motor_guide.md) | [07_电机速度模式](示例代码/07_电机速度模式/07_motor_speed_mode.ino) |
| 9 | 电机多圈位置控制 | [电机指南](教程文档/08_motor_guide.md) | [08_电机位置模式](示例代码/08_电机位置模式/08_motor_position_mode.ino) |
| 10 | 网页综合控制电机 | [电机指南](教程文档/08_motor_guide.md) | [09_电机网页控制](示例代码/09_电机网页控制/09_motor_web_control.ino) |
| 11 | 两电机四舵机整合 | [机器人整合](教程文档/09_robot_integration.md) | [10_机器人整合控制](示例代码/10_机器人整合控制/10_robot_integrated_control.ino) |

## 示例代码索引

| 编号 | 示例 | 说明 |
|------|------|------|
| 01 | [LED 闪烁](示例代码/01_LED闪烁/01_blink_onboard_led.ino) | 不接线，先验证 IDE、板卡、端口和上传 |
| 02 | [串口调试](示例代码/02_串口调试/02_serial_print.ino) | 学会用 `Serial.println()` 看程序状态 |
| 03 | [舵机基础控制](示例代码/03_舵机基础控制/03_servo_basic.ino) | 串口输入角度，控制单个舵机 |
| 04 | [WiFi 网页控制 LED](示例代码/04_WiFi网页控制LED/04_wifi_web_led.ino) | 手机连接 ESP32 热点，用网页控制 LED |
| 05 | [舵机网页控制](示例代码/05_舵机网页控制/05_servo_web_control.ino) | 网页按钮控制舵机角度 |
| 06 | [电机基础控制](示例代码/06_电机基础控制/06_mg4010_motor_basic.ino) | MG4010 运行、停止、清错、读状态 |
| 07 | [电机速度模式](示例代码/07_电机速度模式/07_motor_speed_mode.ino) | 输入目标速度，低速正反转测试 |
| 08 | [电机位置模式](示例代码/08_电机位置模式/08_motor_position_mode.ino) | 输入目标角度，多圈位置闭环 |
| 09 | [电机网页控制](示例代码/09_电机网页控制/09_motor_web_control.ino) | 网页切换速度/位置模式 |
| 10 | [机器人整合控制](示例代码/10_机器人整合控制/10_robot_integrated_control.ino) | 两个 RS485 电机 + 四个 PWM 舵机 |

部分高级示例带有单独说明：

- [06_电机基础控制 README](示例代码/06_电机基础控制/README.md)
- [07_电机速度模式 README](示例代码/07_电机速度模式/README.md)
- [08_电机位置模式 README](示例代码/08_电机位置模式/README.md)
- [09_电机网页控制 README](示例代码/09_电机网页控制/README.md)
- [10_机器人整合控制 README](示例代码/10_机器人整合控制/README.md)

## 硬件资源索引

| 类别 | 资源 |
|------|------|
| 开发板资料页 | [ESP32 开发板资料页](https://www.xinlucity.com/?s=resourcedetail/index/id/61.html) |
| 开发板数据手册 | [ESP32-S3技术规格书.pdf](硬件资料/技术文档/ESP32-S3技术规格书.pdf) |
| 开发板原理图 | [ESP32-S3原理图.pdf](硬件资料/技术文档/ESP32-S3原理图.pdf) |
| 开发板引脚图 | [ESP32-S3开发板引脚图.png](硬件资料/接线图/ESP32-S3开发板引脚图.png) |
| 电机手册 | [MG4010-i10电机手册.pdf](硬件资料/技术文档/MG4010-i10电机手册.pdf) |
| 电机协议 | [MG4010-i10-R485协议.pdf](硬件资料/技术文档/MG4010-i10-R485协议.pdf) |
| 转接板原理图 | [转接板原理图.pdf](硬件资料/技术文档/转接板原理图.pdf) |
| Windows 串口驱动 | [CH341SER.EXE](硬件资料/驱动程序/CH341SER.EXE) |
| 接线图 | [电机转接板接线标注.png](硬件资料/接线图/电机转接板接线标注.png) |
| 转接板实物图 | [电机转接板实物图.png](硬件资料/接线图/电机转接板实物图.png) |
| 电机 3D 模型 | [MG4010-i10电机.STEP](硬件资料/3D模型/电机/MG4010-i10电机.STEP) |
| 舵机 3D 模型 | [MG90S舵机主体.STEP](硬件资料/3D模型/舵机/MG90S舵机主体.STEP) |
| 舵盘 3D 模型 | [单向](硬件资料/3D模型/舵机/单向舵盘.STEP) / [双向](硬件资料/3D模型/舵机/双向舵盘.STEP) / [四向](硬件资料/3D模型/舵机/四向舵盘.STEP) |

更多说明见 [硬件资料 README](硬件资料/README.md)。

## 默认引脚

| 功能 | 默认 GPIO | 说明 |
|------|-----------|------|
| 板载普通 LED | GPIO2 | 开发板自带 LED，不需要外接 |
| RS485 TX | GPIO17 | 接转接板 TTL-485 调试口 `T`；通用 RS485 模块接 `DI` |
| RS485 RX | GPIO18 | 接转接板 TTL-485 调试口 `R`；通用 RS485 模块接 `RO` |
| RS485 DE/RE | GPIO16 | 只在使用散装 RS485 模块时需要；课程转接板不接 |
| 舵机 1-4 | GPIO10、GPIO11、GPIO12、GPIO13 | 只接信号线，舵机电源必须用独立 5V |

安全底线：

1. 舵机和电机不能从 ESP32 的 3.3V 引脚取电。
2. ESP32 GND、转接板 GND、电机电源负极、舵机电源负极必须共地。
3. 第一次测试电机先低速、小角度、空载。
4. 出现发热、异响、复位、失控时立即断电。

## 快速安装要点

国内安装 ESP32 板卡包建议使用 Espressif 国内镜像：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

在 Arduino IDE 的开发板管理器中安装带 `-cn` 后缀的 `esp32 by Espressif Systems` 版本。详细步骤见 [Windows 安装教程](教程文档/01_install_windows.md)。

推荐 ESP32-S3 参数：

```text
Board: ESP32S3 Dev Module
Upload Speed: 460800
USB CDC On Boot: Enabled
CPU Frequency: 240MHz
Flash Mode: QIO 或 DIO
Partition Scheme: Default
```

上传失败时，把 `Upload Speed` 改成 `115200`，或按住 `BOOT` 后再上传。

## 目录结构

```text
engineering-training/
├── README.md
├── 教程文档/
├── 示例代码/
├── 硬件资料/
│   ├── 技术文档/
│   ├── 驱动程序/
│   ├── 接线图/
│   └── 3D模型/
```
