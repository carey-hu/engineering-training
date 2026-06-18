/*
  ESP32-S3 示例 06：MG4010 电机基础控制（Modbus-RTU）

  功能：
  - RS485 总线 + Modbus-RTU 协议通信
  - 电机使能 / 失能
  - 零点设置
  - 电机状态读取
  - 串口监视器输入命令控制

  硬件连接（外接 RS485 模块，如 MAX485 / MAX3485）：
  ┌──────────────┐         ┌──────────────┐         ┌─────────────┐
  │  ESP32-S3    │         │  RS485 模块  │         │  MG4010     │
  │              │         │              │         │  电机驱动器  │
  ├──────────────┤         ├──────────────┤         ├─────────────┤
  │ GPIO17 (TX2) ├────────►│ DI           │         │             │
  │ GPIO18 (RX2) ├────────►│ RO           │         │             │
  │ GPIO16       ├────────►│ DE + RE      │         │             │
  │              │         │ A+           ├─────────┤ A+          │
  │              │         │ B-           ├─────────┤ B-          │
  │ GND          ├────────►│ GND          │         │ GND         │
  └──────────────┘         └──────────────┘         │ DC 24V+     │
                                                     │ DC 24V-     │
                                                     └─────────────┘
  注意：MG4010 电机需要外部 24V 电源供电。
        ESP32 和电机驱动器必须共地（GND 连接在一起）。

  Modbus-RTU 协议：
  - 波特率 9600，8 数据位，1 停止位，无校验（8N1）
  - 从站地址 1（默认）
  - 帧格式：地址 功能码 数据 CRC16(低字节在前)

  串口命令：
  - e    电机使能
  - d    电机失能
  - z    设置零点
  - s    读取状态
  - h    显示帮助
*/

#include <HardwareSerial.h>

// ==================== 引脚定义 ====================
#define RS485_TX      17    // 接 RS485 模块 DI
#define RS485_RX      18    // 接 RS485 模块 RO
#define RS485_DE_RE   16    // 接 RS485 模块 DE+RE（连在一起）

// ==================== 协议定义 ====================
#define SLAVE_ID      0x01  // 电机 Modbus 从站地址（默认 1）
#define BAUD_RATE     9600  // 波特率

// Modbus 功能码
#define FUNC_READ     0x03  // 读保持寄存器
#define FUNC_WRITE    0x06  // 写单个寄存器

// 寄存器地址（参照 MG4010-i10-R485 协议文档）
#define REG_CONTROL   0x0001 // 控制字
#define REG_SPEED     0x0002 // 目标速度（RPM）
#define REG_POS_HI    0x0003 // 目标位置高 16 位（0.01°）
#define REG_POS_LO    0x0004 // 目标位置低 16 位（0.01°）

// 控制字定义
#define CTRL_STOP     0x0000 // 停机
#define CTRL_RUN      0x0001 // 运行

// ==================== 全局变量 ====================
HardwareSerial RS485(2);       // UART2
bool motorRunning = false;

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

// ==================== RS485 收发切换 ====================
void rs485TxMode() {
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(20);
}

void rs485RxMode() {
  digitalWrite(RS485_DE_RE, LOW);
  delayMicroseconds(20);
}

// ==================== Modbus 帧发送 ====================
void sendModbus(uint8_t *frame, int len) {
  rs485TxMode();
  RS485.write(frame, len);
  RS485.flush();
  rs485RxMode();
}

// 写单个寄存器（功能码 0x06）
void writeRegister(uint16_t reg, uint16_t value) {
  uint8_t frame[8];
  frame[0] = SLAVE_ID;
  frame[1] = FUNC_WRITE;
  frame[2] = (reg >> 8) & 0xFF;     // 寄存器地址高字节
  frame[3] = reg & 0xFF;            // 寄存器地址低字节
  frame[4] = (value >> 8) & 0xFF;   // 值高字节
  frame[5] = value & 0xFF;          // 值低字节

  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF;            // CRC 低字节在前
  frame[7] = (crc >> 8) & 0xFF;

  sendModbus(frame, 8);

  // 调试输出
  Serial.print("发送: ");
  for (int i = 0; i < 8; i++) {
    if (frame[i] < 0x10) Serial.print("0");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// 读寄存器（功能码 0x03），返回接收到的数据字节数
int readRegisters(uint16_t startAddr, uint16_t count) {
  uint8_t frame[8];
  frame[0] = SLAVE_ID;
  frame[1] = FUNC_READ;
  frame[2] = (startAddr >> 8) & 0xFF;
  frame[3] = startAddr & 0xFF;
  frame[4] = (count >> 8) & 0xFF;
  frame[5] = count & 0xFF;

  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;

  sendModbus(frame, 8);

  // 等待响应
  unsigned long timeout = millis() + 200;
  while (millis() < timeout) {
    if (RS485.available()) {
      int len = 0;
      uint8_t rx[64];
      while (RS485.available() && len < 64) {
        rx[len++] = RS485.read();
      }

      // 打印响应
      Serial.print("响应: ");
      for (int i = 0; i < len; i++) {
        if (rx[i] < 0x10) Serial.print("0");
        Serial.print(rx[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      return len;
    }
  }
  Serial.println("无响应（超时）");
  return 0;
}

// ==================== 电机操作 ====================
void motorEnable() {
  Serial.println(">>> 电机使能");
  writeRegister(REG_CONTROL, CTRL_RUN);
  motorRunning = true;
}

void motorDisable() {
  Serial.println(">>> 电机失能");
  writeRegister(REG_CONTROL, CTRL_STOP);
  motorRunning = false;
}

void motorSetZero() {
  Serial.println(">>> 设置零点");
  writeRegister(REG_CONTROL, 0x0002);
}

void motorReadStatus() {
  Serial.println(">>> 读取状态");
  readRegisters(REG_CONTROL, 4);
}

// ==================== 串口命令处理 ====================
void handleCommand() {
  if (!Serial.available()) return;

  char cmd = Serial.read();
  switch (cmd) {
    case 'e': motorEnable();              break;
    case 'd': motorDisable();             break;
    case 'z': motorSetZero();             break;
    case 's': motorReadStatus();          break;
    case 'h': printHelp();                break;
    default:
      if (cmd > 32 && cmd != '\r' && cmd != '\n') {
        Serial.printf("未知命令: %c，输入 h 查看帮助\n", cmd);
      }
      break;
  }
}

void printHelp() {
  Serial.println("\n===== MG4010 电机基础控制 =====");
  Serial.println("  e  - 电机使能");
  Serial.println("  d  - 电机失能");
  Serial.println("  z  - 设置零点");
  Serial.println("  s  - 读取状态");
  Serial.println("  h  - 显示帮助");
  Serial.println("===============================\n");
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n============================");
  Serial.println(" MG4010 电机基础控制");
  Serial.println(" Modbus-RTU @ RS485");
  Serial.println("============================");

  pinMode(RS485_DE_RE, OUTPUT);
  rs485RxMode();

  RS485.begin(BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);

  Serial.printf("RS485 初始化: GPIO%d(TX) GPIO%d(RX) GPIO%d(DE/RE) @ %d\n",
                RS485_TX, RS485_RX, RS485_DE_RE, BAUD_RATE);
  printHelp();
}

void loop() {
  handleCommand();
  delay(10);
}
