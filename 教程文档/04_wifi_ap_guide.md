# 04 WiFi AP 模式与网页控制入门

> 本指南从零开始讲解 ESP32-S3 的 WiFi AP（接入点）模式和网页控制的基础原理，为后续的舵机、电机网页控制打下基础。

---

## 本章学习目标

完成本章后，你应该能够：

1. 理解 WiFi AP 模式和 STA 模式的区别
2. 让 ESP32-S3 创建一个 WiFi 热点
3. 用手机或电脑连接 ESP32 热点
4. 理解 IP 地址 `192.168.4.1` 的来源
5. 用浏览器访问 ESP32 的网页
6. 通过网页按钮控制板载 LED

---

## 1. WiFi 的两种工作模式

在开始编程之前，先理解 WiFi 设备的两种基本工作模式。

### 1.1 STA 模式（Station，工作站模式）

**类比：** 就像你的手机连接家里的路由器上网。

```
你的手机 → 连接到 → 家里的路由器 → 连接到 → 互联网
```

**特点：**
- 设备作为"客户端"，连接到别人的 WiFi
- 需要知道 WiFi 名称（SSID）和密码
- 可以上网、访问其他设备

**Arduino 代码示例：**
```cpp
WiFi.begin("你家WiFi名称", "WiFi密码");  // 连接到现有WiFi
```

---

### 1.2 AP 模式（Access Point，接入点模式）

**类比：** ESP32 自己变成一个路由器，别人可以连接到它。

```
你的手机 → 连接到 → ESP32 热点
```

**特点：**
- ESP32 作为"服务器"，创建一个 WiFi 热点
- 手机/电脑可以连接到这个热点
- **不能上网**（因为 ESP32 没有连接互联网）
- 适合局域网控制（如机器人遥控、智能家居）

**Arduino 代码示例：**
```cpp
WiFi.softAP("ESP32-Hotspot", "12345678");  // 创建热点
```

---

### 1.3 两种模式对比

| 特性 | STA 模式 | AP 模式（本教程使用） |
|------|---------|---------------------|
| 角色 | 客户端 | 服务器 |
| 类比 | 手机连WiFi | ESP32变成路由器 |
| 能否上网 | ✅ 能 | ❌ 不能 |
| 使用场景 | 物联网、数据上传 | 机器人控制、本地调试 |
| 连接方式 | 需要现有WiFi | 手机直连ESP32 |
| 代码函数 | `WiFi.begin()` | `WiFi.softAP()` |

**本教程为什么选择 AP 模式？**

1. **不需要额外路由器**：教室/实验室可能没有 WiFi，或者 WiFi 密码经常变
2. **调试方便**：不用担心网络连接问题
3. **适合机器人**：机器人是移动设备，不可能一直连着固定 WiFi

---

## 2. IP 地址基础

### 2.1 什么是 IP 地址？

**类比：** IP 地址就像门牌号，让你找到网络上的某个设备。

```
家庭地址：北京市朝阳区 xx 街 xx 号
IP 地址：192.168.4.1
```

### 2.2 IP 地址的格式

IP 地址由 **4 个数字** 组成，每个数字范围是 `0-255`：

```
192.168.4.1
 ↑   ↑  ↑ ↑
 段1 段2 段3 段4
```

### 2.3 为什么是 192.168.4.1？

当 ESP32 创建 AP 热点时，它会自动分配 IP 地址：

```
ESP32 自己：192.168.4.1（服务器）
连接的设备：192.168.4.2、192.168.4.3、192.168.4.4...（客户端）
```

**为什么是 `192.168` 开头？**

- `192.168.x.x` 是**私有 IP 地址段**，专门用于局域网（不能直接上互联网）
- 你家路由器通常是 `192.168.1.1` 或 `192.168.0.1`
- ESP32 默认使用 `192.168.4.1`

**记忆技巧：** ESP32 AP 模式的 IP 地址总是 `192.168.4.1`，这是固定的。

---

## 3. HTTP 协议基础

### 3.1 什么是 HTTP？

**类比：** HTTP 就像打电话的礼仪规则。

```
你打电话：喂，你好，我想订外卖
对方回复：好的，您要什么？
你回答：一份盖饭
对方确认：收到，30分钟送达
```

HTTP 也是这样的"问答"规则：

```
浏览器：给我 /index.html 页面（请求）
ESP32：好的，给你页面内容（响应）
```

### 3.2 HTTP 请求方法

| 方法 | 作用 | 类比 |
|------|------|------|
| **GET** | 获取信息 | 查看商品信息 |
| **POST** | 提交信息 | 提交订单 |

**本教程使用 GET 方法**，因为：
- 简单易懂
- 浏览器地址栏可以直接访问
- 适合控制命令（如开灯、关灯）

### 3.3 URL 的组成

```
http://192.168.4.1/led?state=on
  ↑        ↑         ↑     ↑
协议    IP地址      路径   参数
```

- **协议**：`http://`（网页通信规则）
- **IP地址**：`192.168.4.1`（ESP32 的地址）
- **路径**：`/led`（访问哪个"页面"）
- **参数**：`?state=on`（传递的数据）

**类比：**
```
寄信地址：北京市朝阳区 xx 街 xx 号 张三收 附言:生日快乐
URL：    http://192.168.4.1/led?state=on
```

---

## 4. HTML 基础

### 4.1 什么是 HTML？

**HTML** = HyperText Markup Language（超文本标记语言）

**类比：** HTML 就像 Word 文档的"源代码"，告诉浏览器哪里是标题、哪里是按钮。

### 4.2 HTML 的基本结构

```html
<!DOCTYPE html>
<html>
<head>
  <title>页面标题</title>  <!-- 浏览器标签页显示的名字 -->
</head>
<body>
  <h1>这是一级标题</h1>  <!-- 大号粗体文字 -->
  <p>这是一段普通文字</p>  <!-- 段落 -->
  <button>这是一个按钮</button>  <!-- 可点击的按钮 -->
</body>
</html>
```

**标签说明：**

| 标签 | 作用 | 显示效果 |
|------|------|---------|
| `<h1>` | 一级标题 | **大号粗体** |
| `<h2>` | 二级标题 | **中号粗体** |
| `<p>` | 段落 | 普通文字 |
| `<button>` | 按钮 | 可点击的按钮 |
| `<a href="...">` | 链接 | 可点击跳转 |

### 4.3 按钮点击事件

**问题：** 点击按钮后，怎么告诉 ESP32 "开灯"？

**方法 1：链接跳转**（本章使用，最简单）

```html
<a href="/led?state=on">
  <button>开灯</button>
</a>
```

点击后，浏览器自动访问 `http://192.168.4.1/led?state=on`

**方法 2：JavaScript**（下一章介绍，更灵活）

```html
<button onclick="fetch('/led?state=on')">开灯</button>
```

点击后，JavaScript 在后台访问，页面不刷新。

---

## 5. ESP32 WebServer 库

### 5.1 什么是 WebServer？

**类比：** WebServer 就像餐厅服务员，负责：

1. **接待客人**（接受浏览器请求）
2. **记录点单**（解析 URL 和参数）
3. **传菜**（返回 HTML 页面）

### 5.2 基本使用流程

```cpp
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);  // 创建服务器，监听 80 端口

void setup() {
  // 1. 创建 WiFi 热点
  WiFi.softAP("ESP32-LED", "12345678");
  
  // 2. 注册路由（哪个网址对应哪个函数）
  server.on("/", handleRoot);        // 访问根目录
  server.on("/led", handleLED);      // 访问 /led
  
  // 3. 启动服务器
  server.begin();
}

void loop() {
  // 4. 处理客户端请求
  server.handleClient();
}
```

### 5.3 路由注册详解

```cpp
server.on("/led", handleLED);
           ↑         ↑
         路径      处理函数
```

**含义：** 当浏览器访问 `http://192.168.4.1/led` 时，自动调用 `handleLED()` 函数。

### 5.4 获取 URL 参数

```cpp
void handleLED() {
  if (server.hasArg("state")) {  // 检查是否有 state 参数
    String state = server.arg("state");  // 获取参数值
    
    if (state == "on") {
      digitalWrite(LED_PIN, HIGH);  // 开灯
    } else {
      digitalWrite(LED_PIN, LOW);   // 关灯
    }
  }
  
  server.send(200, "text/plain", "OK");  // 返回响应
}
```

**访问示例：**
```
http://192.168.4.1/led?state=on   → state = "on"  → 开灯
http://192.168.4.1/led?state=off  → state = "off" → 关灯
```

---

## 6. 完整示例：网页控制 LED

### 6.1 接线

本例程使用**板载 LED**（GPIO2），**不需要接线**。

如果要接外部 LED：

```
ESP32 GPIO2 → 220Ω 电阻 → LED 长脚（正极）
LED 短脚（负极） → GND
```

### 6.2 完整代码

**对应示例：** `示例代码/04_舵机网页控制/04_wifi_web_led.ino`

关键代码片段：

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-LED";       // WiFi 热点名称
const char* password = "12345678";    // WiFi 密码（至少8位）

WebServer server(80);
const int LED_PIN = 2;

// 主页 HTML
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<title>LED 控制</title></head><body>";
  html += "<h1>ESP32 LED 控制面板</h1>";
  html += "<p><a href='/led?state=on'><button>开灯</button></a></p>";
  html += "<p><a href='/led?state=off'><button>关灯</button></a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// LED 控制
void handleLED() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED: ON");
    } else if (state == "off") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED: OFF");
    }
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // 创建 WiFi 热点
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());  // 输出 192.168.4.1
  
  // 注册路由
  server.on("/", handleRoot);
  server.on("/led", handleLED);
  
  // 启动服务器
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // 处理客户端请求
}
```

### 6.3 使用步骤

1. **上传代码** 到 ESP32
2. **打开串口监视器**，查看 IP 地址（应该是 `192.168.4.1`）
3. **手机 WiFi 设置** → 找到 `ESP32-LED` 热点 → 输入密码 `12345678` → 连接
4. **打开浏览器** → 输入 `http://192.168.4.1`
5. **点击按钮** → 观察 LED 和串口输出

### 6.4 预期现象

**成功标志：**
- ✅ 手机能搜到 `ESP32-LED` 热点
- ✅ 输入密码后能成功连接
- ✅ 浏览器能打开 `http://192.168.4.1` 并显示按钮
- ✅ 点击"开灯"，LED 亮起，串口输出 `LED: ON`
- ✅ 点击"关灯"，LED 熄灭，串口输出 `LED: OFF`

**常见问题：**

| 现象 | 可能原因 | 解决方法 |
|------|---------|---------|
| 搜不到热点 | WiFi 未启动 | 检查串口输出是否有 "AP IP: 192.168.4.1" |
| 连接后无法访问 | IP 地址错误 | 确认输入 `http://192.168.4.1`（不是 https） |
| 点击按钮无反应 | 代码未上传 | 重新上传代码 |
| LED 不亮 | GPIO 错误 | 检查 LED_PIN 是否为 2 |

---

## 7. 代码详解

### 7.1 WiFi 热点创建

```cpp
WiFi.softAP("ESP32-LED", "12345678");
             ↑            ↑
          热点名称      密码（至少8位）
```

**注意事项：**
- 密码必须至少 8 位，否则会变成开放热点（不安全）
- 热点名称建议用英文，避免中文乱码

### 7.2 获取 IP 地址

```cpp
Serial.println(WiFi.softAPIP());  // 输出 192.168.4.1
```

**为什么要输出 IP？**
- 确认 WiFi 已成功启动
- 虽然 AP 模式默认是 `192.168.4.1`，但养成好习惯

### 7.3 路由注册

```cpp
server.on("/", handleRoot);
```

**含义：**
- `/` 表示根目录，即 `http://192.168.4.1/`
- 访问根目录时，调用 `handleRoot()` 函数

### 7.4 HTML 拼接

```cpp
String html = "<!DOCTYPE html><html>...";
html += "<h1>标题</h1>";
html += "<button>按钮</button>";
```

**为什么用字符串拼接？**
- Arduino 没有直接写 HTML 文件的功能
- 需要在代码里用字符串存储 HTML 内容

**更好的写法**（适合长 HTML）：

```cpp
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>LED Control</title></head>
<body>
  <h1>ESP32 LED</h1>
  <button>开灯</button>
</body>
</html>
)rawliteral";
```

`R"rawliteral( ... )rawliteral"` 是 C++ 原始字符串语法，可以直接写多行内容。

### 7.5 响应返回

```cpp
server.send(200, "text/html", html);
              ↑       ↑          ↑
           状态码   内容类型    内容
```

**状态码说明：**
- `200`：成功
- `404`：页面不存在
- `500`：服务器错误

**内容类型：**
- `text/html`：HTML 页面
- `text/plain`：纯文本
- `application/json`：JSON 数据

---

## 8. 进阶话题

### 8.1 如何让页面更美观？

**方法 1：添加 CSS 样式**

```html
<style>
  body {
    font-family: Arial;
    text-align: center;
    background-color: #f0f0f0;
  }
  button {
    padding: 15px 30px;
    font-size: 20px;
    margin: 10px;
    border: none;
    border-radius: 5px;
    cursor: pointer;
  }
  .on { background-color: #4CAF50; color: white; }
  .off { background-color: #f44336; color: white; }
</style>
```

**方法 2：使用 Bootstrap**（需要联网）

```html
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css">
```

**注意：** AP 模式下无法访问外网，Bootstrap 样式不会加载。

### 8.2 如何避免页面刷新？

当前方法点击按钮后，页面会跳转刷新。

**问题：** 体验不好，感觉卡顿。

**解决：** 使用 JavaScript `fetch()` 异步请求（下一章详细讲解）。

```html
<button onclick="controlLED('on')">开灯</button>

<script>
function controlLED(state) {
  fetch('/led?state=' + state)
    .then(response => response.text())
    .then(data => console.log(data));
}
</script>
```

### 8.3 如何同时支持多个设备连接？

ESP32 AP 模式默认支持 **最多 4 个设备** 同时连接。

如果需要更多设备，可以修改：

```cpp
WiFi.softAP(ssid, password, 1, 0, 10);  // 最后一个参数是最大连接数
```

---

## 9. 安全注意事项

### 9.1 密码强度

```cpp
❌ WiFi.softAP("ESP32", "12345678");  // 弱密码，容易被破解
✅ WiFi.softAP("ESP32", "Aa123456!");  // 强密码，包含大小写+数字+符号
```

### 9.2 网络隔离

AP 模式下，连接到 ESP32 的设备**无法上网**，但可以：
- 访问 ESP32 的网页
- 与 ESP32 通信
- 与其他连接到同一热点的设备通信

**安全建议：**
- 不要在公共场所使用弱密码
- 不要在热点名称中暴露敏感信息

### 9.3 生产环境建议

如果是实际产品，建议：
- 使用 HTTPS（加密通信）
- 添加身份验证（用户名+密码）
- 限制请求频率（防止攻击）

**本教程是学习项目，不需要这些复杂功能。**

---

## 10. 常见问题

### 10.1 手机连上热点后无法访问

**检查项：**
1. 确认 IP 地址是 `http://192.168.4.1`（不是 https）
2. 确认手机已连接到 `ESP32-LED` 热点
3. 确认串口输出有 "HTTP server started"
4. 尝试在浏览器输入 `192.168.4.1`（不带 http://）

### 10.2 页面显示乱码

**原因：** HTML 未指定字符编码。

**解决：**
```html
<meta charset="UTF-8">
```

### 10.3 点击按钮后页面卡死

**原因：** `handleLED()` 中使用了 `delay()`。

**错误示例：**
```cpp
void handleLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(5000);  // ❌ 阻塞 5 秒，浏览器等待超时
  server.send(200, "text/plain", "OK");
}
```

**正确写法：**
```cpp
void handleLED() {
  digitalWrite(LED_PIN, HIGH);
  server.send(200, "text/plain", "OK");  // ✅ 立即返回
}
```

### 10.4 ESP32 复位后 WiFi 断开

**正常现象：** ESP32 复位后，WiFi 热点会重新创建，手机需要重新连接。

---

## 11. 学习检查点

完成本章后，请确认你能回答以下问题：

1. ✅ AP 模式和 STA 模式有什么区别？
2. ✅ 为什么 ESP32 AP 模式的 IP 地址是 `192.168.4.1`？
3. ✅ 什么是 HTTP 的 GET 请求？
4. ✅ `<button>` 标签和 `<a>` 标签有什么区别？
5. ✅ `server.on("/led", handleLED)` 是什么意思？
6. ✅ 如何从 URL 中获取参数？

---

## 12. 下一步学习

学完本章后，可以继续学习：

- **[05 舵机网页控制](05_web_servo_guide.md)** - 用网页控制舵机角度，学习 JavaScript 异步请求
- **[07 舵机指南](07_servo_guide.md)** - 深入学习舵机 PWM 控制原理
- **[09 电机网页控制](09_motor_web_control.md)** - 综合应用：网页控制电机

---

## 13. 参考资料

- **ESP32 Arduino WiFi 库文档**: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html
- **ESP32 WebServer 示例**: Arduino IDE → 文件 → 示例 → WebServer
- **HTML 教程**: https://www.runoob.com/html/html-tutorial.html
- **HTTP 协议入门**: https://developer.mozilla.org/zh-CN/docs/Web/HTTP

---

**编写日期**：2026-06-21  
**适用对象**：零基础学习者、工程训练项目  
**对应示例**：[04_舵机网页控制](../示例代码/04_舵机网页控制/04_wifi_web_led.ino)
