/*
  ESP32-S3 示例 06：MG4010-i10 RS485 基础控制

  本例程使用厂家《电机 RS485 通讯协议》中的私有帧格式，不是 Modbus-RTU。

  硬件连接（外接 RS485 模块，如 SP3485 / MAX3485 / 3.3V MAX485 模块）：
  ESP32-S3 GPIO17 (TX) -> RS485 模块 DI
  ESP32-S3 GPIO18 (RX) -> RS485 模块 RO
  ESP32-S3 GPIO16      -> RS485 模块 DE 和 RE（两个引脚连在一起）
  ESP32-S3 GND         -> RS485 模块 GND -> 电机电源 GND
  RS485 A              -> 电机 A/H（RS485-A）
  RS485 B              -> 电机 B/L（RS485-B）

  电机需要 12V~24V 独立电源供电，课堂建议 24V 2A 以上。

  串口命令：
  r  电机运行（Motor ON，命令 0x88）
  t  电机停止（Stop，命令 0x81）
  o  电机关闭（Motor OFF，命令 0x80）
  c  清除错误（命令 0x9B）
  s  读取状态 2（命令 0x9C）
  a  读取多圈角度（命令 0x92）
  z  设置当前位置为零点（写入 ROM，命令 0x19，不建议频繁使用）
  h  显示帮助
*/

#include <HardwareSerial.h>

#define RS485_TX      17
#define RS485_RX      18
#define RS485_DE_RE   16

#define MOTOR_ID      0x01
#define RS485_BAUD    115200

#define CMD_MOTOR_OFF     0x80
#define CMD_MOTOR_STOP    0x81
#define CMD_MOTOR_RUN     0x88
#define CMD_READ_ANGLE    0x92
#define CMD_CLEAR_ERROR   0x9B
#define CMD_READ_STATE2   0x9C
#define CMD_SET_ZERO_ROM  0x19

HardwareSerial RS485(2);

uint8_t byteSum(const uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum & 0xFF;
}

void rs485TxMode() {
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(50);
}

void rs485RxMode() {
  RS485.flush();
  delayMicroseconds(50);
  digitalWrite(RS485_DE_RE, LOW);
}

void clearRxBuffer() {
  while (RS485.available()) {
    RS485.read();
  }
}

void printHex(const uint8_t *data, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
}

void sendCommand(uint8_t cmd, const uint8_t *data, uint8_t dataLen) {
  uint8_t header[5] = {0x3E, cmd, MOTOR_ID, dataLen, 0x00};
  header[4] = byteSum(header, 4);

  clearRxBuffer();
  rs485TxMode();
  RS485.write(header, 5);
  if (dataLen > 0 && data != nullptr) {
    RS485.write(data, dataLen);
    uint8_t dataSum = byteSum(data, dataLen);
    RS485.write(&dataSum, 1);
  }
  rs485RxMode();

  Serial.print("TX: ");
  printHex(header, 5);
  if (dataLen > 0 && data != nullptr) {
    printHex(data, dataLen);
    uint8_t dataSum = byteSum(data, dataLen);
    printHex(&dataSum, 1);
  }
  Serial.println();
}

void sendCommand(uint8_t cmd) {
  sendCommand(cmd, nullptr, 0);
}

bool readFrame(uint8_t expectedCmd, uint8_t *data, uint8_t *dataLen) {
  const uint16_t timeoutMs = 200;
  uint8_t header[5];
  uint8_t index = 0;
  unsigned long deadline = millis() + timeoutMs;

  while (millis() < deadline && index < 5) {
    if (!RS485.available()) continue;
    uint8_t b = RS485.read();
    if (index == 0 && b != 0x3E) continue;
    header[index++] = b;
  }

  if (index < 5) return false;
  if (header[4] != byteSum(header, 4)) return false;
  if (header[1] != expectedCmd || header[2] != MOTOR_ID) return false;

  *dataLen = header[3];
  uint8_t need = *dataLen + ((*dataLen > 0) ? 1 : 0);
  uint8_t payload[32];
  index = 0;

  deadline = millis() + timeoutMs;
  while (millis() < deadline && index < need && index < sizeof(payload)) {
    if (RS485.available()) {
      payload[index++] = RS485.read();
    }
  }

  if (index < need) return false;
  if (*dataLen > 0 && payload[*dataLen] != byteSum(payload, *dataLen)) return false;

  for (uint8_t i = 0; i < *dataLen; i++) {
    data[i] = payload[i];
  }

  Serial.print("RX: ");
  printHex(header, 5);
  printHex(payload, need);
  Serial.println();
  return true;
}

void sendSimpleCommand(uint8_t cmd, const char *label) {
  Serial.print(">>> ");
  Serial.println(label);
  sendCommand(cmd);
  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(cmd, data, &len)) {
    Serial.println("未收到有效回复，请检查 ID、波特率、A/B 接线和共地。");
  }
}

void readState2() {
  Serial.println(">>> 读取状态 2");
  sendCommand(CMD_READ_STATE2);

  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(CMD_READ_STATE2, data, &len) || len < 7) {
    Serial.println("状态读取失败。");
    return;
  }

  int8_t temperature = (int8_t)data[0];
  int16_t iq = (int16_t)(data[1] | (data[2] << 8));
  int16_t speedDps = (int16_t)(data[3] | (data[4] << 8));
  uint16_t encoder = (uint16_t)(data[5] | (data[6] << 8));

  Serial.printf("温度: %d C, 转矩电流采样: %d, 电机速度: %d dps, 编码器: %u\n",
                temperature, iq, speedDps, encoder);
}

void readMultiAngle() {
  Serial.println(">>> 读取多圈角度");
  sendCommand(CMD_READ_ANGLE);

  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(CMD_READ_ANGLE, data, &len) || len < 8) {
    Serial.println("角度读取失败。");
    return;
  }

  uint64_t rawUnsigned = 0;
  for (uint8_t i = 0; i < 8; i++) {
    rawUnsigned |= ((uint64_t)data[i]) << (8 * i);
  }
  int64_t raw = (int64_t)rawUnsigned;
  float motorAngle = raw / 100.0f;
  float outputAngle = motorAngle / 10.0f;  // MG4010-i10 减速比 1:10

  Serial.printf("电机侧角度: %.2f deg, 输出轴角度: %.2f deg\n", motorAngle, outputAngle);
}

void handleCommand() {
  if (!Serial.available()) return;

  char cmd = Serial.read();
  while (Serial.available()) Serial.read();

  switch (cmd) {
    case 'r': sendSimpleCommand(CMD_MOTOR_RUN, "电机运行"); break;
    case 't': sendSimpleCommand(CMD_MOTOR_STOP, "电机停止"); break;
    case 'o': sendSimpleCommand(CMD_MOTOR_OFF, "电机关闭"); break;
    case 'c': sendSimpleCommand(CMD_CLEAR_ERROR, "清除错误"); break;
    case 's': readState2(); break;
    case 'a': readMultiAngle(); break;
    case 'z':
      Serial.println(">>> 设置零点到 ROM。该命令会写入 Flash，不要频繁使用。");
      sendSimpleCommand(CMD_SET_ZERO_ROM, "设置零点");
      break;
    case 'h': printHelp(); break;
    default:
      if (cmd > 32) Serial.println("未知命令，输入 h 查看帮助。");
      break;
  }
}

void printHelp() {
  Serial.println();
  Serial.println("===== MG4010-i10 RS485 基础控制 =====");
  Serial.println("r  电机运行");
  Serial.println("t  电机停止");
  Serial.println("o  电机关闭");
  Serial.println("c  清除错误");
  Serial.println("s  读取状态 2");
  Serial.println("a  读取多圈角度");
  Serial.println("z  设置当前位置为零点（写 ROM，谨慎使用）");
  Serial.println("h  显示帮助");
  Serial.println("====================================");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);

  Serial.println();
  Serial.println("MG4010-i10 RS485 基础控制");
  Serial.printf("RS485: GPIO%d(TX) GPIO%d(RX) GPIO%d(DE/RE) @ %d\n",
                RS485_TX, RS485_RX, RS485_DE_RE, RS485_BAUD);
  printHelp();
}

void loop() {
  handleCommand();
}
