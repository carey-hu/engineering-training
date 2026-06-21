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

// ==================== 初始化函数 ====================

void setup() {
  // Serial.begin() - 初始化串口通信
  // 参数：115200 - 波特率（Baud Rate）
  //
  // 【什么是波特率？】
  // 波特率表示每秒传输多少个数据位（比特）
  // 115200 表示每秒传输 115200 位数据
  //
  // 【常用波特率】
  // 9600   - 低速，适合简单通信
  // 115200 - 常用，速度快且稳定（推荐）
  // 921600 - 高速，可能不稳定
  //
  // 【重要】
  // 代码中的波特率必须与串口监视器的波特率一致！
  // 否则会显示乱码：������ÿ����
  Serial.begin(115200);

  // delay() - 延时 1000 毫秒 = 1 秒
  // 作用：等待串口稳定，确保第一条消息能正确发送
  // 某些开发板需要这个延时，否则第一条消息可能丢失
  delay(1000);

  // Serial.println() - 向串口输出一行文字，并自动换行
  // println = print line（打印一行）
  // 相当于：Serial.print("文字"); Serial.print("\n");
  Serial.println("ESP32-S3 Serial Test Start");

  // 此时串口监视器应该显示：
  // ESP32-S3 Serial Test Start
}

// ==================== 主循环函数 ====================

void loop() {
  // Serial.print() - 向串口输出文字，不换行
  // 可以连续调用多次，拼接成一行
  Serial.print("millis = ");

  // millis() - 返回程序运行的毫秒数
  //
  // 【返回值】
  // 类型：unsigned long（无符号长整型）
  // 范围：0 到 4,294,967,295 毫秒（约 49.7 天）
  // 计时起点：程序启动或复位时归零
  //
  // 【用途】
  // 1. 测量程序运行时间
  // 2. 实现非阻塞延时（不用 delay() 阻塞程序）
  // 3. 定时执行任务
  //
  // 【示例】
  // 程序运行 1 秒后：millis() 返回 1000
  // 程序运行 10 秒后：millis() 返回 10000
  Serial.println(millis());

  // 此时串口监视器显示完整一行，例如：
  // millis = 1003
  //
  // 注意：数字不是精确的 1000、2000、3000
  // 因为 delay(1000) 只是延时 1 秒，不包括程序执行时间
  // 实际周期约为 1.003 秒

  // delay() - 延时 1 秒
  // 单位：毫秒（milliseconds）
  // 1000 毫秒 = 1 秒
  delay(1000);

  // loop() 执行完毕后，自动从头开始
  // 每隔 1 秒打印一次 millis() 的值
}

// ==================== 知识扩展 ====================

/*
  【Serial.print() vs Serial.println()】

  - Serial.print()   - 输出后不换行
  - Serial.println() - 输出后自动换行

  示例：

  Serial.print("A");
  Serial.print("B");
  Serial.println("C");
  Serial.println("D");

  输出：
  ABC
  D

  【串口通信原理】

  ESP32 ----USB线----> 电脑
         (串口数据)

  1. ESP32 通过 USB 线发送数据到电脑
  2. 电脑的串口驱动接收数据
  3. Arduino IDE 的串口监视器显示数据

  【波特率不匹配会怎样？】

  代码：Serial.begin(115200);
  监视器：波特率选择 9600

  结果：显示乱码 ������

  原因：发送速度和接收速度不一致，就像：
  - 你说话速度是 1 倍速
  - 对方按 0.5 倍速理解
  - 结果听到的都是"嗯嗯啊啊"

  【millis() 溢出问题】

  millis() 最大值约 49.7 天后会归零（溢出）

  如果需要长时间运行，要考虑溢出情况：

  // ❌ 错误写法
  if (millis() > previousMillis + 1000) { ... }

  // ✅ 正确写法
  if (millis() - previousMillis >= 1000) { ... }

  【串口调试技巧】

  1. 在关键位置输出变量值
     Serial.print("angle = ");
     Serial.println(angle);

  2. 输出执行流程
     Serial.println("进入 setup()");
     Serial.println("进入 loop()");

  3. 输出调试信息
     Serial.print("错误：速度超出范围 ");
     Serial.println(speed);

  【预期输出示例】

  正常情况下，串口监视器应该显示：

  ESP32-S3 Serial Test Start
  millis = 1003
  millis = 2004
  millis = 3005
  millis = 4006
  millis = 5007
  ...

  注意：
  - 第一行只出现一次（setup 中输出）
  - 后面每秒出现一行（loop 中输出）
  - 数字每次增加约 1000（误差几毫秒是正常的）

  【下一步学习】

  完成本例程后，继续学习：
  - 示例代码/03_舵机基础控制/ - 控制舵机并用串口调试
  - 示例代码/04_WiFi网页控制LED/ - 学习 WiFi 和网页控制
*/
