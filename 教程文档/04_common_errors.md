# 04 常见错误与解决办法

## 1. Boards Manager 下载失败

现象：

```text
Failed to install platform: esp32
Download failed
```

处理：

1. 确认使用的是国内镜像地址：

```text
https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json
```

2. 确认安装的是带 `-cn` 后缀的版本。
3. 不要点自动更新到非 `-cn` 版本。
4. 关闭代理或切换网络后重试。

---

## 2. 找不到 COM 口

排查顺序：

```text
换 USB 数据线
换 USB 接口
设备管理器看未知设备
安装 CH340/CH343 驱动
按住 BOOT 插入 USB
```

---

## 3. 上传卡在 Connecting...

处理：

```text
按住 BOOT → 点上传 → 出现 Connecting... → 松开 BOOT
```

仍失败则：

```text
Upload Speed 改为 115200
换 USB 线
关闭串口监视器
检查是否选错 COM 口
```

---

## 4. Serial Monitor 没输出

ESP32-S3 原生 USB 板常见原因：

```text
USB CDC On Boot 没有 Enabled
波特率不一致
程序中没有 Serial.begin(115200)
上传后没按 EN 复位
```

建议代码中加：

```cpp
Serial.begin(115200);
delay(1000);
Serial.println("ESP32-S3 start");
```

---

## 5. 中文路径导致异常

避免把项目放在：

```text
C:\Users\张三\桌面\新建文件夹
```

建议：

```text
D:\ArduinoProjects\esp32s3_class
```

---

## 6. Web 控制卡顿

教学项目里，如果用网页滑块控制舵机，可能高频发送请求导致 WebServer 阻塞。

建议：

```text
用按键代替滑块
或对滑块请求做 100ms~150ms 限流
舵机动作不要写在请求处理函数的 delay/for 里
```

---

## 7. MG4010 电机无响应

本课程使用的 MG4010-i10 RS485 电机采用厂家私有协议，帧头为 `0x3E`，不是 Modbus-RTU。

排查顺序：

```text
1. 波特率是否为 115200
2. 电机 ID 是否为 1
3. RS485 A/B 是否接反
4. ESP32、RS485 模块、电机电源负极是否共地
5. DE/RE 是否接到 GPIO16，并且两个引脚已连在一起
6. RS485 模块是否支持 3.3V 逻辑
7. 电机是否已经独立供电
```

首次测试建议：

```text
先运行 06_电机基础控制
先输入 s 读取状态
再输入 r 让电机进入运行状态
速度模式从 30dps 或 60dps 开始
位置模式从 30°、60°、90° 小角度开始
```
