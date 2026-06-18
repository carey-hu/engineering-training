/*
  ESP32-S3 示例 03：基础舵机控制（串口输入角度）

  本例程重点：
  - 去掉 WiFi，纯本地控制
  - 串口输入角度（0-180）控制舵机
  - 使用非阻塞写法，舵机平滑转动
  - 注释尽量写清楚，适合初学者理解

  舵机接线：
  舵机红线 → 外部 5V+
  舵机棕/黑线 → 外部 5V-
  ESP32 GND → 外部 5V- （共地！！）
  舵机黄/橙线 → ESP32 GPIO13

  重要提醒：
  - GPIO13 已在本教程使用的 ESP32 开发板排针上引出，可以直接接舵机信号线
  - 舵机不要直接从 ESP32 3.3V 供电，必须使用外部 5V 电源
  - ESP32 的 GND 必须与外部电源的 GND 连接（共地）

  使用方法：
  1. 打开串口监视器（波特率 115200）
  2. 输入 0-180 之间的数字，按回车
  3. 观察舵机平滑转动到目标角度
*/

// ===========================================
// 1. 引脚和参数配置
// ===========================================
const int SERVO_PIN = 13;          // 舵机信号线接 GPIO13，排针已引出
const int PWM_FREQ = 50;           // 舵机标准频率 50Hz（周期 20ms）
const int PWM_RESOLUTION = 16;     // PWM 分辨率 16 位（0-65535）

// ===========================================
// 2. 舵机状态变量
// ===========================================
int currentAngle = 90;             // 当前角度（初始化为 90°）
int targetAngle = 90;              // 目标角度（初始化为 90°）

// ===========================================
// 3. 非阻塞定时器变量
// ===========================================
unsigned long lastServoUpdate = 0; // 上次更新舵机的时间
const unsigned long servoInterval = 15; // 舵机更新间隔（毫秒）
                                   // 每 15ms 转动 1°，转动 180° 需要 2.7 秒

// ===========================================
// 4. 辅助函数：角度转 PWM 占空比
// ===========================================
/*
  舵机工作原理：
  - 舵机通过 PWM 脉宽控制角度
  - 标准舵机：500us → 0°，1500us → 90°，2500us → 180°
  - 50Hz 周期 = 20000us

  本函数将角度（0-180）转换为 16 位 PWM 占空比值
*/
uint32_t angleToDuty(int angle) {
  // 限制角度范围在 0-180 度之间
  angle = constrain(angle, 0, 180);

  // 定义脉宽范围（微秒）
  const int minUs = 500;   // 0° 对应 500us
  const int maxUs = 2500;  // 180° 对应 2500us

  // 将角度映射到脉宽（微秒）
  int pulseUs = map(angle, 0, 180, minUs, maxUs);

  // 计算 16 位 PWM 的最大占空比值（2^16 - 1 = 65535）
  uint32_t maxDuty = (1UL << PWM_RESOLUTION) - 1;

  // 将脉宽（微秒）转换为占空比值
  // 占空比 = (脉宽 / 周期) × 最大占空比
  return (uint32_t)((pulseUs * maxDuty) / 20000UL);
}

// ===========================================
// 5. 辅助函数：写入舵机角度
// ===========================================
/*
  将指定角度写入舵机
  内部调用 angleToDuty() 转换为 PWM 占空比
*/
void writeServo(int angle) {
  ledcWrite(SERVO_PIN, angleToDuty(angle));
}

// ===========================================
// 6. 串口输入处理函数
// ===========================================
/*
  处理串口输入的角度值
  - 读取用户输入的数字
  - 限制在 0-180 范围内
  - 设置为目标角度
*/
void handleSerialInput() {
  // 检查串口是否有数据可读
  if (Serial.available() > 0) {
    // 读取整数（自动跳过空格、回车等）
    int inputAngle = Serial.parseInt();

    // 清空串口缓冲区剩余字符（如回车换行）
    while (Serial.available() > 0) {
      Serial.read();
    }

    // 限制角度范围
    inputAngle = constrain(inputAngle, 0, 180);

    // 设置目标角度
    targetAngle = inputAngle;

    // 打印反馈信息
    Serial.print("目标角度设置为: ");
    Serial.print(targetAngle);
    Serial.println("°");
  }
}

// ===========================================
// 7. 舵机更新函数（非阻塞）
// ===========================================
/*
  非阻塞舵机控制核心逻辑：
  - 每隔 servoInterval 毫秒执行一次
  - 如果当前角度 < 目标角度，则 +1°
  - 如果当前角度 > 目标角度，则 -1°
  - 直到当前角度 = 目标角度，停止转动

  为什么不用 delay()？
  - delay() 会阻塞整个程序
  - 使用 millis() 计时，程序可以同时处理其他任务
*/
void updateServo() {
  // 获取当前时间（毫秒）
  unsigned long now = millis();

  // 如果距离上次更新时间不足 servoInterval，直接返回
  if (now - lastServoUpdate < servoInterval) {
    return;
  }

  // 更新时间戳
  lastServoUpdate = now;

  // 如果当前角度小于目标角度，增加 1°
  if (currentAngle < targetAngle) {
    currentAngle++;
    writeServo(currentAngle);
    Serial.print("当前角度: ");
    Serial.println(currentAngle);
  }
  // 如果当前角度大于目标角度，减少 1°
  else if (currentAngle > targetAngle) {
    currentAngle--;
    writeServo(currentAngle);
    Serial.print("当前角度: ");
    Serial.println(currentAngle);
  }
  // 如果当前角度等于目标角度，不做任何操作（已到达目标）
}

// ===========================================
// 8. 初始化函数
// ===========================================
void setup() {
  // 初始化串口通信（波特率 115200）
  Serial.begin(115200);
  delay(1000); // 等待串口稳定

  // 打印欢迎信息
  Serial.println();
  Serial.println("=========================================");
  Serial.println("  ESP32-S3 基础舵机控制");
  Serial.println("=========================================");
  Serial.println("请输入角度（0-180）并按回车：");
  Serial.println();

  // 配置 PWM 通道
  // Arduino-ESP32 3.x 推荐写法：ledcAttach(pin, freq, resolution)
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RESOLUTION);

  // 将舵机初始化到 90° 中间位置
  writeServo(currentAngle);
  Serial.print("舵机初始化完成，当前角度: ");
  Serial.print(currentAngle);
  Serial.println("°");
  Serial.println();
}

// ===========================================
// 9. 主循环函数
// ===========================================
/*
  主循环中调用两个函数：
  1. handleSerialInput() - 处理串口输入
  2. updateServo() - 更新舵机位置

  两个函数都是非阻塞的，loop() 可以快速循环
*/
void loop() {
  handleSerialInput();  // 检查并处理串口输入
  updateServo();        // 平滑更新舵机角度
}
