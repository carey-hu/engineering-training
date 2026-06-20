/*
  ESP32-S3 示例 01：板载 RGB 闪烁

  本例程不需要外接元件。上传后，开发板上的板载 RGB 灯会每秒闪烁一次。

  本教程使用的 ESP32-S3 开发板板载 RGB 是 WS2812，控制脚为 GPIO48。
  WS2812 不能用普通 digitalWrite() 点亮，需要使用 Arduino-ESP32 提供的
  neopixelWrite() 函数发送专用时序。

  如果你的开发板没有板载 RGB，或板载 RGB 不在 GPIO48，请按硬件资料修改
  ONBOARD_RGB_PIN，或者改做 GPIO2 外接 LED 实验。
*/

const int ONBOARD_RGB_PIN = 48;
const int RGB_BRIGHTNESS = 20;  // 亮度不要太高，避免刺眼

void setup() {
  neopixelWrite(ONBOARD_RGB_PIN, 0, 0, 0);
}

void loop() {
  neopixelWrite(ONBOARD_RGB_PIN, RGB_BRIGHTNESS, 0, 0);
  delay(500);
  neopixelWrite(ONBOARD_RGB_PIN, 0, 0, 0);
  delay(500);
}
