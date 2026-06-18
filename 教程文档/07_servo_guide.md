# 07 舵机使用指南

> 本指南帮助学生从零开始理解舵机，避开常见坑，最终实现网页控制舵机。

---

## 1. 舵机是什么

舵机（Servo）是一种带反馈控制的电机，可以精确转到指定角度。

**常见应用场景**：
- 机器人关节
- 云台转向
- 机械臂控制
- 模型飞机舵面

**和普通电机的区别**：
- 普通电机：只能控制转速和方向，不知道转到哪了
- 舵机：可以精确控制角度（比如转到 45°、90°、135°）

---

## 2. 舵机的基本结构

一个标准舵机有 **3 根线**：

| 线的颜色 | 功能 | 接到哪里 |
|---------|------|---------|
| 红色 | 电源正极（VCC） | 外部 5V+ |
| 棕色/黑色 | 电源负极（GND） | 外部 5V- |
| 黄色/橙色/白色 | 信号线（PWM） | ESP32 GPIO |

**重要提醒**：
- 舵机需要 **外部 5V 电源供电**，不能直接接 ESP32 的 3.3V 引脚
- ESP32 的 GND 必须和外部电源的 GND 连接（**共地**）

---

## 3. 舵机的工作原理（简化版）

### 3.1 什么是 PWM

PWM（Pulse Width Modulation，脉宽调制）是一种用方波信号控制设备的技术。

**简单理解**：
- PWM 信号就是一个不断重复的方波
- 通过改变 **高电平持续时间**（脉宽），控制舵机角度

### 3.2 舵机的 PWM 控制规则

标准舵机的控制信号：
- **频率**：50Hz（周期 20ms = 20000μs）
- **脉宽范围**：500μs ~ 2500μs

**角度与脉宽的关系**：

| 脉宽 | 角度 | 说明 |
|-----|------|------|
| 500μs | 0° | 最左边 |
| 1000μs | 45° | 左侧 |
| 1500μs | 90° | 中间位置 |
| 2000μs | 135° | 右侧 |
| 2500μs | 180° | 最右边 |

**示意图**（文字版）：

```
周期 20ms（20000μs）
┌───┐                     ┌───┐
│   │                     │   │
│   │                     │   │
└───┴─────────────────────┘   └───
 500μs = 0°

┌──────┐                  ┌──────┐
│      │                  │      │
│      │                  │      │
└──────┴──────────────────┘      └──
 1500μs = 90°

┌────────────┐            ┌────────────┐
│            │            │            │
│            │            │            │
└────────────┴────────────┘            └──
 2500μs = 180°
```

---

## 4. ESP32-S3 的 PWM 配置

### 4.1 使用 LEDC 外设

ESP32-S3 通过 **LEDC（LED PWM 控制器）** 生成 PWM 信号。

**关键参数**：
- **频率**（frequency）：50Hz（舵机标准）
- **分辨率**（resolution）：推荐 16 位（0-65535）

### 4.2 代码实现

```cpp
const int SERVO_PIN = 13;          // 舵机信号线接 GPIO13
const int PWM_FREQ = 50;           // 频率 50Hz
const int PWM_RESOLUTION = 16;     // 16 位分辨率

void setup() {
  // 配置 PWM 通道
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RESOLUTION);
}
```

### 4.3 角度转占空比函数

```cpp
uint32_t angleToDuty(int angle) {
  // 限制角度范围
  angle = constrain(angle, 0, 180);
  
  // 定义脉宽范围（微秒）
  const int minUs = 500;   // 0° 对应 500μs
  const int maxUs = 2500;  // 180° 对应 2500μs
  
  // 将角度映射到脉宽
  int pulseUs = map(angle, 0, 180, minUs, maxUs);
  
  // 计算 16 位 PWM 最大值
  uint32_t maxDuty = (1UL << 16) - 1;  // 65535
  
  // 转换为占空比
  return (uint32_t)((pulseUs * maxDuty) / 20000UL);
}

void writeServo(int angle) {
  ledcWrite(SERVO_PIN, angleToDuty(angle));
}
```

**使用方法**：

```cpp
writeServo(0);    // 转到 0°
writeServo(90);   // 转到 90°
writeServo(180);  // 转到 180°
```

---

## 5. 常见问题与解决方法

### 5.1 舵机抖动（最常见）

**现象**：
- 舵机到达目标位置后不停抖动
- 发出"嗡嗡"声
- 舵机发热

**原因分析**：
1. **供电不足**（最常见原因）
   - 舵机工作电流可达 500mA-1A
   - ESP32 的 5V 引脚供电能力有限（通常来自 USB 口）
   - 多个舵机同时工作时供电不足

2. **电源纹波过大**
   - 劣质电源适配器
   - USB 线太细、太长
   - 没有加滤波电容

3. **PWM 信号不稳定**
   - 程序中有长时间的 `delay()`
   - WiFi 或其他中断导致 PWM 波形抖动

**解决方法**：

- **方法 1：使用外部稳压电源**

```
外部 5V 2A 电源 → 舵机红线
外部 GND → 舵机棕线 + ESP32 GND（共地）
ESP32 GPIO13 → 舵机黄线
```

- **方法 2：添加大电容滤波**

在舵机电源线附近并联一个 **100μF-470μF 电解电容**：

```
电容正极 → 舵机红线（5V）
电容负极 → 舵机棕线（GND）
```

- **方法 3：优化代码（避免阻塞）**

```cpp
// - 错误写法：阻塞主循环
void loop() {
  server.handleClient();
  
  // delay 会导致 WiFi 和 PWM 抖动
  for (int i = 0; i < 180; i++) {
    writeServo(i);
    delay(15);  // 阻塞 2.7 秒
  }
}

// - 正确写法：非阻塞
void loop() {
  server.handleClient();  // WiFi 请求处理
  updateServo();          // 舵机平滑更新（无 delay）
}
```

---

### 5.2 舵机不转

**现象**：
- 上传代码后舵机没有反应
- 串口输出正常，但舵机不动

**排查步骤**：

1. **检查接线**
   ```
   舵机红线 → 外部 5V+（不是 ESP32 的 3.3V）
   舵机棕线 → 外部 5V-
   ESP32 GND → 外部 5V-（共地！）
   舵机黄线 → ESP32 GPIO13
   ```

2. **检查电源**
   - 用万用表测量舵机红线，应该是 4.8V-6V
   - 舵机转动瞬间电压不应跌落超过 0.5V

3. **检查引脚号**
   - 代码中 `SERVO_PIN = 13` 是否和实际接线一致
   - ESP32-S3 的 GPIO 编号和丝印可能不一致

4. **检查 PWM 配置**
   ```cpp
   // 确认初始化代码存在
   ledcAttach(SERVO_PIN, 50, 16);
   writeServo(90);  // 启动时转到中间位置
   ```

---

### 5.3 舵机角度不准

**现象**：
- 设置 0° 但舵机没转到最左边
- 设置 180° 但舵机转不到最右边
- 90° 位置偏左或偏右

**原因**：
- 不同品牌舵机的脉宽范围略有差异
- 标准范围是 500μs-2500μs，但实际可能是 600μs-2400μs

**解决方法**：

调整 `angleToDuty()` 函数中的脉宽范围：

```cpp
uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  
  // 原始范围
  // const int minUs = 500;
  // const int maxUs = 2500;
  
  // 调整后的范围（根据实际舵机测试）
  const int minUs = 600;   // 增大最小脉宽
  const int maxUs = 2400;  // 减小最大脉宽
  
  int pulseUs = map(angle, 0, 180, minUs, maxUs);
  uint32_t maxDuty = (1UL << 16) - 1;
  return (uint32_t)((pulseUs * maxDuty) / 20000UL);
}
```

**调试技巧**：

1. 先测试 90°，确保舵机在中间位置
2. 再测试 0° 和 180°，观察是否到达机械极限
3. 如果某个方向转不到位，调整对应的 `minUs` 或 `maxUs`

---

### 5.4 多个舵机控制

**注意事项**：

1. **每个舵机需要独立的 GPIO 引脚**
   ```cpp
   const int SERVO1_PIN = 13;
   const int SERVO2_PIN = 14;
   const int SERVO3_PIN = 15;
   ```

2. **供电能力要足够**
   - 1 个舵机：500mA
   - 3 个舵机：1.5A-2A
   - 建议使用 **5V 3A** 以上电源

3. **避免同时启动**
   ```cpp
   // - 错误：3 个舵机同时启动，瞬间电流过大
   writeServo1(0);
   writeServo2(0);
   writeServo3(0);
   
   // - 正确：错开启动时间
   writeServo1(0);
   delay(200);
   writeServo2(0);
   delay(200);
   writeServo3(0);
   ```

---

### 5.5 串口输出乱码

**现象**：
- 打开串口监视器，看到一堆乱码
- 部分文字正常，部分乱码

**原因**：
- 波特率设置不一致
- ESP32-S3 原生 USB 的 `USB CDC On Boot` 未启用

**解决方法**：

1. **检查波特率**
   ```cpp
   Serial.begin(115200);  // 代码中设置 115200
   ```
   串口监视器也要选择 **115200** 波特率

2. **启用 USB CDC On Boot**（ESP32-S3 原生 USB）
   ```
   Tools → USB CDC On Boot → Enabled
   ```

3. **上传后按 EN 键复位**
   - 上传完成后，按一下开发板上的 EN（或 RST）键
   - 让程序从头运行

---

## 6. 从基础到网页控制的进阶路径

### 6.1 学习路径图

```
第 1 步：基础舵机控制
   ↓
第 2 步：串口输入角度
   ↓
第 3 步：非阻塞平滑控制
   ↓
第 4 步：WiFi 热点 + 网页控制
   ↓
第 5 步：网页滑块控制（进阶）
```

---

### 6.2 第 1 步：基础舵机控制

**目标**：让舵机转起来

**示例代码**：

```cpp
const int SERVO_PIN = 13;

uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  int pulseUs = map(angle, 0, 180, 500, 2500);
  uint32_t maxDuty = 65535;
  return (uint32_t)((pulseUs * maxDuty) / 20000UL);
}

void writeServo(int angle) {
  ledcWrite(SERVO_PIN, angleToDuty(angle));
}

void setup() {
  Serial.begin(115200);
  ledcAttach(SERVO_PIN, 50, 16);
  
  writeServo(90);  // 转到 90°
  Serial.println("舵机已初始化");
}

void loop() {
  // 循环转动
  writeServo(0);
  delay(1000);
  writeServo(90);
  delay(1000);
  writeServo(180);
  delay(1000);
}
```

**学习重点**：
- 理解 PWM 频率（50Hz）和分辨率（16 位）
- 掌握 `angleToDuty()` 函数的原理
- 理解舵机的 0°、90°、180° 位置

---

### 6.3 第 2 步：串口输入角度

**目标**：通过串口监视器手动控制舵机角度

**关键代码**：

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

**使用方法**：
1. 打开串口监视器（波特率 115200）
2. 输入 0-180 之间的数字，按回车
3. 观察舵机转到对应角度

**学习重点**：
- 掌握 `Serial.parseInt()` 读取整数
- 理解 `constrain()` 限制范围
- 学会用串口调试硬件

**参考示例**：`示例代码/03_舵机基础控制/`

---

### 6.4 第 3 步：非阻塞平滑控制

**目标**：舵机平滑转动，不阻塞主循环

**为什么需要非阻塞**：

```cpp
// - 阻塞写法：舵机转动时，WiFi、串口等都被卡住
void loop() {
  for (int i = 0; i < 180; i++) {
    writeServo(i);
    delay(15);  // 阻塞 2.7 秒
  }
}

// - 非阻塞写法：舵机平滑转动，其他功能正常
void loop() {
  server.handleClient();  // WiFi 正常处理
  updateServo();          // 舵机逐步更新
}
```

**关键代码**：

```cpp
int currentAngle = 90;
int targetAngle = 90;
unsigned long lastServoUpdate = 0;
const unsigned long servoInterval = 15;

void updateServo() {
  unsigned long now = millis();
  if (now - lastServoUpdate < servoInterval) {
    return;  // 时间未到，直接返回
  }
  lastServoUpdate = now;
  
  // 逐步逼近目标角度
  if (currentAngle < targetAngle) {
    currentAngle++;
    writeServo(currentAngle);
  } else if (currentAngle > targetAngle) {
    currentAngle--;
    writeServo(currentAngle);
  }
}

void loop() {
  // 处理其他任务
  Serial.println("主循环正常运行");
  
  // 更新舵机（不会阻塞）
  updateServo();
}
```

**学习重点**：
- 理解 `millis()` 计时原理
- 掌握非阻塞编程思想
- 学会用状态变量（`currentAngle`、`targetAngle`）

**参考示例**：`示例代码/03_舵机基础控制/`

---

### 6.5 第 4 步：WiFi 热点 + 网页控制

**目标**：手机连接 ESP32 热点，通过网页控制舵机

**关键代码**：

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32S3_SERVO";
const char* password = "12345678";

WebServer server(80);

void setup() {
  // 初始化舵机
  ledcAttach(SERVO_PIN, 50, 16);
  writeServo(90);
  
  // 启动 WiFi 热点
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  // 注册网页路由
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.begin();
}

void loop() {
  server.handleClient();  // 处理 HTTP 请求
  updateServo();          // 平滑更新舵机
}
```

**网页控制逻辑**：

```cpp
void handleServo() {
  if (server.hasArg("angle")) {
    targetAngle = server.arg("angle").toInt();
    targetAngle = constrain(targetAngle, 0, 180);
  }
  server.send(200, "text/plain", "OK");
}
```

**HTML 按钮**：

```html
<button onclick="setServo(45)">左 45°</button>
<button onclick="setServo(90)">回中 90°</button>
<button onclick="setServo(135)">右 135°</button>

<script>
function setServo(angle) {
  fetch('/servo?angle=' + angle);
}
</script>
```

**学习重点**：
- 掌握 ESP32 WiFi AP 模式
- 理解 HTTP GET 请求传参
- 学会网页 JavaScript 与 ESP32 通信

**参考示例**：`示例代码/05_舵机网页控制/`

---

### 6.6 第 5 步：网页滑块控制（进阶）

**目标**：网页滑块实时控制舵机角度

**注意事项**：
- 滑块会产生 **高频请求**（每移动 1° 发送一次）
- 如果处理不当，会导致 ESP32 WebServer 阻塞
- 必须使用 **非阻塞写法** + **请求限流**

**HTML 滑块代码**：

```html
<input type="range" min="0" max="180" value="90" 
       id="slider" oninput="updateServo()">
<p>当前角度: <span id="angle">90</span>°</p>

<script>
let lastRequest = 0;
const throttleDelay = 100;  // 限流 100ms

function updateServo() {
  const now = Date.now();
  if (now - lastRequest < throttleDelay) {
    return;  // 请求太频繁，跳过
  }
  lastRequest = now;
  
  const angle = document.getElementById('slider').value;
  document.getElementById('angle').innerText = angle;
  fetch('/servo?angle=' + angle);
}
</script>
```

**关键技巧**：
1. **JavaScript 限流**：100ms 内只发送一次请求
2. **服务端快速响应**：`handleServo()` 立即返回，不要 `delay()`
3. **非阻塞舵机更新**：`loop()` 中用 `updateServo()` 平滑转动

**学习重点**：
- 理解高频请求的危害
- 掌握 JavaScript 请求限流
- 优化 ESP32 WebServer 性能

---

## 7. 课堂教学建议

### 7.1 时间安排（2 学时）

| 环节 | 时间 | 内容 |
|-----|------|------|
| 舵机基础知识 | 10 分钟 | 舵机原理、PWM 控制、接线方法 |
| 第 1 步：基础控制 | 15 分钟 | 舵机转到固定角度 |
| 第 2 步：串口输入 | 15 分钟 | 串口监视器手动控制 |
| 第 3 步：非阻塞控制 | 20 分钟 | 平滑转动，理解 `millis()` |
| 第 4 步：网页控制 | 30 分钟 | WiFi 热点 + 网页按钮 |
| 常见问题排查 | 20 分钟 | 抖动、不转、角度不准 |

---

### 7.2 教学重点

1. **强调供电问题**
   - 舵机 **必须外部 5V 供电**
   - ESP32 GND **必须共地**
   - 这是最常见的问题来源

2. **非阻塞编程思想**
   - `delay()` 的危害
   - `millis()` 计时原理
   - 状态机思想

3. **调试技巧**
   - 用串口输出调试信息
   - 用万用表测量电压
   - 逐步排查问题

---

### 7.3 常见课堂问题

**问题 1**：学生接线时忘记共地

**现象**：舵机不转，或者随机抖动

**解决**：画接线图，强调 **ESP32 GND 必须连接外部电源 GND**

---

**问题 2**：学生代码中用了 `delay()`，导致 WiFi 卡顿

**现象**：网页控制舵机时，点击按钮没反应

**解决**：对比演示阻塞和非阻塞写法，让学生理解差异

---

**问题 3**：学生把舵机接到 ESP32 的 3.3V 引脚

**现象**：舵机抖动、发热、不转

**解决**：用万用表测量电压，强调 **舵机需要 5V，不能接 3.3V**

---

## 8. 总结

### 8.1 关键知识点

1. **舵机需要外部 5V 供电，ESP32 GND 必须共地**
2. **PWM 控制原理**：50Hz 频率，500μs-2500μs 脉宽
3. **非阻塞编程**：用 `millis()` 代替 `delay()`
4. **网页控制舵机**：WiFi AP + WebServer + JavaScript

---

### 8.2 常见问题速查表

| 问题 | 原因 | 解决方法 |
|-----|------|---------|
| 舵机抖动 | 供电不足 | 外部 5V 2A 电源 + 大电容 |
| 舵机不转 | 接线错误 | 检查 5V、GND、信号线 |
| 角度不准 | 脉宽范围不匹配 | 调整 `minUs`、`maxUs` |
| 网页卡顿 | 阻塞代码 | 非阻塞 + 请求限流 |
| 串口乱码 | 波特率不一致 | 统一 115200 |

---

### 8.3 进阶学习方向

1. **多个舵机协同控制**（机械臂、四足机器人）
2. **PCA9685 舵机驱动板**（同时控制 16 个舵机）
3. **舵机反馈**（360° 舵机、总线舵机）
4. **路径规划**（平滑轨迹、加速度控制）

---

## 9. 参考资源

- **本仓库示例代码**：
  - `示例代码/03_舵机基础控制/` - 基础舵机控制
  - `示例代码/05_舵机网页控制/` - 网页按钮控制

- **ESP32-S3 官方文档**：
  - [LEDC PWM 控制器](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)

- **舵机原理参考**：
  - [Wikipedia: Servo motor](https://en.wikipedia.org/wiki/Servo_(radio_control))

---

**编写日期**：2026-06-18  
**适用对象**：零基础学生、工程训练课、机器人课堂
