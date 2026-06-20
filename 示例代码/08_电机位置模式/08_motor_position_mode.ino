/*
  ESP32-S3 示例 08：MG4010-i10 多圈位置闭环控制

  使用厂家 RS485 私有协议，多圈位置 + 速度限制命令为 0xA4。
  串口中输入输出轴角度和输出轴速度，代码按 i10 减速比换算到电机侧。

  协议单位：
  位置 angleControl：0.01 degree/LSB，int64_t，电机侧角度
  速度 maxSpeed：0.01 dps/LSB，uint32_t，电机侧速度

  接线与 06 基础控制例程相同，推荐使用课程配套 RS485/供电转接板：
  GPIO17 -> RS485 DI，GPIO18 -> RS485 RO，GPIO16 -> RS485 DE 和 RE。
  这三个 GPIO 均已在本教程使用的 ESP32 开发板排针上引出。

  串口命令：
  数字       设置输出轴目标角度，如 90
  p 数字     同上
  v 数字     设置输出轴最大速度 dps，如 v 90
  0          回到输出轴 0°
  +90/-90    在当前位置基础上增减 90°
  t          停止
  r          读取多圈角度
  h          帮助
*/

#include <HardwareSerial.h>

#define RS485_TX      17   // 排针已引出，接 RS485 转接板/模块 DI
#define RS485_RX      18   // 排针已引出，接 RS485 转接板/模块 RO
#define RS485_DE_RE   16   // 排针已引出，接 RS485 转接板/模块 DE/RE 或方向控制端

#define MOTOR_ID      0x01
#define RS485_BAUD    115200
#define REDUCTION     10.0f

#define CMD_MOTOR_RUN    0x88
#define CMD_MOTOR_STOP   0x81
#define CMD_POSITION     0xA4
#define CMD_READ_ANGLE   0x92

HardwareSerial RS485(2);

float targetOutputAngle = 0.0f;
float maxOutputDps = 90.0f;
unsigned long lastReadTime = 0;

uint8_t byteSum(const uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum += data[i];
  return sum & 0xFF;
}

void rs485Tx() {
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(50);
}

void rs485Rx() {
  RS485.flush();
  delayMicroseconds(50);
  digitalWrite(RS485_DE_RE, LOW);
}

void clearRx() {
  while (RS485.available()) RS485.read();
}

void sendCommand(uint8_t cmd, const uint8_t *data, uint8_t dataLen) {
  uint8_t header[5] = {0x3E, cmd, MOTOR_ID, dataLen, 0x00};
  header[4] = byteSum(header, 4);

  clearRx();
  rs485Tx();
  RS485.write(header, 5);
  if (dataLen > 0 && data != nullptr) {
    RS485.write(data, dataLen);
    uint8_t sum = byteSum(data, dataLen);
    RS485.write(&sum, 1);
  }
  rs485Rx();
}

void sendCommand(uint8_t cmd) {
  sendCommand(cmd, nullptr, 0);
}

bool readFrame(uint8_t expectedCmd, uint8_t *data, uint8_t *dataLen) {
  const uint16_t timeoutMs = 150;
  uint8_t header[5];
  uint8_t i = 0;
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline && i < 5) {
    if (!RS485.available()) continue;
    uint8_t b = RS485.read();
    if (i == 0 && b != 0x3E) continue;
    header[i++] = b;
  }
  if (i < 5 || header[4] != byteSum(header, 4)) return false;
  if (header[1] != expectedCmd || header[2] != MOTOR_ID) return false;

  *dataLen = header[3];
  uint8_t need = *dataLen + ((*dataLen > 0) ? 1 : 0);
  uint8_t payload[32];
  i = 0;
  deadline = millis() + timeoutMs;
  while (millis() < deadline && i < need && i < sizeof(payload)) {
    if (RS485.available()) payload[i++] = RS485.read();
  }
  if (i < need) return false;
  if (*dataLen > 0 && payload[*dataLen] != byteSum(payload, *dataLen)) return false;
  for (uint8_t n = 0; n < *dataLen; n++) data[n] = payload[n];
  return true;
}

void motorRun() {
  sendCommand(CMD_MOTOR_RUN);
}

void motorStop() {
  sendCommand(CMD_MOTOR_STOP);
}

void writeInt64LE(uint8_t *buf, int64_t value) {
  uint64_t raw = (uint64_t)value;
  for (uint8_t i = 0; i < 8; i++) {
    buf[i] = (raw >> (8 * i)) & 0xFF;
  }
}

void writeUint32LE(uint8_t *buf, uint32_t value) {
  buf[0] = value & 0xFF;
  buf[1] = (value >> 8) & 0xFF;
  buf[2] = (value >> 16) & 0xFF;
  buf[3] = (value >> 24) & 0xFF;
}

void setPosition(float outputAngle) {
  outputAngle = constrain(outputAngle, -720.0f, 720.0f);
  maxOutputDps = constrain(maxOutputDps, 10.0f, 360.0f);
  targetOutputAngle = outputAngle;

  int64_t angleRaw = (int64_t)(outputAngle * REDUCTION * 100.0f);
  uint32_t speedRaw = (uint32_t)(maxOutputDps * REDUCTION * 100.0f);

  uint8_t data[12];
  writeInt64LE(data, angleRaw);
  writeUint32LE(data + 8, speedRaw);

  motorRun();
  delay(5);
  sendCommand(CMD_POSITION, data, 12);

  Serial.printf(">>> 输出轴目标角度: %.1f deg，最大速度: %.1f dps\n",
                targetOutputAngle, maxOutputDps);
}

void readAngle() {
  sendCommand(CMD_READ_ANGLE);
  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(CMD_READ_ANGLE, data, &len) || len < 8) {
    Serial.println("角度读取失败。");
    return;
  }

  uint64_t rawUnsigned = 0;
  for (uint8_t i = 0; i < 8; i++) rawUnsigned |= ((uint64_t)data[i]) << (8 * i);
  int64_t raw = (int64_t)rawUnsigned;
  float motorAngle = raw / 100.0f;
  float outputAngle = motorAngle / REDUCTION;

  Serial.printf("电机侧角度: %.2f deg，输出轴角度: %.2f deg\n", motorAngle, outputAngle);
}

void handleCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  if (input == "t") {
    motorStop();
    Serial.println(">>> 停止");
  } else if (input == "r") {
    readAngle();
  } else if (input == "h") {
    printHelp();
  } else if (input == "0") {
    setPosition(0);
  } else if (input == "+90") {
    setPosition(targetOutputAngle + 90.0f);
  } else if (input == "-90") {
    setPosition(targetOutputAngle - 90.0f);
  } else if (input.startsWith("v ")) {
    maxOutputDps = input.substring(2).toFloat();
    maxOutputDps = constrain(maxOutputDps, 10.0f, 360.0f);
    Serial.printf(">>> 输出轴最大速度设为 %.1f dps\n", maxOutputDps);
  } else if (input.startsWith("p ")) {
    setPosition(input.substring(2).toFloat());
  } else {
    setPosition(input.toFloat());
  }
}

void printHelp() {
  Serial.println();
  Serial.println("===== MG4010-i10 多圈位置闭环 =====");
  Serial.println("数字       设置输出轴目标角度，例如 90");
  Serial.println("p 数字     同上，例如 p -45");
  Serial.println("v 数字     设置输出轴最大速度 dps，例如 v 90");
  Serial.println("0          回到输出轴 0°");
  Serial.println("+90/-90    相对当前目标增减 90°");
  Serial.println("t          停止");
  Serial.println("r          读取多圈角度");
  Serial.println("h          帮助");
  Serial.println("建议首次测试使用 30°、60°、90° 小角度。");
  Serial.println("=================================");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);

  Serial.println();
  Serial.println("MG4010-i10 多圈位置闭环控制");
  printHelp();
}

void loop() {
  handleCommand();

  if (millis() - lastReadTime > 1000) {
    lastReadTime = millis();
    readAngle();
  }
}
