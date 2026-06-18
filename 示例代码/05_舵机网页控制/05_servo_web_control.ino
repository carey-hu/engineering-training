/*
  ESP32-S3 示例 05：网页按键控制舵机，非阻塞写法

  本例程的设计思路：
  - 不使用滑块高频请求
  - 网页按钮每次只发送一个目标角度
  - loop() 中用 millis() 慢慢转舵机

  舵机接线：
  舵机红线 → 外部 5V+
  舵机棕/黑线 → 外部 5V-
  ESP32 GND → 外部 5V- 共地
  舵机黄/橙线 → ESP32 GPIO13

  注意：舵机不要直接从 ESP32 3.3V 供电。
  GPIO13 已在本教程使用的 ESP32 开发板排针上引出，可以直接接舵机信号线。
*/

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32S3_SERVO";
const char* password = "12345678";

WebServer server(80);

const int SERVO_PIN = 13;        // 舵机信号线接 GPIO13，排针已引出
const int PWM_FREQ = 50;         // 舵机常用 50Hz
const int PWM_RESOLUTION = 16;   // 16 位分辨率

int currentAngle = 90;
int targetAngle = 90;
unsigned long lastServoUpdate = 0;
const unsigned long servoInterval = 15;

// 将角度转换为 16 位 LEDC 占空比
uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  const int minUs = 500;
  const int maxUs = 2500;
  int pulseUs = map(angle, 0, 180, minUs, maxUs);

  // 50Hz 周期 = 20000us
  uint32_t maxDuty = (1UL << PWM_RESOLUTION) - 1;
  return (uint32_t)((pulseUs * maxDuty) / 20000UL);
}

void writeServo(int angle) {
  ledcWrite(SERVO_PIN, angleToDuty(angle));
}

String htmlPage() {
  String html = "";
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32-S3 Servo</title>";
  html += "<style>body{font-family:Arial;text-align:center;margin-top:40px;}button{font-size:22px;padding:14px 24px;margin:8px;}</style>";
  html += "</head><body>";
  html += "<h2>ESP32-S3 舵机按键控制</h2>";
  html += "<p>建议先用按键，稳定后再改滑块。</p>";
  html += "<button onclick=\"setServo(45)\">左 45°</button>";
  html += "<button onclick=\"setServo(90)\">回中 90°</button>";
  html += "<button onclick=\"setServo(135)\">右 135°</button>";
  html += "<script>function setServo(a){fetch('/servo?angle='+a).then(r=>r.text()).then(t=>console.log(t));}</script>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleServo() {
  if (server.hasArg("angle")) {
    targetAngle = server.arg("angle").toInt();
    targetAngle = constrain(targetAngle, 0, 180);
    Serial.print("targetAngle = ");
    Serial.println(targetAngle);
  }

  // 立刻返回，不要在这里 for + delay 慢慢转
  server.send(200, "text/plain", "OK");
}

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

void checkLoopBlocked() {
  static unsigned long lastLoop = millis();
  unsigned long now = millis();
  if (now - lastLoop > 100) {
    Serial.print("Loop blocked for ");
    Serial.print(now - lastLoop);
    Serial.println(" ms");
  }
  lastLoop = now;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Arduino-ESP32 3.x 推荐写法：ledcAttach(pin, freq, resolution)
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RESOLUTION);
  writeServo(currentAngle);

  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("Wi-Fi AP started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.begin();
}

void loop() {
  server.handleClient();
  updateServo();
  checkLoopBlocked();
}
