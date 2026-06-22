/*
  ESP32-S3 示例 02：串口输出与调试

  【学习目标】
  1. 理解串口通信的基本概念
  2. 学会使用 Serial.begin() 初始化串口
  3. 学会使用 Serial.print() 和 Serial.println() 输出信息
  4. 理解波特率的含义
  5. 学会使用 millis() 获取程序运行时间

  【硬件接线】
  本例程不需要接线，只需要 USB 数据线连接电脑。

  【预期现象】
  上传成功后，打开串口监视器（Tools → Serial Monitor 或 Ctrl+Shift+M），
  波特率选择 115200，应该看到：

  ```
  ESP32-S3 Serial Test Start
  millis = 1003
  millis = 2004
  millis = 3005
  millis = 4006
  ...
  ```

  每秒打印一次，数字不断增大。

  【故障排查】
  如果串口监视器没有输出：
  1. 检查 Tools → USB CDC On Boot 是否为 Enabled（必须启用）
  2. 检查串口监视器右下角波特率是否为 115200
  3. 上传后按一下开发板的 EN 或 RST 键
  4. 确认串口未被其他程序占用

  如果显示乱码：
  1. 检查波特率是否匹配（代码和串口监视器都是 115200）
  2. 按 EN 键复位开发板

  【相关教程】
  - 学习路线：教程文档/00_learning_path.md
  - 常见问题：教程文档/FAQ_troubleshooting.md
*/

void setup() {
  Serial.begin(115200);  // 初始化串口，波特率115200
  delay(1000);  // 等待1秒让串口稳定
  Serial.println("ESP32-S3 Serial Test Start");  // 输出启动信息并换行
}

void loop() {
  Serial.print("millis = ");  // 输出提示文本，不换行
  Serial.println(millis());  // 输出运行毫秒数并换行
  delay(1000);  // 延时1秒
}
