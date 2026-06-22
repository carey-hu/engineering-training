/*
  ESP32-S3 示例 06：MG4010-i10 速度闭环控制

  使用厂家 RS485 私有协议，速度闭环命令为 0xA2。
  串口中输入的是输出轴速度，单位 dps（degree per second，度/秒）。
  MG4010-i10 减速比为 1:10，因此协议中的电机侧速度 = 输出轴速度 * 10。
  协议单位为 0.01dps/LSB，因此 raw = 输出轴 dps * 10 * 100。

  接线与 05 基础控制例程相同，推荐使用课程配套 RS485/供电转接板：
  GPIO17(TX) -> 转接板 TTL-485 调试口 T，GPIO18(RX) -> R，ESP32 5V/GND -> USB调试 5V/G。
  转接板已处理 RS485 收发方向，不需要 GPIO16。

  串口命令：
  数字  设置输出轴目标速度 dps，如 36 表示输出轴每秒 36 度
  +/-   加减 10 dps
  t     停止
  s     读取状态
  h     帮助
*/

#include <HardwareSerial.h>

#define RS485_TX      17  // RS485 发送引脚
#define RS485_RX      18  // RS485 接收引脚
#define RS485_DE_RE   -1  // 收发控制引脚，-1表示不使用
#define MOTOR_ID      0x01  // 电机ID地址
#define RS485_BAUD    115200  // RS485波特率
#define REDUCTION     10.0f  // 电机减速比 1:10

#define CMD_MOTOR_RUN    0x88  // 电机启动命令
#define CMD_MOTOR_STOP   0x81  // 电机停止命令
#define CMD_SPEED        0xA2  // 速度闭环控制命令
#define CMD_READ_STATE2  0x9C  // 读取状态命令

HardwareSerial RS485(2);  // 使用UART2
float targetOutputDps = 0.0f;  // 输出轴目标速度
unsigned long lastReadTime = 0;  // 上次读取状态时间戳

uint8_t byteSum(const uint8_t *data, uint8_t len) {  // 计算字节累加和校验
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum += data[i];
  return sum & 0xFF;  // 取低8位
}

void rs485SetMode(bool tx) {  // 切换RS485收发模式
  if (tx && RS485_DE_RE >= 0) digitalWrite(RS485_DE_RE, HIGH);  // 发送模式
  if (!tx) {
    RS485.flush();  // 等待发送完成
    if (RS485_DE_RE >= 0) digitalWrite(RS485_DE_RE, LOW);  // 接收模式
  }
}

void sendCommand(uint8_t cmd, const uint8_t *data = nullptr, uint8_t dataLen = 0) {  // 发送协议命令
  uint8_t header[5] = {0x3E, cmd, MOTOR_ID, dataLen, 0x00};  // 帧头格式
  header[4] = byteSum(header, 4);  // 计算帧头校验和

  while (RS485.available()) RS485.read();  // 清空接收缓冲区
  rs485SetMode(true);  // 切换到发送模式
  RS485.write(header, 5);  // 发送帧头
  if (dataLen > 0 && data != nullptr) {
    RS485.write(data, dataLen);  // 发送数据段
    uint8_t sum = byteSum(data, dataLen);  // 计算数据校验和
    RS485.write(&sum, 1);  // 发送数据校验和
  }
  rs485SetMode(false);  // 切换到接收模式
}

bool readFrame(uint8_t expectedCmd, uint8_t *data, uint8_t *dataLen) {  // 读取协议响应帧
  const uint16_t timeoutMs = 120;  // 超时时间120ms
  uint8_t header[5];
  uint8_t i = 0;
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline && i < 5) {  // 读取5字节帧头
    if (!RS485.available()) continue;
    uint8_t b = RS485.read();
    if (i == 0 && b != 0x3E) continue;  // 等待帧头标识0x3E
    header[i++] = b;
  }
  if (i < 5 || header[4] != byteSum(header, 4)) return false;  // 帧头校验失败
  if (header[1] != expectedCmd || header[2] != MOTOR_ID) return false;  // 命令或ID不匹配

  *dataLen = header[3];  // 获取数据段长度
  uint8_t need = *dataLen + (*dataLen > 0 ? 1 : 0);  // 需要读取的字节数（含校验）
  uint8_t payload[32];
  i = 0;
  deadline = millis() + timeoutMs;
  while (millis() < deadline && i < need && i < sizeof(payload)) {  // 读取数据段
    if (RS485.available()) payload[i++] = RS485.read();
  }
  if (i < need) return false;  // 数据读取不完整
  if (*dataLen > 0 && payload[*dataLen] != byteSum(payload, *dataLen)) return false;  // 数据校验失败
  for (uint8_t n = 0; n < *dataLen; n++) data[n] = payload[n];  // 拷贝有效数据
  return true;
}

void motorStop() {  // 停止电机
  sendCommand(CMD_MOTOR_STOP);
  targetOutputDps = 0;
}

void setSpeed(float outputDps) {  // 设置输出轴速度
  outputDps = constrain(outputDps, -360.0f, 360.0f);  // 限制速度范围
  targetOutputDps = outputDps;

  int32_t raw = (int32_t)(outputDps * REDUCTION * 100.0f);  // 转换为协议原始值
  uint8_t data[4] = {  // 小端模式打包4字节
    (uint8_t)(raw & 0xFF),
    (uint8_t)((raw >> 8) & 0xFF),
    (uint8_t)((raw >> 16) & 0xFF),
    (uint8_t)((raw >> 24) & 0xFF)
  };

  sendCommand(CMD_MOTOR_RUN);  // 先发送启动命令
  delay(5);  // 短暂延时
  sendCommand(CMD_SPEED, data, 4);  // 发送速度控制命令
  Serial.printf(">>> 输出轴目标速度: %.1f dps，协议原始值: %ld\n", targetOutputDps, (long)raw);
}

void readStatus() {  // 读取电机状态
  sendCommand(CMD_READ_STATE2);
  uint8_t data[16], len = 0;
  if (!readFrame(CMD_READ_STATE2, data, &len) || len < 7) {  // 接收响应帧
    Serial.println("状态读取失败。");
    return;
  }

  int8_t temperature = (int8_t)data[0];  // 温度，有符号
  int16_t iq = (int16_t)(data[1] | (data[2] << 8));  // 电流采样值
  int16_t motorDps = (int16_t)(data[3] | (data[4] << 8));  // 电机侧速度
  uint16_t encoder = (uint16_t)(data[5] | (data[6] << 8));  // 编码器值
  float outputDps = motorDps / REDUCTION;  // 计算输出轴速度

  Serial.printf("温度: %d C, 电流采样: %d, 电机速度: %d dps, 输出轴约: %.1f dps, 编码器: %u\n",
                temperature, iq, motorDps, outputDps, encoder);
}

void handleCommand() {  // 处理串口命令
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');  // 读取一行
  input.trim();  // 去除首尾空白
  if (input.length() == 0) return;

  if (input == "t") {  // 停止命令
    motorStop();
    Serial.println(">>> 停止");
  } else if (input == "+") {  // 加速命令
    setSpeed(targetOutputDps + 10.0f);
  } else if (input == "-") {  // 减速命令
    setSpeed(targetOutputDps - 10.0f);
  } else if (input == "s") {  // 查询状态命令
    readStatus();
  } else if (input == "h") {  // 帮助命令
    printHelp();
  } else {  // 数字直接设置速度
    setSpeed(input.toFloat());
  }
}

void printHelp() {  // 打印帮助信息
  Serial.println("\n===== MG4010-i10 速度闭环控制 =====\n"
                 "数字  设置输出轴速度 dps，例如 36\n"
                 "+/-   加减 10 dps\n"
                 "t     停止\n"
                 "s     读取状态\n"
                 "h     帮助\n"
                 "建议首次测试从 30 到 60 dps 开始。\n"
                 "==================================\n");
}

void setup() {
  Serial.begin(115200);  // 初始化USB串口
  if (RS485_DE_RE >= 0) {  // 如果使用收发控制引脚
    pinMode(RS485_DE_RE, OUTPUT);
    digitalWrite(RS485_DE_RE, LOW);  // 默认接收模式
  }
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);  // 初始化RS485串口
  Serial.println("MG4010-i10 速度闭环控制");
  printHelp();
}

void loop() {
  handleCommand();  // 处理用户命令
  if (targetOutputDps != 0 && millis() - lastReadTime > 500) {  // 运行时每500ms读取状态
    lastReadTime = millis();
    readStatus();
  }
}
