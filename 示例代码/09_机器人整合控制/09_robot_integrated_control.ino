/*
 * 两电机四舵机机器人整合控制示例
 *
 * 硬件配置：
 * - 两个 MG4010-i10 RS485 电机（ID 1 和 ID 2）
 * - 四个 PWM 舵机（GPIO10/11/12/13）
 * - ESP32-S3 开发板
 *
 * 接线说明：
 * ⚠️ 警告：舵机和电机必须使用独立电源，不能从 ESP32 取电！
 *
 * RS485 连接（课程转接板）：
 *   ESP32 GPIO17 → 转接板 TTL-485 调试口 T
 *   ESP32 GPIO18 → 转接板 TTL-485 调试口 R
 *   ESP32 GND    → 转接板 GND（必须共地）
 *
 * 舵机连接：
 *   舵机1信号线 → GPIO10
 *   舵机2信号线 → GPIO11
 *   舵机3信号线 → GPIO12
 *   舵机4信号线 → GPIO13
 *   所有舵机 GND → 独立 5V 电源 GND → ESP32 GND（共地）
 *   所有舵机 VCC → 独立 5V 电源 +5V（不接 ESP32）
 *
 * 电机连接：
 *   通过课程转接板连接，详见教程文档/09_robot_integration.md
 *
 * 串口命令格式（115200 波特率）：
 *   v <左速度> <右速度>    设置电机速度（RPM，范围 -60 到 60）
 *   pose <角度1> <角度2> <角度3> <角度4>  设置四舵机角度（0-180°）
 *   home                   所有舵机回中位（90°），电机停止
 *   stop                   紧急停止所有运动
 *
 * 示例：
 *   v 20 20        两电机都以 20 RPM 前进
 *   v 20 -20       左电机前进，右电机后退（原地转向）
 *   pose 45 90 135 0   设置四个舵机角度
 *   home           回到初始姿态
 *   stop           紧急停止
 */

#include <Arduino.h>

// ========== 引脚定义 ==========
const int RS485_TX = 17;        // RS485 发送引脚
const int RS485_RX = 18;        // RS485 接收引脚
const int RS485_DE = -1;        // 课程转接板不使用 DE/RE 控制，设为 -1

const int SERVO_PINS[4] = {10, 11, 12, 13}; // 四个舵机引脚

// ========== MG4010 协议常量 ==========
const uint8_t FRAME_HEADER = 0x3E;     // 帧头（MG4010 协议）
const uint8_t CMD_MOTOR_RUN = 0x88;    // 运行命令
const uint8_t CMD_MOTOR_STOP = 0x81;   // 停止命令
const uint8_t CMD_SPEED_MODE = 0xF6;   // 速度模式命令
const uint8_t CMD_READ_STATE2 = 0x74;  // 读取状态2命令

const uint8_t MOTOR_ID_LEFT = 1;       // 左电机 ID
const uint8_t MOTOR_ID_RIGHT = 2;      // 右电机 ID

const float REDUCTION = 10.0f;         // 减速比 1:10
const float RPM_TO_DPS = 6.0f;         // RPM 转 dps（度每秒）

// ========== PWM 配置 ==========
const int PWM_FREQ = 50;               // 舵机 PWM 频率 50Hz
const int PWM_RESOLUTION = 13;         // 13位分辨率
const int PWM_MIN = 205;               // 0.5ms → 0°
const int PWM_MAX = 1024;              // 2.5ms → 180°

// ========== 安全限制 ==========
const float MAX_MOTOR_RPM = 60.0f;     // 输出轴最大转速（安全限制）
const float MIN_MOTOR_RPM = -60.0f;    // 输出轴最小转速

// ========== 全局变量 ==========
uint16_t servoTargets[4] = {512, 512, 512, 512}; // 舵机目标 PWM 值（初始 90°）
unsigned long lastServoUpdate = 0;
const unsigned long servoUpdateInterval = 20; // 舵机更新间隔 20ms

// ========== RS485 通信函数 ==========

// 计算校验和（累加和，取低8位）
uint8_t calculateChecksum(const uint8_t* data, int length) {
  uint16_t sum = 0;
  for (int i = 0; i < length; i++) {
    sum += data[i];
  }
  return (uint8_t)(sum & 0xFF);
}

// 发送命令到指定电机
// 协议格式：0x3E + cmd + id + dataLen + [data] + checksum
void sendMotorCommand(uint8_t motorID, uint8_t cmd, const uint8_t* params = nullptr, uint8_t paramLen = 0) {
  uint8_t frame[16];
  int idx = 0;

  frame[idx++] = FRAME_HEADER;  // 帧头
  frame[idx++] = cmd;            // 命令
  frame[idx++] = motorID;        // 电机 ID
  frame[idx++] = paramLen;       // 数据长度

  // 添加参数数据
  if (params != nullptr && paramLen > 0) {
    for (int i = 0; i < paramLen; i++) {
      frame[idx++] = params[i];
    }
  }

  // 计算校验和（不包含帧头，包含 cmd + id + dataLen + data）
  frame[idx] = calculateChecksum(&frame[1], idx - 1);
  idx++;

  // 发送帧
  Serial1.write(frame, idx);
  Serial1.flush();
}

// 设置电机速度（RPM）
void setMotorSpeed(uint8_t motorID, float rpm) {
  // 安全限制
  rpm = constrain(rpm, MIN_MOTOR_RPM, MAX_MOTOR_RPM);

  // 单位换算：输出轴 RPM → 电机侧 dps → 协议值（0.01 dps/LSB）
  // 电机侧速度 = 输出轴速度 × 减速比
  // 协议值 = 电机侧速度（dps）× 100
  float motorDps = rpm * RPM_TO_DPS * REDUCTION;  // 电机侧 dps
  int32_t speedValue = (int32_t)(motorDps * 100.0f); // 协议值

  // 构造参数：4字节小端序
  uint8_t params[4];
  params[0] = (speedValue) & 0xFF;
  params[1] = (speedValue >> 8) & 0xFF;
  params[2] = (speedValue >> 16) & 0xFF;
  params[3] = (speedValue >> 24) & 0xFF;

  // 先发送运行命令
  sendMotorCommand(motorID, CMD_MOTOR_RUN);
  delay(10);

  // 再发送速度命令
  sendMotorCommand(motorID, CMD_SPEED_MODE, params, 4);
}

// 停止电机
void stopMotor(uint8_t motorID) {
  sendMotorCommand(motorID, CMD_MOTOR_STOP);
}

// 停止所有电机
void stopAllMotors() {
  stopMotor(MOTOR_ID_LEFT);
  delay(5);
  stopMotor(MOTOR_ID_RIGHT);
}

// ========== 舵机控制函数 ==========

// 初始化所有舵机
void initServos() {
  for (int i = 0; i < 4; i++) {
    ledcAttach(SERVO_PINS[i], PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(SERVO_PINS[i], servoTargets[i]);
  }
}

// 更新所有舵机 PWM 输出（非阻塞）
void updateServos() {
  unsigned long now = millis();
  if (now - lastServoUpdate >= servoUpdateInterval) {
    lastServoUpdate = now;
    for (int i = 0; i < 4; i++) {
      ledcWrite(SERVO_PINS[i], servoTargets[i]);
    }
  }
}

// 角度转 PWM 占空比
uint16_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, PWM_MIN, PWM_MAX);
}

// 设置单个舵机角度
void setServoAngle(int servoID, int angle) {
  if (servoID >= 1 && servoID <= 4) {
    servoTargets[servoID - 1] = angleToDuty(angle);
  }
}

// 设置所有舵机角度
void setAllServoAngles(int angle1, int angle2, int angle3, int angle4) {
  servoTargets[0] = angleToDuty(angle1);
  servoTargets[1] = angleToDuty(angle2);
  servoTargets[2] = angleToDuty(angle3);
  servoTargets[3] = angleToDuty(angle4);
}

// 回中位（所有舵机 90°）
void homePosition() {
  setAllServoAngles(90, 90, 90, 90);
  stopAllMotors();
  Serial.println("回到初始姿态：舵机 90°，电机停止");
}

// ========== 串口命令解析 ==========

void processCommand() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();

  if (input.length() == 0) return;

  Serial.print("收到命令: ");
  Serial.println(input);

  // 解析命令
  int spaceIndex = input.indexOf(' ');
  String cmd = (spaceIndex > 0) ? input.substring(0, spaceIndex) : input;
  String args = (spaceIndex > 0) ? input.substring(spaceIndex + 1) : "";

  cmd.toLowerCase();

  if (cmd == "v") {
    // v <左速度> <右速度>
    int space2 = args.indexOf(' ');
    if (space2 > 0) {
      float leftRPM = args.substring(0, space2).toFloat();
      float rightRPM = args.substring(space2 + 1).toFloat();

      setMotorSpeed(MOTOR_ID_LEFT, leftRPM);
      delay(10);
      setMotorSpeed(MOTOR_ID_RIGHT, rightRPM);

      Serial.printf("设置速度：左 %.1f RPM，右 %.1f RPM\n", leftRPM, rightRPM);
    } else {
      Serial.println("错误：需要两个速度参数，如 v 20 20");
    }
  }
  else if (cmd == "pose") {
    // pose <角度1> <角度2> <角度3> <角度4>
    int angles[4];
    int count = 0;
    int lastIndex = 0;

    for (int i = 0; i < 4; i++) {
      int nextSpace = args.indexOf(' ', lastIndex);
      String angleStr;

      if (i < 3) {
        if (nextSpace > lastIndex) {
          angleStr = args.substring(lastIndex, nextSpace);
          lastIndex = nextSpace + 1;
        } else {
          break;
        }
      } else {
        angleStr = args.substring(lastIndex);
      }

      angles[count++] = angleStr.toInt();
    }

    if (count == 4) {
      setAllServoAngles(angles[0], angles[1], angles[2], angles[3]);
      Serial.printf("设置舵机角度：%d° %d° %d° %d°\n",
                    angles[0], angles[1], angles[2], angles[3]);
    } else {
      Serial.println("错误：需要四个角度参数，如 pose 45 90 135 0");
    }
  }
  else if (cmd == "home") {
    // 回到初始姿态
    homePosition();
  }
  else if (cmd == "stop") {
    // 紧急停止
    stopAllMotors();
    Serial.println("紧急停止：所有电机已停止");
  }
  else {
    Serial.println("未知命令。可用命令：");
    Serial.println("  v <左速度> <右速度>    设置电机速度（RPM）");
    Serial.println("  pose <角1> <角2> <角3> <角4>  设置舵机角度");
    Serial.println("  home                   回到初始姿态");
    Serial.println("  stop                   紧急停止");
  }
}

// ========== 主程序 ==========

void setup() {
  // 初始化 USB 串口（调试用）
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========== 两电机四舵机机器人整合控制 ==========");
  Serial.println("⚠️  上电前确认：");
  Serial.println("   1. 电机和舵机使用独立电源（不从 ESP32 取电）");
  Serial.println("   2. 所有 GND 已连接（ESP32、电机电源、舵机电源、转接板）");
  Serial.println("   3. 两个电机 ID 分别为 1 和 2");
  Serial.println("   4. 首次测试使用低速（如 v 10 10）");
  Serial.println("=====================================================\n");

  // 初始化 RS485 串口
  Serial1.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);

  // 初始化舵机
  initServos();
  Serial.println("✓ 舵机初始化完成（90° 中位）");

  // 设置串口超时（避免阻塞）
  Serial.setTimeout(50);

  Serial.println("\n可用命令：");
  Serial.println("  v <左速度> <右速度>    设置电机速度（RPM，-60 到 60）");
  Serial.println("  pose <角1> <角2> <角3> <角4>  设置舵机角度（0-180°）");
  Serial.println("  home                   回到初始姿态");
  Serial.println("  stop                   紧急停止");
  Serial.println("\n等待命令...\n");
}

void loop() {
  // 处理串口命令
  processCommand();

  // 非阻塞更新舵机
  updateServos();

  // 主循环保持快速，不使用 delay
}
