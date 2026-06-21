# 05 网页控制舵机进阶

> 本指南在上一章 WiFi AP 基础上，实现用网页控制舵机角度。重点学习 JavaScript 异步请求、网页调试方法和非阻塞设计。

---

## 本章学习目标

完成本章后，你应该能够：

1. 用网页按钮控制舵机转到指定角度
2. 理解同步跳转和异步请求的区别
3. 使用 JavaScript `fetch()` 发送异步请求
4. 用浏览器开发者工具调试网页
5. 实现网页滑块实时控制舵机
6. 理解请求限流的必要性

---

## 0. 前置知识

在开始本章前，请确保你已经完成：

- ✅ **[04 WiFi AP 模式与网页控制入门](04_wifi_ap_guide.md)** - 理解 AP 模式、HTTP 请求、HTML 基础
- ✅ **[07 舵机指南](07_servo_guide.md)** - 理解 PWM 原理、舵机控制、非阻塞更新

---

## 1. 两种网页控制方式对比

### 1.1 方式一：链接跳转（同步，页面刷新）

**上一章使用的方法：**

```html
<a href="/led?state=on">
  <button>开灯</button>
</a>
```

**工作流程：**
```
点击按钮 → 浏览器跳转到新网址 → 页面刷新 → 显示新页面
```

**优点：**
- ✅ 代码简单，容易理解
- ✅ 不需要 JavaScript

**缺点：**
- ❌ 页面会刷新，体验不流畅
- ❌ 每次点击都要重新加载整个页面
- ❌ 按钮连续点击有明显延迟

---

### 1.2 方式二：异步请求（不刷新页面）

**本章使用的方法：**

```html
<button onclick="setServo(90)">回中 90°</button>

<script>
function setServo(angle) {
  fetch('/servo?angle=' + angle);  // 后台发送请求，不刷新页面
}
</script>
```

**工作流程：**
```
点击按钮 → JavaScript 在后台发送请求 → 页面不刷新 → 舵机转动
```

**优点：**
- ✅ 页面不刷新，体验流畅
- ✅ 可以连续快速点击
- ✅ 可以在页面上显示反馈（如"成功"提示）

**缺点：**
- ❌ 需要学习 JavaScript
- ❌ 代码稍微复杂一点

---

### 1.3 两种方式对比表

| 特性 | 链接跳转 | 异步请求（推荐） |
|------|---------|-----------------|
| 页面刷新 | ✅ 会刷新 | ❌ 不刷新 |
| 用户体验 | 卡顿 | 流畅 |
| 代码复杂度 | 简单 | 稍复杂 |
| 适用场景 | 学习演示 | 实际项目 |
| 需要 JavaScript | ❌ 不需要 | ✅ 需要 |

**本章选择异步请求的原因：**
- 舵机控制需要频繁调整角度
- 页面刷新会打断用户操作
- 实际项目中都使用异步请求

---

## 2. JavaScript fetch() 详解

### 2.1 什么是 fetch()？

**类比：** `fetch()` 就像你派一个快递员去送信，你不用等他回来，可以继续做其他事。

**传统方式（同步）：**
```
你：去送信
等待... 等待... 等待...
快递员回来了
你：继续做事
```

**fetch() 方式（异步）：**
```
你：去送信（然后继续做其他事）
快递员：默默送信，送完自己回来
你：一直在做其他事，没有等待
```

### 2.2 基本语法

```javascript
fetch('/servo?angle=90')  // 发送请求到 ESP32
  .then(response => response.text())  // 收到响应后，转换为文本
  .then(data => console.log(data));   // 打印响应内容
```

**简化版（常用）：**

```javascript
fetch('/servo?angle=90');  // 只发送请求，不处理响应
```

### 2.3 完整示例

```javascript
function setServo(angle) {
  fetch('/servo?angle=' + angle)  // 发送请求
    .then(response => {
      if (response.ok) {  // 检查是否成功（状态码 200）
        console.log('舵机已设置为 ' + angle + '°');
        return response.text();
      } else {
        console.error('请求失败');
      }
    })
    .then(data => {
      console.log('ESP32 返回：' + data);
    })
    .catch(error => {
      console.error('网络错误：', error);
    });
}
```

**代码解释：**

1. `fetch(url)` - 发送 HTTP 请求
2. `.then()` - 请求成功后执行
3. `response.ok` - 检查状态码是否为 200
4. `response.text()` - 将响应转换为文本
5. `.catch()` - 捕获错误（如网络断开）

### 2.4 fetch() vs 传统 AJAX

如果你听说过 **AJAX** 或 **XMLHttpRequest**，它们也是异步请求的方法。

**区别：**

```javascript
// 传统 AJAX（旧方法，复杂）
var xhr = new XMLHttpRequest();
xhr.open('GET', '/servo?angle=90', true);
xhr.onload = function() { console.log(xhr.responseText); };
xhr.send();

// fetch()（新方法，简洁）
fetch('/servo?angle=90')
  .then(response => response.text())
  .then(data => console.log(data));
```

**现代浏览器都支持 `fetch()`，建议使用新方法。**

---

## 3. 舵机网页控制实现

### 3.1 硬件接线

**单舵机测试：**

```
舵机红线 → 外部 5V+
舵机棕/黑线 → 外部 5V-
ESP32 GND → 外部 5V-（共地！）
舵机黄/橙/白线 → ESP32 GPIO13
```

**重要提醒：**
- ⚠️ 舵机不能从 ESP32 的 3.3V 或 5V 引脚取电
- ⚠️ 必须使用独立 5V 电源（如 USB 充电器、稳压模块）
- ⚠️ ESP32 GND 必须与舵机电源 GND 连接（共地）

### 3.2 完整代码

**对应示例：** `示例代码/05_舵机网页控制/05_servo_web_control.ino`

核心代码片段：

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-Servo";
const char* password = "12345678";

WebServer server(80);
const int SERVO_PIN = 13;

// 舵机状态
int currentAngle = 90;
int targetAngle = 90;
unsigned long lastServoUpdate = 0;
const unsigned long servoInterval = 15;

// 主页 HTML
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>舵机控制</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background-color: #f5f5f5;
      margin: 0;
      padding: 20px;
    }
    h1 { color: #333; }
    .button {
      display: inline-block;
      padding: 15px 30px;
      margin: 10px;
      font-size: 18px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      background-color: #4CAF50;
      color: white;
      transition: all 0.3s;
    }
    .button:hover {
      background-color: #45a049;
      transform: translateY(-2px);
    }
    .button:active {
      transform: translateY(0);
    }
    #status {
      margin-top: 20px;
      font-size: 24px;
      color: #666;
    }
  </style>
</head>
<body>
  <h1>ESP32 舵机控制</h1>
  <p>当前角度：<span id='status'>90</span>°</p>
  
  <div>
    <button class='button' onclick='setServo(0)'>0°</button>
    <button class='button' onclick='setServo(45)'>45°</button>
    <button class='button' onclick='setServo(90)'>90°</button>
    <button class='button' onclick='setServo(135)'>135°</button>
    <button class='button' onclick='setServo(180)'>180°</button>
  </div>
  
  <script>
    // 设置舵机角度
    function setServo(angle) {
      fetch('/servo?angle=' + angle)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerText = angle;
          console.log('舵机设置为 ' + angle + '°');
        })
        .catch(error => {
          console.error('请求失败:', error);
        });
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// 处理舵机控制请求
void handleServo() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    angle = constrain(angle, 0, 180);
    targetAngle = angle;
    
    Serial.print("目标角度: ");
    Serial.println(angle);
    
    server.send(200, "text/plain", "OK");  // 立即返回，不阻塞
  } else {
    server.send(400, "text/plain", "Missing angle parameter");
  }
}

void setup() {
  Serial.begin(115200);
  
  // 初始化舵机
  ledcAttach(SERVO_PIN, 50, 16);
  writeServo(currentAngle);
  
  // 创建 WiFi 热点
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  // 注册路由
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  
  // 启动服务器
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // 处理 HTTP 请求
  updateServo();          // 更新舵机位置（非阻塞）
}

// 舵机非阻塞更新函数
void updateServo() {
  unsigned long now = millis();
  if (now - lastServoUpdate < servoInterval) return;
  lastServoUpdate = now;
  
  if (currentAngle < targetAngle) {
    currentAngle++;
    writeServo(currentAngle);
  } else if (currentAngle > targetAngle) {
    currentAngle--;
    writeServo(currentAngle);
  }
}
```

### 3.3 代码关键点

#### 3.3.1 HTML 中的 JavaScript 函数

```javascript
function setServo(angle) {
  fetch('/servo?angle=' + angle)  // 发送异步请求
    .then(response => response.text())
    .then(data => {
      document.getElementById('status').innerText = angle;  // 更新页面显示
      console.log('舵机设置为 ' + angle + '°');
    })
    .catch(error => {
      console.error('请求失败:', error);
    });
}
```

**功能：**
1. 发送请求到 `/servo?angle=90`
2. 更新页面上的角度显示
3. 在控制台打印日志
4. 捕获网络错误

#### 3.3.2 ESP32 立即返回响应

```cpp
void handleServo() {
  if (server.hasArg("angle")) {
    targetAngle = server.arg("angle").toInt();
    server.send(200, "text/plain", "OK");  // ✅ 立即返回
  }
}
```

**为什么立即返回？**

❌ **错误写法（阻塞）：**
```cpp
void handleServo() {
  targetAngle = 90;
  while (currentAngle != targetAngle) {  // ❌ 等待舵机到位
    updateServo();
    delay(15);
  }
  server.send(200, "text/plain", "OK");  // 延迟几秒才返回
}
```

**问题：**
- 浏览器要等待几秒才收到响应
- 期间无法处理其他请求
- 用户体验差

✅ **正确写法（非阻塞）：**
```cpp
void handleServo() {
  targetAngle = 90;
  server.send(200, "text/plain", "OK");  // ✅ 立即返回
}

void loop() {
  server.handleClient();  // 处理请求
  updateServo();          // 舵机慢慢转动
}
```

**优点：**
- 浏览器立即收到响应
- 舵机在后台慢慢转动
- 可以同时处理多个请求

---

## 4. 浏览器开发者工具

### 4.1 如何打开开发者工具

**Chrome/Edge：**
- Windows: `F12` 或 `Ctrl + Shift + I`
- macOS: `Cmd + Option + I`

**Safari（macOS）：**
- 先启用：Safari → 偏好设置 → 高级 → 勾选"在菜单栏中显示开发菜单"
- 然后：`Cmd + Option + I`

### 4.2 控制台（Console）标签

**作用：**
- 查看 JavaScript 输出的日志
- 查看错误信息
- 手动执行 JavaScript 代码

**使用示例：**

1. 打开 `http://192.168.4.1`
2. 按 `F12` 打开开发者工具
3. 切换到 **Console（控制台）** 标签
4. 点击网页上的按钮，会看到日志：

```
舵机设置为 90°
```

5. 手动输入命令测试：

```javascript
setServo(120);  // 直接在控制台执行
```

### 4.3 网络（Network）标签

**作用：**
- 查看所有 HTTP 请求
- 查看请求和响应的详细信息
- 排查网络问题

**使用步骤：**

1. 打开开发者工具 → **Network（网络）** 标签
2. 点击网页上的按钮
3. 会看到一条新的请求记录：

```
servo?angle=90    200    OK    5ms
```

4. 点击这条记录，可以看到：
   - **Headers（请求头）**：URL、请求方法、状态码
   - **Response（响应）**：ESP32 返回的内容（"OK"）
   - **Timing（时间）**：请求耗时

### 4.4 常见错误排查

| 控制台错误 | 原因 | 解决方法 |
|-----------|------|---------|
| `Failed to fetch` | 网络断开 | 检查手机是否连接到 ESP32 热点 |
| `404 Not Found` | 路径错误 | 检查 ESP32 代码中的 `server.on()` |
| `TypeError: setServo is not defined` | 函数未定义 | 检查 HTML 中的 `<script>` 标签 |
| `ERR_CONNECTION_REFUSED` | ESP32 未启动 | 检查串口输出是否有 "HTTP server started" |

---

## 5. 进阶：滑块实时控制

### 5.1 为什么需要滑块？

**按钮控制的缺点：**
- 只能选择固定角度（0°、45°、90°...）
- 无法精确调整到任意角度（如 73°）

**滑块的优点：**
- 可以拖动到 0-180 之间任意角度
- 实时反馈，边拖边看效果

### 5.2 HTML 滑块控件

```html
<input type="range" 
       min="0" 
       max="180" 
       value="90" 
       id="slider" 
       oninput="updateServo()">
<p>当前角度: <span id="angle">90</span>°</p>
```

**属性说明：**
- `type="range"` - 滑块控件
- `min="0"` - 最小值
- `max="180"` - 最大值
- `value="90"` - 初始值
- `oninput="updateServo()"` - 拖动时触发函数

### 5.3 请求限流（重要！）

**问题：** 滑块每移动 1°，就发送一次请求。如果快速拖动，会产生 **高频请求**。

**危害：**
```
拖动滑块从 0° 到 180° 用时 1 秒
→ 产生 180 个请求
→ ESP32 WebServer 处理不过来
→ 页面卡死、ESP32 复位
```

**解决方法：请求限流（Throttle）**

```javascript
let lastRequest = 0;
const throttleDelay = 100;  // 限流间隔 100ms

function updateServo() {
  const now = Date.now();
  if (now - lastRequest < throttleDelay) {
    return;  // 距离上次请求不足 100ms，跳过
  }
  lastRequest = now;
  
  const angle = document.getElementById('slider').value;
  document.getElementById('angle').innerText = angle;
  fetch('/servo?angle=' + angle);
}
```

**效果：**
- 无限流：180 请求/秒 → ESP32 崩溃
- 限流 100ms：最多 10 请求/秒 → 稳定运行

### 5.4 完整滑块示例

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <title>舵机滑块控制</title>
  <style>
    body {
      font-family: Arial;
      text-align: center;
      padding: 50px;
    }
    #slider {
      width: 80%;
      height: 30px;
    }
    #angle {
      font-size: 48px;
      color: #4CAF50;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <h1>舵机滑块控制</h1>
  <p>当前角度: <span id='angle'>90</span>°</p>
  <input type='range' min='0' max='180' value='90' id='slider' oninput='updateServo()'>
  
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
      fetch('/servo?angle=' + angle)
        .catch(error => console.error('请求失败:', error));
    }
  </script>
</body>
</html>
```

---

## 6. 调试技巧

### 6.1 串口输出调试

在 ESP32 代码中增加调试信息：

```cpp
void handleServo() {
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] 收到请求: angle=");
  Serial.println(server.arg("angle"));
  
  targetAngle = server.arg("angle").toInt();
  server.send(200, "text/plain", "OK");
}
```

**串口输出示例：**
```
[1234] 收到请求: angle=90
[1450] 收到请求: angle=95
[1680] 收到请求: angle=100
```

### 6.2 网页端调试

在 JavaScript 中增加日志：

```javascript
function setServo(angle) {
  console.log('发送请求: angle=' + angle);
  fetch('/servo?angle=' + angle)
    .then(response => {
      console.log('响应状态:', response.status);
      return response.text();
    })
    .then(data => {
      console.log('ESP32 返回:', data);
    })
    .catch(error => {
      console.error('请求失败:', error);
    });
}
```

### 6.3 检查请求是否发送成功

**方法 1：查看串口**
```
如果串口有输出 → 请求已到达 ESP32
如果串口无输出 → 请求未发送或网络问题
```

**方法 2：查看浏览器 Network 标签**
```
状态码 200 → 成功
状态码 404 → 路径错误
状态码 500 → ESP32 代码错误
```

---

## 7. 常见问题

### 7.1 点击按钮无反应

**排查步骤：**

1. **打开浏览器控制台** → 看是否有错误
2. **检查网络标签** → 请求是否发送
3. **查看串口输出** → ESP32 是否收到请求
4. **检查 WiFi 连接** → 手机是否连接到 ESP32 热点

### 7.2 舵机抖动或卡顿

**可能原因：**

1. **供电不足** → 使用独立 5V 电源，增加电容
2. **请求太频繁** → 增加限流延迟（200ms）
3. **网络延迟** → AP 模式信号弱，靠近 ESP32

### 7.3 网页显示角度不准

**原因：** 网页显示的是目标角度，不是实际角度。

**解决方法：** ESP32 定期上报当前角度（需要双向通信，进阶话题）。

```cpp
// ESP32 定期推送当前角度（需要 WebSocket 或轮询）
void handleStatus() {
  String json = "{\"current\":" + String(currentAngle) + 
                ",\"target\":" + String(targetAngle) + "}";
  server.send(200, "application/json", json);
}
```

### 7.4 多个设备同时控制冲突

**现象：** A 设置 90°，B 设置 180°，舵机来回抖动。

**原因：** 两个设备同时修改 `targetAngle`。

**解决方法：**
- 方法 1：只允许一个设备连接
- 方法 2：增加设备优先级
- 方法 3：使用 WebSocket 实时同步状态

---

## 8. 扩展练习

### 8.1 增加预设位置

在网页上增加常用位置按钮：

```html
<button onclick="setServo(90)">回中</button>
<button onclick="setServo(0)">左极限</button>
<button onclick="setServo(180)">右极限</button>
<button onclick="setServo(45)">左45°</button>
<button onclick="setServo(135)">右45°</button>
```

### 8.2 增加速度控制

在 ESP32 中增加速度参数：

```cpp
int servoSpeed = 1;  // 每次移动的角度增量

void handleServo() {
  if (server.hasArg("speed")) {
    servoSpeed = server.arg("speed").toInt();
    servoSpeed = constrain(servoSpeed, 1, 10);
  }
  // ...
}

void updateServo() {
  if (currentAngle < targetAngle) {
    currentAngle += servoSpeed;  // 加速
  } else if (currentAngle > targetAngle) {
    currentAngle -= servoSpeed;
  }
}
```

网页端：

```html
<label>速度: <input type="range" min="1" max="10" value="1" onchange="setSpeed(this.value)"></label>

<script>
function setSpeed(speed) {
  fetch('/servo?speed=' + speed);
}
</script>
```

### 8.3 增加动画效果

在网页上增加 CSS 动画：

```css
.button {
  transition: all 0.3s;
}
.button:hover {
  transform: scale(1.1);
  box-shadow: 0 4px 8px rgba(0,0,0,0.2);
}
```

---

## 9. 学习检查点

完成本章后，请确认你能回答以下问题：

1. ✅ 同步跳转和异步请求有什么区别？
2. ✅ `fetch()` 函数的作用是什么？
3. ✅ 如何用浏览器开发者工具查看请求？
4. ✅ 为什么需要请求限流？
5. ✅ 如何在 JavaScript 中更新网页元素内容？
6. ✅ ESP32 为什么要立即返回响应而不是等待舵机到位？

---

## 10. 下一步学习

学完本章后，可以继续学习：

- **[08 电机指南](08_motor_guide.md)** - 学习 RS485 电机控制
- **[09 电机网页控制](09_motor_web_control.md)** - 综合应用：网页控制电机速度和位置
- **[10 机器人整合控制](09_robot_integration.md)** - 最终项目：两电机四舵机网页控制

---

## 11. 参考资料

- **上一章**: [04 WiFi AP 模式与网页控制入门](04_wifi_ap_guide.md)
- **舵机原理**: [07 舵机指南](07_servo_guide.md)
- **MDN fetch() 文档**: https://developer.mozilla.org/zh-CN/docs/Web/API/Fetch_API
- **JavaScript 教程**: https://www.runoob.com/js/js-tutorial.html

---

**编写日期**：2026-06-21  
**适用对象**：零基础学习者、工程训练项目  
**对应示例**：[05_舵机网页控制](../示例代码/05_舵机网页控制/05_servo_web_control.ino)
