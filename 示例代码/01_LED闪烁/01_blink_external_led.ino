/*
  ESP32-S3 示例 01：外接 LED 闪烁

  接线：
  GPIO2 → 220Ω电阻 → LED长脚
  LED短脚 → GND

  说明：
  不同 ESP32-S3 开发板的板载 LED 引脚不统一，课堂教学建议使用外接 LED。
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
