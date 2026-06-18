/*
  ESP32-S3 示例 07：MG4010-i10 速度闭环控制

  使用厂家 RS485 私有协议，速度闭环命令为 0xA2。
  学生输入的是输出轴速度，单位 dps（degree per second，度/秒）。
  MG4010-i10 减速比为 1:10，因此协议中的电机侧速度 = 输出轴速度 * 10。
  协议单位为 0.01dps/LSB，因此 raw = 输出轴 dps * 10 * 100。

  串口命令：
  数字  设置输出轴目标速度 dps，如 36 表示输出轴每秒 36 度
  +/-   加减 10 dps
  t     停止
  s     读取状态
  h     帮助
*/

#include <HardwareSerial.h>

#define RS485_TX      17
#define RS485_RX      18
#define RS485_DE_RE   16

#define MOTOR_ID      0x01
#define RS485_BAUD    115200
#define REDUCTION     10.0f

#define CMD_MOTOR_RUN    0x88
#define CMD_MOTOR_STOP   0x81
#define CMD_SPEED        0xA2
#define CMD_READ_STATE2  0x9C

HardwareSerial RS485(2);

float targetOutputDps = 0.0f;
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
  const uint16_t timeoutMs = 120;
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
  targetOutputDps = 0;
}

void setSpeed(float outputDps) {
  outputDps = constrain(outputDps, -360.0f, 360.0f);
  targetOutputDps = outputDps;

  int32_t raw = (int32_t)(outputDps * REDUCTION * 100.0f);
  uint32_t rawBytes = (uint32_t)raw;
  uint8_t data[4];
  data[0] = rawBytes & 0xFF;
  data[1] = (rawBytes >> 8) & 0xFF;
  data[2] = (rawBytes >> 16) & 0xFF;
  data[3] = (rawBytes >> 24) & 0xFF;

  motorRun();
  delay(5);
  sendCommand(CMD_SPEED, data, 4);
  Serial.printf(">>> 输出轴目标速度: %.1f dps，协议原始值: %ld\n", targetOutputDps, (long)raw);
}

void readStatus() {
  sendCommand(CMD_READ_STATE2);
  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(CMD_READ_STATE2, data, &len) || len < 7) {
    Serial.println("状态读取失败。");
    return;
  }

  int8_t temperature = (int8_t)data[0];
  int16_t iq = (int16_t)(data[1] | (data[2] << 8));
  int16_t motorDps = (int16_t)(data[3] | (data[4] << 8));
  uint16_t encoder = (uint16_t)(data[5] | (data[6] << 8));
  float outputDps = motorDps / REDUCTION;

  Serial.printf("温度: %d C, 电流采样: %d, 电机速度: %d dps, 输出轴约: %.1f dps, 编码器: %u\n",
                temperature, iq, motorDps, outputDps, encoder);
}

void handleCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  if (input == "t") {
    motorStop();
    Serial.println(">>> 停止");
  } else if (input == "+") {
    setSpeed(targetOutputDps + 10.0f);
  } else if (input == "-") {
    setSpeed(targetOutputDps - 10.0f);
  } else if (input == "s") {
    readStatus();
  } else if (input == "h") {
    printHelp();
  } else {
    setSpeed(input.toFloat());
  }
}

void printHelp() {
  Serial.println();
  Serial.println("===== MG4010-i10 速度闭环控制 =====");
  Serial.println("数字  设置输出轴速度 dps，例如 36");
  Serial.println("+/-   加减 10 dps");
  Serial.println("t     停止");
  Serial.println("s     读取状态");
  Serial.println("h     帮助");
  Serial.println("建议课堂首次测试从 30 到 60 dps 开始。");
  Serial.println("==================================");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);

  Serial.println();
  Serial.println("MG4010-i10 速度闭环控制");
  printHelp();
}

void loop() {
  handleCommand();

  if (targetOutputDps != 0 && millis() - lastReadTime > 500) {
    lastReadTime = millis();
    readStatus();
  }
}
