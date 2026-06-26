/*
  ESP32-S3 示例 07：MG4010-i10 多圈位置闭环控制

  关键参数：
  - 角度：用户输入输出轴角度（度），内部转换为电机侧角度（需乘减速比 10）
  - 速度：用户输入输出轴转速（RPM），内部转换为 dps 再乘减速比
    RPM 与 dps 换算：1 RPM = 6 dps（1圈360度，1分钟60秒，360/60=6）
  - 软件零点：上电后把当前位置定义为 0 点

  接线与 05 基础控制例程相同，推荐使用课程配套 RS485/供电转接板：
  GPIO17(TX) -> 转接板 TTL-485 调试口 T，GPIO18(RX) -> R，ESP32 5V/GND -> USB调试 5V/G。
  转接板已处理 RS485 收发方向，不需要 GPIO16。

  串口命令：
  r       读取当前角度
  z       设为软件零点
  v 5     设置最大转速 5 RPM
  30      转到 30°
  +30     增加 30°
  -30     减少 30°
  t       停止
  off     关闭电机
*/

#include <HardwareSerial.h>

#define RS485_TX      17              // RS485 发送引脚
#define RS485_RX      18              // RS485 接收引脚
#define RS485_DE_RE   -1              // 方向控制引脚（-1表示不使用）
#define MOTOR_ID      0x01            // 电机 ID
#define RS485_BAUD    115200          // 通讯波特率

#define REDUCTION     10.0f           // 电机减速比 1:10
#define RPM_TO_DPS    6.0f            // RPM 转 dps 系数（360度/60秒=6）

#define MIN_ANGLE     -720.0f         // 最小角度限制（输出轴）
#define MAX_ANGLE     720.0f          // 最大角度限制（输出轴）
#define MIN_SPEED     1.0f            // 最小转速限制（输出轴 RPM）
#define MAX_SPEED     60.0f           // 最大转速限制（输出轴 RPM）

#define CMD_MOTOR_OFF    0x80         // 电机关闭命令
#define CMD_MOTOR_STOP   0x81         // 电机停止命令
#define CMD_MOTOR_RUN    0x88         // 电机运行命令
#define CMD_READ_ANGLE   0x92         // 读取角度命令
#define CMD_POSITION     0xA4         // 位置控制命令

HardwareSerial RS485(2);              // 使用串口2

int64_t softZeroRaw = 0;              // 软件零点原始值
float targetAngle = 0.0f;             // 目标角度（度）
float maxSpeed = 5.0f;                // 最大运行转速（RPM）

void setRS485Mode(bool isTx) {       // 设置 RS485 收发模式
  if (isTx && RS485_DE_RE >= 0) {
    digitalWrite(RS485_DE_RE, HIGH);  // 切换到发送模式
    delayMicroseconds(50);
  } else {
    RS485.flush();                    // 等待发送完成
    if (RS485_DE_RE >= 0) {
      delayMicroseconds(50);
      digitalWrite(RS485_DE_RE, LOW); // 切换到接收模式
    }
  }
}

template<typename T>
void writeLittleEndian(uint8_t *buf, T value) {  // 写入小端格式数据
  for (uint8_t i = 0; i < sizeof(T); i++) {
    buf[i] = (value >> (8 * i)) & 0xFF;          // 按字节拆分
  }
}

void sendCommand(uint8_t cmd, const uint8_t *data = nullptr, uint8_t dataLen = 0) {  // 发送协议命令
  uint8_t header[5] = {0x3E, cmd, MOTOR_ID, dataLen, 0x00};  // 构建帧头
  uint16_t sum = 0x3E + cmd + MOTOR_ID + dataLen;            // 计算头部校验和
  header[4] = sum & 0xFF;

  while (RS485.available()) RS485.read();  // 清空接收缓冲区

  setRS485Mode(true);                      // 切换到发送模式
  RS485.write(header, 5);                  // 发送帧头
  if (dataLen > 0 && data != nullptr) {
    RS485.write(data, dataLen);            // 发送数据部分
    sum = 0;
    for (uint8_t i = 0; i < dataLen; i++) sum += data[i];  // 计算数据校验和
    uint8_t chk = sum & 0xFF;
    RS485.write(&chk, 1);                  // 发送数据校验字节
  }
  setRS485Mode(false);                     // 切换到接收模式
}

bool waitForBytes(uint8_t *buffer, uint8_t count, unsigned long timeout) {  // 等待接收指定字节数
  uint8_t index = 0;
  unsigned long deadline = millis() + timeout;  // 计算超时时间
  while (millis() < deadline && index < count) {
    if (RS485.available()) buffer[index++] = RS485.read();  // 逐字节读取
  }
  return index == count;                   // 返回是否接收完整
}

bool readFrame(uint8_t expectedCmd, uint8_t *data, uint8_t *dataLen) {  // 读取并解析响应帧
  uint8_t header[5];
  uint8_t index = 0;
  unsigned long deadline = millis() + 200;  // 设置超时 200ms

  while (millis() < deadline && index < 5) {
    if (!RS485.available()) continue;
    uint8_t b = RS485.read();
    if (index == 0 && b != 0x3E) continue;  // 等待帧头标识
    header[index++] = b;
  }

  if (index < 5) return false;              // 帧头接收不完整

  uint16_t sum = header[0] + header[1] + header[2] + header[3];
  if (header[4] != (sum & 0xFF)) return false;  // 头部校验失败
  if (header[1] != expectedCmd || header[2] != MOTOR_ID) return false;  // 命令或 ID 不匹配

  *dataLen = header[3];
  if (*dataLen == 0) return true;           // 无数据部分

  uint8_t payload[32];
  if (!waitForBytes(payload, *dataLen + 1, 200)) return false;  // 接收数据和校验

  sum = 0;
  for (uint8_t i = 0; i < *dataLen; i++) sum += payload[i];
  if (payload[*dataLen] != (sum & 0xFF)) return false;  // 数据校验失败

  memcpy(data, payload, *dataLen);          // 拷贝有效数据
  return true;
}

bool sendMotorCommand(uint8_t cmd) {       // 发送简单电机命令
  sendCommand(cmd);
  uint8_t data[16], len;
  return readFrame(cmd, data, &len);       // 等待响应
}

bool readRawAngle(int64_t *rawAngle) {     // 读取电机原始角度值
  sendCommand(CMD_READ_ANGLE);
  uint8_t data[16], len;

  if (!readFrame(CMD_READ_ANGLE, data, &len) || len < 8) return false;  // 响应无效

  memcpy(rawAngle, data, 8);               // 提取 8 字节角度值
  return true;
}

void readAndPrintAngle() {                 // 读取并打印当前角度
  int64_t raw;
  if (!readRawAngle(&raw)) {
    Serial.println("读取失败");
    return;
  }

  float angle = (raw - softZeroRaw) / 100.0f / REDUCTION;  // 电机侧角度 → 输出轴角度
  Serial.printf("当前角度: %.2f°\n", angle);
}

bool setZeroHere() {                       // 将当前位置设为软件零点
  int64_t raw;
  if (!readRawAngle(&raw)) {
    Serial.println("设置零点失败");
    return false;
  }

  softZeroRaw = raw;                       // 记录零点原始值
  targetAngle = 0.0f;                      // 重置目标角度
  Serial.println("已设为软件零点");
  return true;
}

bool setPosition(float angle) {            // 设置电机目标位置（输出轴角度）
  angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);  // 限制角度范围
  targetAngle = angle;                     // 更新目标角度

  // 输出轴角度 → 电机侧角度 → 协议原始值
  int64_t targetRaw = softZeroRaw + (int64_t)(angle * REDUCTION * 100.0f);
  // 输出轴转速(RPM) → dps → 电机侧速度 → 协议原始值
  float maxSpeedDps = constrain(maxSpeed, MIN_SPEED, MAX_SPEED) * RPM_TO_DPS;
  uint32_t speedRaw = (uint32_t)(maxSpeedDps * REDUCTION * 100.0f);

  uint8_t data[12];
  writeLittleEndian(data, targetRaw);      // 写入目标位置（8字节）
  writeLittleEndian(data + 8, speedRaw);   // 写入最大速度（4字节）

  Serial.printf("目标: %.2f°, 转速: %.2f RPM (%.2f dps)\n", angle, maxSpeed, maxSpeedDps);

  if (!sendMotorCommand(CMD_MOTOR_RUN)) return false;  // 启用电机
  delay(5);

  sendCommand(CMD_POSITION, data, 12);     // 发送位置控制命令
  uint8_t rxData[16], len;
  return readFrame(CMD_POSITION, rxData, &len);
}

void handleCommand() {                     // 处理串口命令
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  if (input == "r") {                      // 读取角度
    readAndPrintAngle();
  } else if (input == "z") {               // 设置零点
    setZeroHere();
  } else if (input == "t") {               // 停止电机
    sendMotorCommand(CMD_MOTOR_STOP);
    Serial.println("停止");
  } else if (input == "off") {             // 关闭电机
    sendMotorCommand(CMD_MOTOR_OFF);
    Serial.println("关闭");
  } else if (input == "0") {               // 回零位
    setPosition(0.0f);
  } else if (input.startsWith("v")) {      // 设置转速（支持 v5 或 v 5）
    String speedStr = input.substring(1);
    speedStr.trim();  // 去除前后空格
    maxSpeed = constrain(speedStr.toFloat(), MIN_SPEED, MAX_SPEED);
    Serial.printf("转速设为 %.2f RPM\n", maxSpeed);
  } else if (input.startsWith("+") || input.startsWith("-")) {  // 增量控制
    setPosition(targetAngle + input.toFloat());
  } else {                                 // 绝对位置控制
    setPosition(input.toFloat());
  }
}

void setup() {
  Serial.begin(115200);                    // 初始化调试串口
  delay(1000);

  if (RS485_DE_RE >= 0) {
    pinMode(RS485_DE_RE, OUTPUT);          // 配置方向控制引脚
    digitalWrite(RS485_DE_RE, LOW);
  }

  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);  // 初始化 RS485 串口

  Serial.println("\n===== MG4010-i10 位置控制 =====");
  Serial.println("命令: r-读角度 z-零点 v5-转速 30-角度 +30-增量 t-停止 off-关闭");
  Serial.println("建议首次测试转速 5 RPM，角度从 30° 开始");

  if (setZeroHere()) {                     // 初始化软件零点
    Serial.println("初始化完成\n");
  } else {
    Serial.println("初始化失败，检查接线\n");
  }
}

void loop() {
  handleCommand();                         // 循环处理命令
}
