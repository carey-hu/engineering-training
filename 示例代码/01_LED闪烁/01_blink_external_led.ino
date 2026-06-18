/*
  ESP32-S3 示例 01：外接 LED 闪烁

  接线：
  ESP32-S3 GPIO2 → 220Ω电阻 → LED长脚
  LED短脚 → GND

  说明：
  本教程使用的 ESP32 开发板已在原理图中引出 GPIO2，适合直接接外部 LED。
  板载 RGB 灯是 WS2812，控制脚为 GPIO48，不能用普通 digitalWrite() 直接当 LED 点亮。
*/

const int LED_PIN = 2;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
