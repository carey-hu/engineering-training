/*
  ESP32-S3 示例 08：MG4010 闭环位置模式（Modbus-RTU）

  功能：
  - 闭环位置控制，设定目标角度，电机自动转到指定位置
  - 可配置位置模式下的最大移动速度
  - 定时读取实际位置反馈
  - 适合：机械臂关节、云台、分度盘等定位应用

  硬件连接：同 06_电机基础控制

  Modbus 寄存器说明（参照 MG4010-i10-R485 协议文档）：
  - 0x0001  控制字      （W） 0x0003=位置模式+使能
  - 0x0003  目标位置高16位（W） 32 位位置值的高字
  - 0x0004  目标位置低16位（W） 32 位位置值的低字
  - 0x0005  位置模式速度  （W） RPM

  位置单位：0.01°（即值 36000 = 360° = 1圈）

  串口命令：
  - 数字       设置目标角度（度）
  - p 数字     同上
  - s 数字     设置最大速度（RPM）
  - 0/-360/360 快捷定位
  - t          停止
  - r          读取位置
  - h          帮助
*/

#include <HardwareSerial.h>

// ==================== 引脚定义 ====================
#define RS485_TX      17
#define RS485_RX      18
#define RS485_DE_RE   16

// ==================== Modbus 协议 ====================
#define SLAVE_ID      0x01
#define BAUD_RATE     9600
#define FUNC_WRITE     0x06
#define FUNC_WRITE_MULTI 0x10  // 写多个寄存器
#define FUNC_READ      0x03

#define REG_CONTROL   0x0001
#define REG_POS_HI    0x0003
#define REG_POS_LO    0x0004
#define REG_POS_SPEED 0x0005

// 控制字
#define CTRL_STOP     0x0000
#define CTRL_RUN      0x0001
#define CTRL_POS_MODE 0x0003  // 位置模式 + 使能

// ==================== 位置限制 ====================
#define POS_MAX       4500.0f   // 最大角度
#define POS_MIN      -4500.0f
#define SPEED_MAX     400       // 位置模式最大速度 RPM

// ==================== 全局变量 ====================
HardwareSerial RS485(2);
float targetAngle = 0;            // 目标角度（度）
int   positionSpeed = 200;        // 位置模式速度 RPM
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

// ==================== RS485 控制 ====================
void rs485TxMode() { digitalWrite(RS485_DE_RE, HIGH); delayMicroseconds(20); }
void rs485RxMode() { digitalWrite(RS485_DE_RE, LOW);  delayMicroseconds(20); }

void sendModbus(uint8_t *frame, int len) {
  rs485TxMode();
  RS485.write(frame, len);
  RS485.flush();
  rs485RxMode();
}

// 写单个寄存器
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

// 写多个寄存器（功能码 0x10）
void writeMultipleRegisters(uint16_t startReg, uint16_t count,
                            uint16_t *values) {
  uint8_t frame[64];
  frame[0] = SLAVE_ID;
  frame[1] = FUNC_WRITE_MULTI;
  frame[2] = (startReg >> 8) & 0xFF;
  frame[3] = startReg & 0xFF;
  frame[4] = (count >> 8) & 0xFF;
  frame[5] = count & 0xFF;
  frame[6] = count * 2;  // 字节数

  for (int i = 0; i < count; i++) {
    frame[7 + i * 2]     = (values[i] >> 8) & 0xFF;
    frame[7 + i * 2 + 1] = values[i] & 0xFF;
  }

  int len = 7 + count * 2;
  uint16_t crc = modbusCRC(frame, len);
  frame[len]     = crc & 0xFF;
  frame[len + 1] = (crc >> 8) & 0xFF;

  sendModbus(frame, len + 2);
}

// ==================== 电机控制 ====================
void enableMotor() {
  writeRegister(REG_CONTROL, CTRL_RUN);
}

void stopMotor() {
  writeRegister(REG_CONTROL, CTRL_STOP);
}

// 设置位置（度）
void setPosition(float angleDeg) {
  angleDeg = constrain(angleDeg, POS_MIN, POS_MAX);
  targetAngle = angleDeg;

  // 角度 → 0.01° 单位
  int32_t posRaw = (int32_t)(angleDeg * 100.0f);

  // 写位置模式速度
  writeRegister(REG_POS_SPEED, positionSpeed);
  delay(5);

  // 写 32 位目标位置（寄存器 0x0003-0x0004）
  uint16_t posHi = (uint32_t)(posRaw >> 16) & 0xFFFF;
  uint16_t posLo = (uint32_t)(posRaw & 0xFFFF);

  uint16_t values[2] = {posHi, posLo};
  writeMultipleRegisters(REG_POS_HI, 2, values);
  delay(5);

  // 启动位置模式
  writeRegister(REG_CONTROL, CTRL_POS_MODE);

  Serial.printf(">>> 目标位置: %.1f° (0x%04X%04X)  速度: %d RPM\n",
                angleDeg, posHi, posLo, positionSpeed);
}

// ==================== 状态读取 ====================
void readPosition() {
  uint8_t frame[8];
  frame[0] = SLAVE_ID;
  frame[1] = FUNC_READ;
  frame[2] = 0x00;
  frame[3] = 0x01;
  frame[4] = 0x00;
  frame[5] = 0x04;  // 读 4 个寄存器
  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;
  sendModbus(frame, 8);

  unsigned long timeout = millis() + 100;
  while (millis() < timeout) {
    if (RS485.available() >= 7) {
      uint8_t rx[32];
      int len = 0;
      while (RS485.available() && len < 32) rx[len++] = RS485.read();

      if (len >= 5 && rx[0] == SLAVE_ID && rx[1] == FUNC_READ) {
        Serial.print("状态: ");
        for (int i = 3; i < 3 + rx[2]; i += 2) {
          uint16_t val = (rx[i] << 8) | rx[i + 1];
          Serial.print(val);
          Serial.print(" ");
        }
        Serial.println();
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

  if (input == "0") {
    setPosition(0);
  } else if (input == "360" || input == "+360") {
    setPosition(targetAngle + 360);
  } else if (input == "-360") {
    setPosition(targetAngle - 360);
  } else if (input == "t") {
    stopMotor();
    Serial.println(">>> 停止");
  } else if (input == "r") {
    readPosition();
  } else if (input == "h") {
    printHelp();
  } else if (input.startsWith("s ")) {
    int spd = input.substring(2).toInt();
    if (spd > 0 && spd <= SPEED_MAX) {
      positionSpeed = spd;
      Serial.printf(">>> 位置模式速度设为 %d RPM\n", positionSpeed);
    }
  } else if (input.startsWith("p ")) {
    float ang = input.substring(2).toFloat();
    setPosition(ang);
  } else {
    float val = input.toFloat();
    if (val >= POS_MIN && val <= POS_MAX) {
      setPosition(val);
    }
  }
}

void printHelp() {
  Serial.println("\n===== MG4010 闭环位置模式 =====");
  Serial.printf("角度范围: %.0f° ~ %.0f°\n", POS_MIN, POS_MAX);
  Serial.println("  数字       目标角度（如 180）");
  Serial.println("  p 角度     同上（如 p 180）");
  Serial.println("  s 速度     位置模式速度 RPM（如 s 200）");
  Serial.println("  0/-360/360 快捷定位");
  Serial.println("  t          停止");
  Serial.println("  r          读取位置");
  Serial.println("  h          帮助");
  Serial.println("===============================\n");
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n============================");
  Serial.println(" MG4010 闭环位置模式");
  Serial.println("============================");

  pinMode(RS485_DE_RE, OUTPUT);
  rs485RxMode();
  RS485.begin(BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);

  enableMotor();
  delay(100);

  printHelp();
}

void loop() {
  handleCommand();

  // 每 500ms 读取位置
  if (millis() - lastReadTime > 500) {
    lastReadTime = millis();
    readPosition();
  }

  delay(10);
}
