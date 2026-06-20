/*
  ESP32-S3 示例 01：板载普通 LED 闪烁

  上传后，开发板上的板载普通 LED 会每秒闪烁一次。
  本例程只使用 pinMode() 和 digitalWrite()，适合作为 GPIO 输出第一课。

  本教程默认板载普通 LED 接在 GPIO2。
  如果你的板子 LED 亮灭相反，交换 LED_ON 和 LED_OFF 的值即可。
*/

const int LED_PIN = 2;
const int LED_ON = HIGH;
const int LED_OFF = LOW;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);
}

void loop() {
  digitalWrite(LED_PIN, LED_ON);
  delay(500);

  digitalWrite(LED_PIN, LED_OFF);
  delay(500);
}
