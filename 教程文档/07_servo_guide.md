# 07 舵机使用指南

> [!TIP]
> 本篇是 [学习路线](00_learning_path.md) 的第 4 步。帮助你从零开始理解舵机，避开常见坑，最终实现网页控制舵机。

## 本节目标

- 理解舵机工作原理和 PWM 控制
- 正确接线：外部 5V 供电、共地
- 实现串口控制单舵机
- 学会非阻塞平滑控制
- 网页控制舵机角度

## 需要的硬件

| 物品 | 用途 |
|------|------|
| ESP32-S3 开发板 | 主控 |
| 普通 PWM 舵机（如 SG90、MG90S） | 学习 50Hz PWM 控制 |
| 独立 5V 电源（至少 1A） | 给舵机供电 |
| 杜邦线 | 连接信号线和共地 |

> [!CAUTION]
> **安全底线：**
> 1. 舵机**不能**从 ESP32 的 3.3V 引脚取电，必须用独立 5V 电源。
> 2. ESP32 GND **必须**和舵机电源 GND 共地。
> 3. 首次测试先空载，确认方向和角度范围正常再装到机器人结构上。

---

## 步骤

### 1. 认识舵机

舵机（Servo）是一种带反馈控制的电机，可以精确转到指定角度（如 0°、90°、180°）。

**一个标准舵机有 3 根线：**

| 线的颜色 | 功能 | 接到哪里 |
|---------|------|---------|
| 红色 | 电源正极（VCC） | 外部 5V+ |
| 棕色/黑色 | 电源负极（GND） | 外部 5V- |
| 黄色/橙色/白色 | 信号线（PWM） | ESP32 GPIO |

**舵机的 PWM 控制规则：**

- **频率**：50Hz（周期 20ms）
- **脉宽范围**：500μs ~ 2500μs

| 脉宽 | 角度 |
|-----|------|
| 500μs | 0° |
| 1500μs | 90° |
| 2500μs | 180° |

---

### 2. 接线

```text
舵机红线 → 外部 5V+
舵机棕/黑线 → 外部 5V-
ESP32 GND → 外部 5V-（共地！）
舵机黄/橙/白线 → GPIO13
```

> [!IMPORTANT]
> **ESP32 GND 必须和外部电源 GND 连接（共地）**，否则舵机收不到正确的 PWM 信号。

---

### 3. ESP32-S3 的 PWM 配置

ESP32-S3 通过 LEDC（LED PWM 控制器）生成 PWM 信号。

**关键参数：**
- **频率**：50Hz（舵机标准）
- **分辨率**：16 位（0-65535）

**代码实现：**

```cpp
const int SERVO_PIN = 13;          // 舵机信号线接 GPIO13
const int PWM_FREQ = 50;           // 频率 50Hz
const int PWM_RESOLUTION = 16;     // 16 位分辨率

void setup() {
  Serial.begin(115200);
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RESOLUTION);
}

// 角度转占空比函数
uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  
  const int minUs = 500;   // 0° 对应 500μs
  const int maxUs = 2500;  // 180° 对应 2500μs
  
  int pulseUs = map(angle, 0, 180, minUs, maxUs);
  uint32_t maxDuty = (1UL << 16) - 1;  // 65535
  
  return (uint32_t)((pulseUs * maxDuty) / 20000UL);
}

void writeServo(int angle) {
  ledcWrite(SERVO_PIN, angleToDuty(angle));
}
```

---

### 4. 串口控制舵机

打开串口监视器，输入 0-180 之间的角度，控制舵机转动。

```cpp
void loop() {
  if (Serial.available() > 0) {
    int angle = Serial.parseInt();
    angle = constrain(angle, 0, 180);
    
    writeServo(angle);
    Serial.print("舵机转到: ");
    Serial.print(angle);
    Serial.println("°");
  }
}
```

**参考示例：** [`示例代码/03_舵机基础控制/03_servo_basic.ino`](../示例代码/03_舵机基础控制/03_servo_basic.ino)

---

### 5. 网页控制舵机

手机连接 ESP32 热点，通过网页按钮控制舵机角度。

**关键代码：**

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32S3_SERVO";
const char* password = "12345678";

WebServer server(80);

void setup() {
  ledcAttach(SERVO_PIN, 50, 16);
  writeServo(90);
  
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());  // 通常是 192.168.4.1
  
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.begin();
}

void loop() {
  server.handleClient();
}

void handleServo() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    angle = constrain(angle, 0, 180);
    writeServo(angle);
  }
  server.send(200, "text/plain", "OK");
}
```

**参考示例：** [`示例代码/04_舵机网页控制/04_servo_web_control.ino`](../示例代码/04_舵机网页控制/04_servo_web_control.ino)

---

## 完成标志

- [x] 舵机能响应串口输入的角度（0°、90°、180°）
- [x] 舵机转动时 ESP32 不复位，串口不断开
- [x] 手机连接 ESP32 热点，网页能控制舵机角度
- [x] 理解外部 5V 供电和共地的重要性

---

## 常见错误

| 现象 | 原因 | 处理 |
|------|------|------|
| 舵机抖动、发热 | 供电不足 | 1. 使用外部 5V 2A 电源<br>2. 在舵机电源线附近并联 100μF-470μF 电容<br>3. 检查 USB 线质量 |
| 舵机不转 | 接线错误 | 1. 确认舵机红线接外部 5V+（不是 ESP32 的 3.3V）<br>2. 确认 ESP32 GND 和外部电源 GND 共地<br>3. 确认信号线接 GPIO13 |
| 角度不准（偏左或偏右） | 脉宽范围不匹配 | 调整 `angleToDuty()` 中的 `minUs` 和 `maxUs`，如改为 `600` 和 `2400` |
| 网页控制卡顿 | 代码中用了 `delay()` | 使用非阻塞写法，用 `millis()` 代替 `delay()` |
| 串口输出乱码 | 波特率不一致 | 1. 确认代码中 `Serial.begin(115200)`<br>2. 串口监视器也选 115200<br>3. 上传后按 EN 键复位 |

> [!TIP]
> **舵机抖动是最常见问题**，优先检查供电（外部 5V 电源）和共地。

<details>
<summary><b>非阻塞平滑控制（进阶）</b></summary>

<br>

使用 `delay()` 会阻塞主循环，导致 WiFi 卡顿。推荐用 `millis()` 实现非阻塞控制：

```cpp
int currentAngle = 90;
int targetAngle = 90;
unsigned long lastServoUpdate = 0;
const unsigned long servoInterval = 15;

void updateServo() {
  unsigned long now = millis();
  if (now - lastServoUpdate < servoInterval) {
    return;
  }
  lastServoUpdate = now;
  
  if (currentAngle < targetAngle) {
    currentAngle++;
    writeServo(currentAngle);
  } else if (currentAngle > targetAngle) {
    currentAngle--;
    writeServo(currentAngle);
  }
}

void loop() {
  server.handleClient();  // WiFi 正常处理
  updateServo();          // 舵机平滑更新
}
```

</details>

<details>
<summary><b>多舵机控制要点</b></summary>

<br>

1. **每个舵机需要独立的 GPIO 引脚**
   ```cpp
   const int SERVO1_PIN = 10;
   const int SERVO2_PIN = 11;
   const int SERVO3_PIN = 12;
   const int SERVO4_PIN = 13;
   ```

2. **供电能力要足够**
   - 1 个舵机：500mA
   - 4 个舵机：2A-3A
   - 建议使用 **5V 3A** 以上电源

3. **避免同时启动**
   ```cpp
   writeServo1(90);
   delay(200);
   writeServo2(90);
   delay(200);
   writeServo3(90);
   delay(200);
   writeServo4(90);
   ```

**参考示例：** [`示例代码/09_机器人整合控制/`](../示例代码/09_机器人整合控制/)

</details>

---

**[← 上一篇：硬件资料说明](06_hardware_info.md)** ·
[返回学习路线](00_learning_path.md) ·
**[下一篇：电机指南 →](08_motor_guide.md)**
