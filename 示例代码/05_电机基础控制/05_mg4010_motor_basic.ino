/**
 * MG4010 伺服电机基础控制示例
 *
 * 功能：通过 RS485 串口与 MG4010 电机通信，支持读取状态、控制转矩/速度/位置等
 * 硬件连接：
 *   - RS485_TX (GPIO17) -> RS485 模块 DI
 *   - RS485_RX (GPIO18) -> RS485 模块 RO
 *   - RS485_DE (GPIO16) -> RS485 模块 DE/RE
 */

#define RS485_TX 17  // RS485 发送引脚
#define RS485_RX 18  // RS485 接收引脚
#define RS485_DE 16  // RS485 方向控制引脚
#define MOTOR_ID 0x01  // 电机 ID 地址

struct Command {
  char key;  // 命令按键
  const char* name;  // 命令名称
};

Command commands[] = {  // 命令列表数组
  {'1', "读取状态1"},
  {'2', "读取状态2"},
  {'3', "读取多圈角度"},
  {'4', "清除错误"},
  {'5', "关闭电机"},
  {'6', "停止电机"},
  {'7', "运行电机"},
  {'8', "转矩开环控制"},
  {'9', "速度开环控制"},
  {'a', "速度闭环控制"},
  {'b', "位置闭环控制1"},
  {'c', "位置闭环控制2"},
  {'d', "位置闭环控制3"},
  {'e', "位置闭环控制4"}
};

void setup() {
  Serial.begin(115200);  // 初始化 USB 串口
  Serial1.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);  // 初始化 RS485 串口
  pinMode(RS485_DE, OUTPUT);  // 设置方向控制为输出

  delay(1000);  // 等待串口稳定
  Serial.println("\n=== MG4010 电机控制系统 ===");
  printHelp();  // 显示命令帮助
}

void loop() {
  if (Serial.available()) {  // 检查是否有串口输入
    char cmd = Serial.read();  // 读取命令字符
    while (Serial.available()) Serial.read();  // 清空缓冲区

    switch (cmd) {
      case '1': readState1(); break;
      case '2': readState2(); break;
      case '3': readMultiAngle(); break;
      case '4': clearError(); break;
      case '5': motorOff(); break;
      case '6': motorStop(); break;
      case '7': motorRun(); break;
      case '8': torqueOpenLoopControl(); break;
      case '9': speedOpenLoopControl(); break;
      case 'a': speedClosedLoopControl(); break;
      case 'b': positionClosedLoopControl1(); break;
      case 'c': positionClosedLoopControl2(); break;
      case 'd': positionClosedLoopControl3(); break;
      case 'e': positionClosedLoopControl4(); break;
      default:
        Serial.println("无效命令");
        printHelp();
    }
  }
}

void printHelp() {  // 打印命令帮助菜单
  Serial.println("\n命令列表：");
  for (const auto& cmd : commands) {  // 遍历命令数组
    Serial.printf("  %c - %s\n", cmd.key, cmd.name);
  }
  Serial.println();
}

void printHex(const uint8_t* data, uint8_t len) {  // 以十六进制打印数据
  Serial.print("发送: ");
  for (uint8_t i = 0; i < len; i++) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}

uint8_t calculateChecksum(const uint8_t* data, uint8_t len) {  // 计算校验和
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];  // 累加所有字节
  }
  return sum;
}

void sendCommand(uint8_t cmd, const uint8_t* params = nullptr, uint8_t paramLen = 0) {  // 发送命令到电机
  uint8_t data[16], len = 0;

  data[len++] = 0x3E;  // 帧头固定为 0x3E
  data[len++] = cmd;  // 命令字节
  if (params) {
    for (uint8_t i = 0; i < paramLen; i++) {
      data[len++] = params[i];  // 添加参数
    }
  }
  data[len++] = calculateChecksum(data, len);  // 添加校验和

  printHex(data, len);  // 打印发送的数据

  digitalWrite(RS485_DE, HIGH);  // 切换到发送模式
  delayMicroseconds(100);  // 等待模式切换稳定
  Serial1.write(data, len);  // 发送数据
  Serial1.flush();  // 等待发送完成
  delayMicroseconds(100);
  digitalWrite(RS485_DE, LOW);  // 切换到接收模式
}

uint16_t bytesToU16(uint8_t low, uint8_t high) {  // 两字节转 16 位无符号整数
  return ((uint16_t)high << 8) | low;
}

uint64_t bytesToU64(const uint8_t* bytes) {  // 字节数组转 64 位无符号整数
  uint64_t result = 0;
  for (int i = 7; i >= 0; i--) {
    result = (result << 8) | bytes[i];  // 小端序拼接
  }
  return result;
}

bool waitResponse(uint8_t* buffer, uint8_t expectedLen, uint16_t timeout = 500) {  // 等待电机响应
  unsigned long start = millis();
  uint8_t index = 0;

  while (millis() - start < timeout) {  // 在超时时间内等待
    if (Serial1.available()) {
      buffer[index++] = Serial1.read();  // 读取响应字节
      if (index >= expectedLen) {  // 接收完整数据包
        Serial.print("接收: ");
        for (uint8_t i = 0; i < expectedLen; i++) {
          Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();

        uint8_t checksum = calculateChecksum(buffer, expectedLen - 1);  // 计算校验和
        if (buffer[expectedLen - 1] == checksum) {  // 校验通过
          return true;
        } else {
          Serial.printf("校验失败 (期望: %02X, 实际: %02X)\n", checksum, buffer[expectedLen - 1]);
          return false;
        }
      }
    }
  }
  Serial.println("响应超时");
  return false;
}

void readState1() {  // 读取电机状态1（温度、电压、电流）
  Serial.println("\n--- 读取状态1 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x9A, params, 1);  // 发送读状态1命令

  uint8_t response[9];
  if (waitResponse(response, 9)) {
    int8_t temp = (int8_t)response[2];  // 温度（有符号）
    uint16_t voltage = bytesToU16(response[4], response[3]);  // 电压（0.1V 单位）
    int16_t current = (int16_t)bytesToU16(response[6], response[5]);  // 电流（0.01A 单位）

    Serial.printf("温度: %d°C\n", temp);
    Serial.printf("电压: %.1fV\n", voltage / 10.0);
    Serial.printf("电流: %.2fA\n", current / 100.0);
    Serial.printf("错误码: 0x%02X\n", response[7]);
  }
}

void readState2() {  // 读取电机状态2（温度、电流、速度、编码器）
  Serial.println("\n--- 读取状态2 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x9C, params, 1);  // 发送读状态2命令

  uint8_t response[14];
  if (waitResponse(response, 14)) {
    int8_t temp = (int8_t)response[2];  // 温度（有符号）
    int16_t current = (int16_t)bytesToU16(response[4], response[3]);  // 电流（0.01A 单位）
    int16_t speed = (int16_t)bytesToU16(response[6], response[5]);  // 速度（dps 单位）
    uint16_t encoder = bytesToU16(response[8], response[7]);  // 编码器原始值

    Serial.printf("温度: %d°C\n", temp);
    Serial.printf("电流: %.2fA\n", current / 100.0);
    Serial.printf("速度: %d dps\n", speed);
    Serial.printf("编码器: %d\n", encoder);
  }
}

void readMultiAngle() {  // 读取多圈累计角度
  Serial.println("\n--- 读取多圈角度 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x92, params, 1);  // 发送读多圈角度命令

  uint8_t response[14];
  if (waitResponse(response, 14)) {
    int64_t angle = (int64_t)bytesToU64(&response[2]);  // 多圈角度（0.01度单位）

    Serial.printf("多圈角度: %lld\n", angle);
    Serial.printf("转数: %.2f圈\n", angle / 36000.0);  // 转换为圈数
  }
}

void clearError() {  // 清除电机错误状态
  Serial.println("\n--- 清除错误 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x9B, params, 1);  // 发送清除错误命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.println("错误已清除");
  }
}

void motorOff() {  // 关闭电机（断电）
  Serial.println("\n--- 关闭电机 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x80, params, 1);  // 发送关闭命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.println("电机已关闭");
  }
}

void motorStop() {  // 停止电机（保持通电）
  Serial.println("\n--- 停止电机 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x81, params, 1);  // 发送停止命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.println("电机已停止");
  }
}

void motorRun() {  // 运行电机
  Serial.println("\n--- 运行电机 ---");
  uint8_t params[] = {MOTOR_ID};
  sendCommand(0x88, params, 1);  // 发送运行命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.println("电机已运行");
  }
}

void torqueOpenLoopControl() {  // 转矩开环控制
  Serial.println("\n--- 转矩开环控制 ---");
  int16_t torque = 500;  // 转矩值（0.01A 单位）
  uint8_t params[] = {MOTOR_ID, (uint8_t)(torque & 0xFF), (uint8_t)(torque >> 8)};  // 小端序
  sendCommand(0xA1, params, 3);  // 发送转矩控制命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置转矩: %.2fA\n", torque / 100.0);
  }
}

void speedOpenLoopControl() {  // 速度开环控制
  Serial.println("\n--- 速度开环控制 ---");
  int16_t speed = 360;  // 速度值（dps 单位）
  uint8_t params[] = {MOTOR_ID, (uint8_t)(speed & 0xFF), (uint8_t)(speed >> 8)};  // 小端序
  sendCommand(0xA2, params, 3);  // 发送速度控制命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置速度: %d dps\n", speed);
  }
}

void speedClosedLoopControl() {  // 速度闭环控制
  Serial.println("\n--- 速度闭环控制 ---");
  int32_t speed = 18000;  // 速度值（0.01dps 单位）
  uint8_t params[] = {  // 32 位小端序
    MOTOR_ID,
    (uint8_t)(speed & 0xFF),
    (uint8_t)((speed >> 8) & 0xFF),
    (uint8_t)((speed >> 16) & 0xFF),
    (uint8_t)((speed >> 24) & 0xFF)
  };
  sendCommand(0xA6, params, 5);  // 发送闭环速度命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置速度: %.2f dps\n", speed / 100.0);
  }
}

void positionClosedLoopControl1() {  // 位置闭环控制1（仅角度）
  Serial.println("\n--- 位置闭环控制1 ---");
  int32_t angle = 36000;  // 角度值（0.01度单位）
  uint8_t params[] = {  // 32 位小端序
    MOTOR_ID,
    (uint8_t)(angle & 0xFF),
    (uint8_t)((angle >> 8) & 0xFF),
    (uint8_t)((angle >> 16) & 0xFF),
    (uint8_t)((angle >> 24) & 0xFF)
  };
  sendCommand(0xA3, params, 5);  // 发送位置控制命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置角度: %.2f度\n", angle / 100.0);
  }
}

void positionClosedLoopControl2() {  // 位置闭环控制2（角度+速度）
  Serial.println("\n--- 位置闭环控制2 ---");
  int32_t angle = 36000;  // 角度值（0.01度单位）
  uint16_t speed = 500;  // 速度限制（dps 单位）
  uint8_t params[] = {  // 角度 32 位 + 速度 16 位
    MOTOR_ID,
    (uint8_t)(angle & 0xFF),
    (uint8_t)((angle >> 8) & 0xFF),
    (uint8_t)((angle >> 16) & 0xFF),
    (uint8_t)((angle >> 24) & 0xFF),
    (uint8_t)(speed & 0xFF),
    (uint8_t)(speed >> 8)
  };
  sendCommand(0xA4, params, 7);  // 发送带速度的位置命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置角度: %.2f度, 速度: %d dps\n", angle / 100.0, speed);
  }
}

void positionClosedLoopControl3() {  // 位置闭环控制3（角度+方向+速度）
  Serial.println("\n--- 位置闭环控制3 ---");
  uint16_t angle = 360;  // 角度值（0.01度单位）
  uint8_t direction = 0x00;  // 方向（0顺时针，1逆时针）
  uint16_t speed = 500;  // 速度限制（dps 单位）
  uint8_t params[] = {
    MOTOR_ID,
    (uint8_t)(angle & 0xFF),
    (uint8_t)(angle >> 8),
    direction,
    (uint8_t)(speed & 0xFF),
    (uint8_t)(speed >> 8)
  };
  sendCommand(0xA7, params, 6);  // 发送带方向的位置命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置角度: %.2f度, 方向: %s, 速度: %d dps\n",
                  angle / 100.0, direction ? "逆时针" : "顺时针", speed);
  }
}

void positionClosedLoopControl4() {  // 位置闭环控制4（增量角度+速度）
  Serial.println("\n--- 位置闭环控制4 ---");
  uint16_t angle = 360;  // 增量角度（0.01度单位）
  uint16_t speed = 500;  // 速度限制（dps 单位）
  uint8_t params[] = {
    MOTOR_ID,
    (uint8_t)(angle & 0xFF),
    (uint8_t)(angle >> 8),
    (uint8_t)(speed & 0xFF),
    (uint8_t)(speed >> 8)
  };
  sendCommand(0xA8, params, 5);  // 发送增量位置命令

  uint8_t response[4];
  if (waitResponse(response, 4)) {
    Serial.printf("设置角度: %.2f度, 速度: %d dps\n", angle / 100.0, speed);
  }
}
