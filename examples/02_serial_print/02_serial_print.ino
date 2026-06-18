/*
  ESP32-S3 示例 02：串口输出

  上传后打开 Serial Monitor，波特率选择 115200。
  如果没有输出，请检查：
  1. Tools → USB CDC On Boot 是否为 Enabled
  2. 端口是否选对
  3. 上传后是否按 EN/RST 复位
*/

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 Serial Test Start");
}

void loop() {
  Serial.print("millis = ");
  Serial.println(millis());
  delay(1000);
}
