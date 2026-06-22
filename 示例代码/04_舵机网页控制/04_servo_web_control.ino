/*
  ESP32-S3 示例 04：网页按键控制舵机，非阻塞写法

  学习目标：
  - 网页按钮发送目标角度，loop() 中用 millis() 实现非阻塞舵机转动
  - 避免高频请求和阻塞式延时

  舵机接线：
  舵机红线 → 外部 5V+
  舵机棕/黑线 → 外部 5V-
  ESP32 GND → 外部 5V- 共地（必须）
  舵机黄/橙线 → ESP32 GPIO13

  使用方法：
  1. 上传代码后，连接 ESP32S3_SERVO Wi-Fi（密码 12345678）
  2. 浏览器访问 192.168.4.1
  3. 点击按钮控制舵机角度
*/

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32S3_SERVO";
const char* password = "12345678";

WebServer server(80);

const int SERVO_PIN = 13;
const int PWM_FREQ = 50;
const int PWM_RESOLUTION = 16;

int currentAngle = 90;
int targetAngle = 90;
unsigned long lastServoUpdate = 0;
const unsigned long servoInterval = 15;

uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  int pulseUs = map(angle, 0, 180, 500, 2500);
  return (uint32_t)((pulseUs * ((1UL << PWM_RESOLUTION) - 1)) / 20000UL);
}

void writeServo(int angle) {
  ledcWrite(SERVO_PIN, angleToDuty(angle));
}

String htmlPage() {
  return R"(<!DOCTYPE html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ESP32-S3 Servo</title>
<style>body{font-family:Arial;text-align:center;margin-top:40px}button{font-size:22px;padding:14px 24px;margin:8px}</style>
</head><body>
<h2>ESP32-S3 舵机按键控制</h2>
<p>建议先用按键，稳定后再改滑块。</p>
<button onclick="setServo(45)">左 45°</button>
<button onclick="setServo(90)">回中 90°</button>
<button onclick="setServo(135)">右 135°</button>
<script>function setServo(a){fetch('/servo?angle='+a).then(r=>r.text()).then(t=>console.log(t))}</script>
</body></html>)";
}

void updateServo() {
  unsigned long now = millis();
  if (now - lastServoUpdate < servoInterval) return;
  lastServoUpdate = now;

  if (currentAngle < targetAngle) {
    currentAngle++;
  } else if (currentAngle > targetAngle) {
    currentAngle--;
  } else {
    return;
  }
  writeServo(currentAngle);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RESOLUTION);
  writeServo(currentAngle);

  WiFi.softAP(ssid, password);
  Serial.printf("\nWi-Fi AP started\nSSID: %s\nIP: %s\n", ssid, WiFi.softAPIP().toString().c_str());

  server.on("/", []() { server.send(200, "text/html", htmlPage()); });
  server.on("/servo", []() {
    if (server.hasArg("angle")) {
      targetAngle = constrain(server.arg("angle").toInt(), 0, 180);
      Serial.printf("targetAngle = %d\n", targetAngle);
    }
    server.send(200, "text/plain", "OK");
  });
  server.begin();
}

void loop() {
  server.handleClient();
  updateServo();
}
