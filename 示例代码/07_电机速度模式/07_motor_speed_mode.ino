/*
  ESP32-S3 示例 07：MG4010 闭环速度模式（Modbus-RTU）

  功能：
  - 闭环速度控制，设定目标转速（RPM），电机自动维持
  - 串口输入目标速度，实时调整
  - 定时读取实际速度反馈
  - 适合：传送带、风扇、搅拌器等恒速应用

  硬件连接：同 06_电机基础控制

  Modbus 寄存器说明（参照 MG4010-i10-R485 协议文档）：
  - 0x0001  控制字    （W） 0x0001=运行
  - 0x0002  目标速度  （W） 单位 RPM

  串口命令：
  - 数字     设置目标速度（RPM），如输入 100 回车
  - +/-      加减 10 RPM
  - t        停止（速度设 0）
  - s        读取状态
  - h        帮助
*/

#include <HardwareSerial.h>

// ==================== 引脚定义 ====================
#define RS485_TX      17
#define RS485_RX      18
#define RS485_DE_RE   16

// ==================== Modbus 协议 ====================
#define SLAVE_ID      0x01
#define BAUD_RATE     9600
#define FUNC_WRITE    0x06
#define FUNC_READ     0x03

#define REG_CONTROL   0x0001
#define REG_SPEED     0x0002

// ==================== 速度限制（根据电机规格） ====================
#define SPEED_MAX     3000   // 最大转速 RPM
#define SPEED_MIN     0

// ==================== 全局变量 ====================
HardwareSerial RS485(2);
int targetSpeed = 0;              // 目标速度 RPM
int actualSpeed = 0;              // 实际速度 RPM
unsigned long lastCmdTime = 0;
unsigned long lastReadTime = 0;

// ==================== Modbus CRC16 ====================
uint16_t modbusCRC(uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= buf[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

// ==================== RS485 收发控制 ====================
void rs485TxMode() { digitalWrite(RS485_DE_RE, HIGH); delayMicroseconds(20); }
void rs485RxMode() { digitalWrite(RS485_DE_RE, LOW);  delayMicroseconds(20); }

void sendModbus(uint8_t *frame, int len) {
  rs485TxMode();
  RS485.write(frame, len);
  RS485.flush();
  rs485RxMode();
}

// ==================== Modbus 写寄存器 ====================
void writeRegister(uint16_t reg, uint16_t value) {
  uint8_t frame[8];
  frame[0] = SLAVE_ID;
  frame[1] = FUNC_WRITE;
  frame[2] = (reg >> 8) & 0xFF;
  frame[3] = reg & 0xFF;
  frame[4] = (value >> 8) & 0xFF;
  frame[5] = value & 0xFF;

  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;

  sendModbus(frame, 8);
}

// ==================== 电机控制 ====================
void enableMotor() {
  writeRegister(REG_CONTROL, 0x0001);  // 使能
}

void stopMotor() {
  writeRegister(REG_SPEED, 0);
  delay(20);
  writeRegister(REG_CONTROL, 0x0000);  // 停机
  targetSpeed = 0;
}

void setSpeed(int rpm) {
  rpm = constrain(rpm, SPEED_MIN, SPEED_MAX);
  targetSpeed = rpm;
  writeRegister(REG_SPEED, rpm);
}

// ==================== 状态读取 ====================
void readStatus() {
  uint8_t frame[8];
  frame[0] = SLAVE_ID;
  frame[1] = FUNC_READ;
  frame[2] = 0x00;
  frame[3] = 0x01;   // 从 0x0001 开始读
  frame[4] = 0x00;
  frame[5] = 0x02;   // 读 2 个寄存器

  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;

  sendModbus(frame, 8);

  unsigned long timeout = millis() + 100;
  while (millis() < timeout) {
    if (RS485.available() >= 7) {
      uint8_t rx[32];
      int len = 0;
      while (RS485.available() && len < 32) {
        rx[len++] = RS485.read();
      }

      // 解析：地址+功能码+字节数+数据+CRC（至少 5 字节）
      if (len >= 5 && rx[0] == SLAVE_ID && rx[1] == FUNC_READ) {
        int dataLen = rx[2];
        if (len >= 5 + dataLen) {
          // 简单显示原始数据
          Serial.print("状态: ");
          for (int i = 3; i < 3 + dataLen; i += 2) {
            uint16_t val = (rx[i] << 8) | rx[i + 1];
            Serial.print(val);
            Serial.print(" ");
          }
          Serial.println();
        }
      }
      return;
    }
  }
}

// ==================== 串口命令 ====================
void handleCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  if (input == "t") {
    stopMotor();
    Serial.printf(">>> 停止，目标速度: %d RPM\n", targetSpeed);
  } else if (input == "+") {
    setSpeed(targetSpeed + 10);
    Serial.printf(">>> 目标速度: %d RPM\n", targetSpeed);
  } else if (input == "-") {
    setSpeed(targetSpeed - 10);
    Serial.printf(">>> 目标速度: %d RPM\n", targetSpeed);
  } else if (input == "s") {
    readStatus();
  } else if (input == "h") {
    printHelp();
  } else {
    int val = input.toInt();
    if (val >= 0 && val <= SPEED_MAX) {
      setSpeed(val);
      Serial.printf(">>> 目标速度: %d RPM\n", targetSpeed);
    }
  }
}

void printHelp() {
  Serial.println("\n===== MG4010 闭环速度模式 =====");
  Serial.printf("速度范围: %d ~ %d RPM\n", SPEED_MIN, SPEED_MAX);
  Serial.println("  数字  设置目标速度（如 500 回车）");
  Serial.println("  +/-   加减 10 RPM");
  Serial.println("  t     停止电机");
  Serial.println("  s     读取状态");
  Serial.println("  h     帮助");
  Serial.println("===============================\n");
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n============================");
  Serial.println(" MG4010 闭环速度模式");
  Serial.println("============================");

  pinMode(RS485_DE_RE, OUTPUT);
  rs485RxMode();
  RS485.begin(BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);

  enableMotor();
  delay(100);

  printHelp();
}

// ==================== 主循环 ====================
void loop() {
  handleCommand();

  // 每 200ms 读取状态
  if (millis() - lastReadTime > 200) {
    lastReadTime = millis();
    if (targetSpeed > 0) {
      readStatus();
    }
  }

  delay(10);
}
