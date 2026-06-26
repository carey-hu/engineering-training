/*
 * 机器人整合控制示例 - 驱动麦克纳姆轮底盘和多自由度机械臂
 *
 * 硬件连接：
 * - RS485：TX=GPIO17, RX=GPIO18, DE/RE=GPIO16
 * - 四个机械臂舵机分别连接到 GPIO_SERVO1-4
 * - WS2812B LED 连接到 GPIO48（板载）
 *
 * 串口指令格式：
 * 底盘控制：
 *   1 <Vx> <Vy> <Vw>      // 设置底盘速度（-100 到 100）
 *   2                      // 停止底盘运动
 *
 * 舵机控制：
 *   3 <id> <angle>         // 单独控制舵机（id: 1-4, angle: 0-180°）
 *   4 <a1> <a2> <a3> <a4>  // 同时控制四个舵机角度
 *
 * LED控制：
 *   5 <r> <g> <b>          // 设置 LED 颜色（0-255）
 *
 * 示例：
 *   1 50 0 0       // 底盘向前运动
 *   3 1 90         // 第一个舵机转到 90 度
 *   4 90 45 135 0  // 设置所有舵机角度
 *   5 255 0 0      // LED 设为红色
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ========== 引脚定义 ==========
#define RS485_TX 17        // RS485 发送引脚
#define RS485_RX 18        // RS485 接收引脚
#define RS485_DE 16        // RS485 收发控制引脚

#define GPIO_SERVO1 14     // 舵机1控制引脚
#define GPIO_SERVO2 13     // 舵机2控制引脚
#define GPIO_SERVO3 12     // 舵机3控制引脚
#define GPIO_SERVO4 11     // 舵机4控制引脚

#define WS2812B_PIN 48     // 板载LED引脚
#define WS2812B_COUNT 1    // LED数量

// ========== 麦克纳姆轮运动学参数 ==========
#define WHEEL_RADIUS 0.05f // 轮子半径（米）
#define LX 0.15f           // 车体半长（米）
#define LY 0.10f           // 车体半宽（米）

// ========== RS485 协议常量 ==========
#define FRAME_HEADER 0xAA  // 帧头标志
#define FRAME_FOOTER 0x55  // 帧尾标志
#define FRAME_LENGTH 8     // 帧长度
#define MOTOR_COUNT 4      // 电机数量

// ========== PWM 配置 ==========
#define LEDC_TIMER_BIT 13  // PWM分辨率
#define LEDC_BASE_FREQ 50  // PWM频率50Hz
#define PWM_MIN_DUTY 1638  // 0.5ms对应0度
#define PWM_MAX_DUTY 8192  // 2.5ms对应180度

// ========== 全局对象 ==========
Adafruit_NeoPixel strip(WS2812B_COUNT, WS2812B_PIN, NEO_GRB + NEO_KHZ800); // LED对象

// ========== 状态数组 ==========
uint8_t motorIDs[MOTOR_COUNT] = {1, 2, 3, 4};           // 电机ID数组
int8_t motorSpeeds[MOTOR_COUNT] = {0, 0, 0, 0};         // 电机速度数组
uint16_t servoTargets[4] = {1500, 1500, 1500, 1500};    // 舵机目标值数组

// ========== RS485 控制 ==========
inline void rs485ControlMode(bool transmit) {            // 切换RS485收发模式
  digitalWrite(RS485_DE, transmit ? HIGH : LOW);         // HIGH为发送，LOW为接收
  if (!transmit) delayMicroseconds(100);                 // 接收模式等待稳定
}

// ========== 舵机控制 ==========
void initServos() {                                      // 初始化所有舵机
  uint8_t pins[] = {GPIO_SERVO1, GPIO_SERVO2, GPIO_SERVO3, GPIO_SERVO4};
  for (uint8_t i = 0; i < 4; i++) {
    ledcAttach(pins[i], LEDC_BASE_FREQ, LEDC_TIMER_BIT); // 配置PWM（新版API）
    ledcWrite(pins[i], servoTargets[i]);                  // 写入初始值
  }
}

inline void updateServos() {                             // 更新所有舵机PWM输出
  uint8_t pins[] = {GPIO_SERVO1, GPIO_SERVO2, GPIO_SERVO3, GPIO_SERVO4};
  for (uint8_t i = 0; i < 4; i++) ledcWrite(pins[i], servoTargets[i]);
}

inline void waitAndUpdateServos() {                      // 更新舵机并等待
  updateServos();
  delayMicroseconds(100);                                // 短暂延时确保稳定
}

void setServoAngle(uint8_t id, uint16_t angle) {         // 设置单个舵机角度
  if (id >= 1 && id <= 4) {
    servoTargets[id - 1] = map(constrain(angle, 0, 180), 0, 180, PWM_MIN_DUTY, PWM_MAX_DUTY); // 角度转PWM占空比
    updateServos();
  }
}

void setAllServoAngles(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4) { // 设置所有舵机角度
  uint16_t angles[] = {a1, a2, a3, a4};
  for (uint8_t i = 0; i < 4; i++) {
    servoTargets[i] = map(constrain(angles[i], 0, 180), 0, 180, PWM_MIN_DUTY, PWM_MAX_DUTY); // 批量转换
  }
  updateServos();
}

// ========== LED 控制 ==========
void setLED(uint8_t r, uint8_t g, uint8_t b) {           // 设置LED颜色
  strip.setPixelColor(0, strip.Color(r, g, b));          // 设置RGB值
  strip.show();                                           // 刷新显示
}

// ========== 数据打包 ==========
uint8_t calculateCRC(const uint8_t* data, size_t len) {  // 计算校验和
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) crc += data[i];       // 累加所有字节
  return crc;
}

// ========== RS485 通信 ==========
void motorCommand(uint8_t id, int8_t speed) {            // 发送电机控制指令
  speed = constrain(speed, -100, 100);                   // 限制速度范围

  uint8_t frame[FRAME_LENGTH];                           // 创建数据帧
  frame[0] = FRAME_HEADER;                               // 设置帧头
  frame[1] = id;                                          // 设置电机ID

  uint8_t mode = (speed == 0) ? 0x00 : 0x01;            // 0为停止，1为运行
  frame[2] = mode;                                        // 设置运行模式

  int16_t speedValue = (mode == 0x00) ? 0 : speed;      // 停止时速度清零
  memcpy(&frame[3], &speedValue, sizeof(int16_t));      // 拷贝速度值

  frame[5] = 0x00;                                        // 保留字节
  frame[6] = calculateCRC(frame, 6);                     // 计算校验码
  frame[7] = FRAME_FOOTER;                               // 设置帧尾

  rs485ControlMode(true);                                // 切换到发送模式
  Serial1.write(frame, FRAME_LENGTH);                    // 发送数据帧
  Serial1.flush();                                        // 等待发送完成
  rs485ControlMode(false);                               // 切换回接收模式

  delay(10);                                              // 指令间隔延时
}

// ========== 麦克纳姆轮运动学 ==========
void mecanumKinematics(float Vx, float Vy, float Vw, float* wheelSpeeds) { // 逆运动学计算
  wheelSpeeds[0] = (Vx - Vy - (LX + LY) * Vw) / WHEEL_RADIUS; // 左前轮速度
  wheelSpeeds[1] = (Vx + Vy + (LX + LY) * Vw) / WHEEL_RADIUS; // 右前轮速度
  wheelSpeeds[2] = (Vx + Vy - (LX + LY) * Vw) / WHEEL_RADIUS; // 左后轮速度
  wheelSpeeds[3] = (Vx - Vy + (LX + LY) * Vw) / WHEEL_RADIUS; // 右后轮速度
}

void normalizeWheelSpeeds(float* wheelSpeeds, float maxSpeed) { // 归一化轮速
  float maxVal = 0.0f;
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    float absVal = fabs(wheelSpeeds[i]);                 // 取绝对值
    if (absVal > maxVal) maxVal = absVal;                // 找最大值
  }

  if (maxVal > maxSpeed) {                               // 超出限制需缩放
    float scale = maxSpeed / maxVal;                     // 计算缩放比例
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) wheelSpeeds[i] *= scale; // 等比缩放
  }
}

void setMotorOutputSpeed(float speed, uint8_t motorIndex) { // 设置电机输出速度
  motorSpeeds[motorIndex] = constrain((int8_t)round(speed), -100, 100); // 四舍五入并限幅
}

void setChassisVelocity(float Vx, float Vy, float Vw) { // 设置底盘速度
  float wheelSpeeds[MOTOR_COUNT];                        // 创建轮速数组
  mecanumKinematics(Vx, Vy, Vw, wheelSpeeds);           // 计算各轮速度
  normalizeWheelSpeeds(wheelSpeeds, 100.0f);            // 归一化到-100~100

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    setMotorOutputSpeed(wheelSpeeds[i], i);              // 设置电机速度
  }

  Serial.printf("设定底盘速度 Vx=%.2f, Vy=%.2f, Vw=%.2f -> 轮速=[%d, %d, %d, %d]\n",
                Vx, Vy, Vw, motorSpeeds[0], motorSpeeds[1], motorSpeeds[2], motorSpeeds[3]);

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    motorCommand(motorIDs[i], motorSpeeds[i]);           // 发送速度指令
    waitAndUpdateServos();                               // 更新舵机并等待
  }
}

void stopChassis() {                                     // 停止底盘运动
  Serial.println("停止底盘运动");
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    motorSpeeds[i] = 0;                                  // 清零速度
    motorCommand(motorIDs[i], 0);                        // 发送停止指令
    waitAndUpdateServos();                               // 更新舵机并等待
  }
}

// ========== 串口指令解析 ==========
void processSerialCommand() {                            // 处理串口指令
  if (!Serial.available()) return;                       // 无数据则返回

  int cmdType = Serial.parseInt();                       // 读取指令类型

  switch (cmdType) {
    case 1: {                                            // 底盘速度控制
      float Vx = Serial.parseFloat();                    // 读取X方向速度
      float Vy = Serial.parseFloat();                    // 读取Y方向速度
      float Vw = Serial.parseFloat();                    // 读取角速度
      setChassisVelocity(Vx, Vy, Vw);                   // 设置底盘速度
      break;
    }

    case 2:                                              // 停止底盘
      stopChassis();
      break;

    case 3: {                                            // 单个舵机控制
      uint8_t id = Serial.parseInt();                    // 读取舵机ID
      uint16_t angle = Serial.parseInt();                // 读取目标角度
      setServoAngle(id, angle);                          // 设置舵机角度
      Serial.printf("舵机 %d 转到 %d°\n", id, angle);
      break;
    }

    case 4: {                                            // 所有舵机控制
      uint16_t angles[4];
      for (uint8_t i = 0; i < 4; i++) angles[i] = Serial.parseInt(); // 读取四个角度
      setAllServoAngles(angles[0], angles[1], angles[2], angles[3]); // 批量设置
      Serial.printf("设置所有舵机角度: [%d°, %d°, %d°, %d°]\n",
                    angles[0], angles[1], angles[2], angles[3]);
      break;
    }

    case 5: {                                            // LED颜色控制
      uint8_t r = Serial.parseInt();                     // 读取红色分量
      uint8_t g = Serial.parseInt();                     // 读取绿色分量
      uint8_t b = Serial.parseInt();                     // 读取蓝色分量
      setLED(r, g, b);                                   // 设置LED颜色
      Serial.printf("设置 LED 颜色: RGB(%d, %d, %d)\n", r, g, b);
      break;
    }

    default:                                             // 无效指令
      Serial.println(R"(无效指令。指令格式：
1 <Vx> <Vy> <Vw>      底盘速度控制
2                      停止底盘
3 <id> <angle>         单个舵机控制
4 <a1> <a2> <a3> <a4>  所有舵机控制
5 <r> <g> <b>          LED 颜色控制)");
  }

  while (Serial.available()) Serial.read();              // 清空缓冲区
}

// ========== 系统初始化 ==========
void setup() {
  Serial.begin(115200);                                  // 初始化串口监视器
  Serial1.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX); // 初始化RS485串口

  pinMode(RS485_DE, OUTPUT);                             // 设置收发控制引脚
  rs485ControlMode(false);                               // 默认接收模式

  strip.begin();                                         // 初始化LED库
  strip.show();                                          // 熄灭LED

  initServos();                                          // 初始化舵机

  Serial.println(R"(
========================================
机器人整合控制系统已启动
- RS485 初始化完成
- 舵机初始化完成
- WS2812B LED 初始化完成
========================================
请通过串口发送控制指令)");
}

// ========== 主循环 ==========
void loop() {
  processSerialCommand();                                // 处理串口指令
  delay(10);                                             // 循环延时
}
