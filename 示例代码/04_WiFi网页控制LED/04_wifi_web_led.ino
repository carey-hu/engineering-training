/*
  ESP32-S3 示例 04：手机连接 ESP32 热点，用网页控制板载普通 LED

  本例程不需要外接 LED。手机连接 ESP32 热点后，可以在网页上控制
  开发板板载普通 LED 亮灭。

  说明：
  本教程默认板载普通 LED 接在 GPIO2。
  代码使用 pinMode() 和 digitalWrite()，和第一个 LED 闪烁例程保持一致。

  使用：
  1. 上传程序
  2. 手机连接 Wi-Fi：ESP32S3_CLASS，密码：12345678
  3. 浏览器打开：http://192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32S3_CLASS";
const char* password = "12345678";

const int LED_PIN = 2;
const int LED_ON = HIGH;
const int LED_OFF = LOW;
WebServer server(80);

String htmlPage() {
  String html = "";
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32-S3 Web LED</title>";
  html += "<style>body{font-family:Arial;text-align:center;margin-top:40px;}button{font-size:24px;padding:16px 32px;margin:10px;}</style>";
  html += "</head><body>";
  html += "<h2>ESP32-S3 网页控制板载 LED</h2>";
  html += "<button onclick=\"fetch('/on')\">开灯</button>";
  html += "<button onclick=\"fetch('/off')\">关灯</button>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleOn() {
  digitalWrite(LED_PIN, LED_ON);
  server.send(200, "text/plain", "LED ON");
}

void handleOff() {
  digitalWrite(LED_PIN, LED_OFF);
  server.send(200, "text/plain", "LED OFF");
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  Serial.begin(115200);
  delay(1000);

  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("Wi-Fi AP started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();

  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
