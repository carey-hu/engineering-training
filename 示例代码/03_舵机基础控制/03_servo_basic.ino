/*
  ESP32-S3 示例 03：基础舵机控制（串口输入角度）

  本例程重点：
  - 去掉 WiFi，纯本地控制
  - 串口输入角度（0-180）控制舵机
  - 使用非阻塞写法，舵机平滑转动

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

const int SERVO_PIN = 13;  // 舵机控制引脚
const int PWM_FREQ = 50;  // PWM频率50Hz，舵机标准
const int PWM_RESOLUTION = 16;  // 16位分辨率提高精度

int currentAngle = 90, targetAngle = 90;  // 当前角度和目标角度

unsigned long lastServoUpdate = 0;  // 上次更新舵机的时间
const unsigned long servoInterval = 15;  // 舵机更新间隔15ms

// 将角度（0-180）转换为 16 位 PWM 占空比
uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);  // 限制角度范围0-180

  const int minUs = 500;   // 0° 对应 500us
  const int maxUs = 2500;  // 180° 对应 2500us

  int pulseUs = map(angle, 0, 180, minUs, maxUs);  // 角度映射到脉宽
  uint32_t maxDuty = (1UL << PWM_RESOLUTION) - 1;  // 计算最大占空比值

  return (uint32_t)((pulseUs * maxDuty) / 20000UL);  // 转换为占空比（20ms周期）
}

// 处理串口输入
void handleSerialInput() {
  if (Serial.available() > 0) {  // 检查是否有数据
    int inputAngle = Serial.parseInt();  // 读取整数角度

    while (Serial.available() > 0) Serial.read();  // 清空缓冲区

    targetAngle = constrain(inputAngle, 0, 180);  // 限制目标角度范围
    Serial.print("目标角度设置为: ");
    Serial.print(targetAngle);
    Serial.println("°");
  }
}

// 非阻塞舵机更新
void updateServo() {
  unsigned long now = millis();  // 获取当前时间

  if (now - lastServoUpdate < servoInterval) return;  // 未到更新时间则返回

  lastServoUpdate = now;  // 记录更新时间

  if (currentAngle != targetAngle) {  // 当前角度未到达目标
    currentAngle += (currentAngle < targetAngle) ? 1 : -1;  // 每次移动1度
    ledcWrite(SERVO_PIN, angleToDuty(currentAngle));  // 输出PWM信号
    Serial.print("当前角度: ");
    Serial.println(currentAngle);
  }
}

void setup() {
  Serial.begin(115200);  // 初始化串口
  delay(1000);  // 等待串口稳定

  Serial.println("\n=========================================\n"
                 "  ESP32-S3 基础舵机控制\n"
                 "=========================================\n"
                 "请输入角度（0-180）并按回车：\n");

  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RESOLUTION);  // 配置PWM通道
  ledcWrite(SERVO_PIN, angleToDuty(currentAngle));  // 设置初始角度

  Serial.print("舵机初始化完成，当前角度: ");
  Serial.print(currentAngle);
  Serial.println("°\n");
}

void loop() {
  handleSerialInput();  // 处理串口输入
  updateServo();  // 更新舵机位置
}
