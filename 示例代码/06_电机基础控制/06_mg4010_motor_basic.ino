/*
  ESP32-S3 示例 06：MG4010 电机基础控制（RS485 自定义协议）

  功能说明：
  - 通过 RS485 串口控制 MG4010 无刷电机
  - 使用自定义协议（非标准 Modbus）
  - 支持速度控制、启停控制
  - 串口监视器输入命令控制电机

  硬件连接：
  ┌─────────────┐         ┌──────────────┐         ┌─────────────┐
  │  ESP32-S3   │         │ RS485 模块   │         │  MG4010     │
  │             │         │ (MAX485/     │         │  电机驱动   │
  │             │         │  MAX3485)    │         │             │
  ├─────────────┤         ├──────────────┤         ├─────────────┤
  │ GPIO17 (TX2)├────────►│ DI (TXD)     │         │             │
  │ GPIO18 (RX2)├────────►│ RO (RXD)     │         │             │
  │ GPIO16 (DE) ├────────►│ DE           │   RS485 │  A+         │
  │ GPIO16 (RE) ├────────►│ RE           │   线缆  │  B-         │
  │             │         │ A+           ├─────────┤  GND        │
  │         GND ├─────┬──►│ B-           │  屏蔽   │             │
  │             │     │   │ GND          │  双绞线 │  DC 24V+    │
  │             │     │   └──────────────┘         │  DC 24V-    │
  └─────────────┘     │                            └─────────────┘
                      │                                   │
                   共地连接                           外部 24V 电源

  RS485 模块说明：
  - DI (Driver Input)：数据输入，连接到 ESP32 的 TX
  - RO (Receiver Output)：数据输出，连接到 ESP32 的 RX
  - DE (Driver Enable)：发送使能，高电平发送
  - RE (Receiver Enable)：接收使能，低电平接收
  - DE 和 RE 通常连在一起，由同一个 GPIO 控制

  MG4010 协议说明（自定义协议）：
  - 波特率：9600
  - 数据位：8
  - 停止位：1
  - 校验位：无

  命令格式：
  帧头  地址  功能码  数据长度  数据      校验和
  0xAA  0x01  0xXX    0xXX      [...]     CRC16

  串口监视器命令：
  - s       : 启动电机
  - p       : 停止电机
  - +       : 增加速度
  - -       : 减少速度
  - r       : 读取电机状态
  - 0-9     : 设置速度（0=停止，9=最大速度）

  注意事项：
  1. RS485 是差分信号，A+ 和 B- 不能接反
  2. 长距离通信时，A+/B- 两端需要加 120Ω 终端电阻
  3. 必须使用外部 24V 电源供电给电机驱动
  4. ESP32 和电机驱动器必须共地（GND 连接）
  5. RS485 模块的 VCC 接 3.3V 或 5V（查看模块规格）
*/

#include <HardwareSerial.h>

// ==================== 引脚定义 ====================
#define RS485_TX_PIN    17    // ESP32-S3 TX2
#define RS485_RX_PIN    18    // ESP32-S3 RX2
#define RS485_DE_RE_PIN 16    // DE/RE 控制引脚（收发切换）

// ==================== 协议定义 ====================
#define FRAME_HEADER    0xAA  // 帧头
#define MOTOR_ADDR      0x01  // 电机地址

// 功能码定义
#define CMD_START       0x01  // 启动电机
#define CMD_STOP        0x02  // 停止电机
#define CMD_SET_SPEED   0x03  // 设置速度
#define CMD_READ_STATUS 0x04  // 读取状态

// ==================== 全局变量 ====================
HardwareSerial RS485Serial(2);  // 使用 UART2
int currentSpeed = 0;            // 当前速度 (0-100)
bool motorRunning = false;       // 电机运行状态

// ==================== CRC16 校验 ====================
uint16_t calculateCRC16(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;

  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc = crc >> 1;
      }
    }
  }

  return crc;
}

// ==================== RS485 控制函数 ====================
// 设置为发送模式
void setRS485Transmit() {
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(10);  // 等待模式切换
}

// 设置为接收模式
void setRS485Receive() {
  digitalWrite(RS485_DE_RE_PIN, LOW);
  delayMicroseconds(10);  // 等待模式切换
}

// ==================== 发送命令函数 ====================
void sendMotorCommand(uint8_t cmd, uint8_t *data, uint8_t dataLen) {
  uint8_t frame[32];
  uint8_t index = 0;

  // 构建数据帧
  frame[index++] = FRAME_HEADER;    // 帧头
  frame[index++] = MOTOR_ADDR;      // 地址
  frame[index++] = cmd;             // 功能码
  frame[index++] = dataLen;         // 数据长度

  // 添加数据
  for (uint8_t i = 0; i < dataLen; i++) {
    frame[index++] = data[i];
  }

  // 计算 CRC16 校验
  uint16_t crc = calculateCRC16(frame, index);
  frame[index++] = (crc & 0xFF);        // CRC 低字节
  frame[index++] = ((crc >> 8) & 0xFF); // CRC 高字节

  // 切换到发送模式
  setRS485Transmit();

  // 发送数据
  RS485Serial.write(frame, index);
  RS485Serial.flush();  // 等待发送完成

  // 切换回接收模式
  setRS485Receive();

  // 调试输出
  Serial.print("发送命令: ");
  for (uint8_t i = 0; i < index; i++) {
    Serial.print("0x");
    if (frame[i] < 0x10) Serial.print("0");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// ==================== 电机控制函数 ====================
// 启动电机
void startMotor() {
  Serial.println(">>> 启动电机");
  uint8_t data[1] = {0x01};
  sendMotorCommand(CMD_START, data, 1);
  motorRunning = true;
}

// 停止电机
void stopMotor() {
  Serial.println(">>> 停止电机");
  uint8_t data[1] = {0x00};
  sendMotorCommand(CMD_STOP, data, 1);
  motorRunning = false;
  currentSpeed = 0;
}

// 设置速度 (0-100)
void setMotorSpeed(int speed) {
  if (speed < 0) speed = 0;
  if (speed > 100) speed = 100;

  currentSpeed = speed;
  Serial.print(">>> 设置速度: ");
  Serial.print(currentSpeed);
  Serial.println("%");

  uint8_t data[2];
  data[0] = (currentSpeed >> 8) & 0xFF;  // 高字节
  data[1] = currentSpeed & 0xFF;         // 低字节

  sendMotorCommand(CMD_SET_SPEED, data, 2);
}

// 读取电机状态
void readMotorStatus() {
  Serial.println(">>> 读取电机状态");
  sendMotorCommand(CMD_READ_STATUS, NULL, 0);

  // 等待响应
  unsigned long timeout = millis() + 500;
  while (millis() < timeout) {
    if (RS485Serial.available() >= 8) {
      uint8_t response[32];
      uint8_t len = 0;

      while (RS485Serial.available() && len < 32) {
        response[len++] = RS485Serial.read();
      }

      // 解析响应
      Serial.print("接收响应: ");
      for (uint8_t i = 0; i < len; i++) {
        Serial.print("0x");
        if (response[i] < 0x10) Serial.print("0");
        Serial.print(response[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // 这里可以添加具体的状态解析逻辑
      if (len >= 8 && response[0] == FRAME_HEADER) {
        Serial.println("状态读取成功");
      }

      return;
    }
  }

  Serial.println("状态读取超时");
}

// ==================== 串口命令处理 ====================
void handleSerialCommand() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    switch (cmd) {
      case 's':
      case 'S':
        startMotor();
        break;

      case 'p':
      case 'P':
        stopMotor();
        break;

      case '+':
        setMotorSpeed(currentSpeed + 10);
        break;

      case '-':
        setMotorSpeed(currentSpeed - 10);
        break;

      case 'r':
      case 'R':
        readMotorStatus();
        break;

      case '0':
        setMotorSpeed(0);
        break;

      case '1':
        setMotorSpeed(10);
        break;

      case '2':
        setMotorSpeed(20);
        break;

      case '3':
        setMotorSpeed(30);
        break;

      case '4':
        setMotorSpeed(40);
        break;

      case '5':
        setMotorSpeed(50);
        break;

      case '6':
        setMotorSpeed(60);
        break;

      case '7':
        setMotorSpeed(70);
        break;

      case '8':
        setMotorSpeed(80);
        break;

      case '9':
        setMotorSpeed(90);
        break;

      case 'h':
      case 'H':
      case '?':
        printHelp();
        break;

      default:
        if (cmd > 32) {  // 可打印字符
          Serial.print("未知命令: ");
          Serial.println(cmd);
          Serial.println("输入 h 查看帮助");
        }
        break;
    }
  }
}

// 打印帮助信息
void printHelp() {
  Serial.println("\n========== MG4010 电机控制命令 ==========");
  Serial.println("s      - 启动电机");
  Serial.println("p      - 停止电机");
  Serial.println("+      - 增加速度 (+10%)");
  Serial.println("-      - 减少速度 (-10%)");
  Serial.println("0-9    - 设置速度 (0=0%, 9=90%)");
  Serial.println("r      - 读取电机状态");
  Serial.println("h/?    - 显示帮助");
  Serial.println("========================================\n");
}

// ==================== 初始化 ====================
void setup() {
  // 初始化 USB 串口（用于调试和命令输入）
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("   MG4010 电机控制系统");
  Serial.println("   RS485 通信 - 自定义协议");
  Serial.println("========================================\n");

  // 初始化 DE/RE 控制引脚
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  setRS485Receive();  // 默认接收模式

  // 初始化 RS485 串口
  RS485Serial.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  Serial.println("✓ RS485 串口初始化完成");
  Serial.println("  - 波特率: 9600");
  Serial.println("  - TX: GPIO17");
  Serial.println("  - RX: GPIO18");
  Serial.println("  - DE/RE: GPIO16");

  delay(500);

  printHelp();

  Serial.println("系统就绪！等待命令...\n");
}

// ==================== 主循环 ====================
void loop() {
  handleSerialCommand();

  // 可以在这里添加周期性的状态监测
  static unsigned long lastStatusCheck = 0;
  if (motorRunning && millis() - lastStatusCheck > 5000) {
    // 每 5 秒自动读取一次状态
    // readMotorStatus();  // 可选：取消注释以启用自动状态读取
    lastStatusCheck = millis();
  }

  delay(10);
}
