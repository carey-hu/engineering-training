/*
  ESP32-S3 示例 10：机器人整合控制

  功能：
  - 通过一条 RS485 总线控制两个 MG4010-i10 电机，默认 ID 为 1 和 2
  - 通过 LEDC PWM 控制四个普通 PWM 舵机
  - 使用串口命令测试左右电机速度和四个舵机角度

  本示例是最终工程的主程序模板。前面的单电机、单舵机例程用于理解模块，
  最终机器人项目应采用这种集中调度结构。

  重要要求：
  1. 两个 MG4010-i10 电机必须设置为不同 ID。
  2. 电机使用独立 12V~24V 电源，舵机使用独立 5V 电源。
  3. ESP32 GND、RS485/供电转接板或 RS485 模块 GND、电机电源负极、舵机电源负极必须共地。
  4. 不要在主循环中使用长时间 delay()。

  默认接线：
  RS485：推荐使用课程配套 RS485/供电转接板。
  GPIO17 -> DI，GPIO18 -> RO，GPIO16 -> DE/RE 或方向控制端。
  舵机：GPIO10、GPIO11、GPIO12、GPIO13 分别接四路舵机信号线。
  以上 GPIO 均已在本教程使用的 ESP32 开发板排针上引出。
*/

#include <Arduino.h>
#include <HardwareSerial.h>

// ==================== 1. 引脚和硬件参数 ====================

#define RS485_TX      17   // 排针已引出，接 RS485 转接板/模块 DI
#define RS485_RX      18   // 排针已引出，接 RS485 转接板/模块 RO
#define RS485_DE_RE   16   // 排针已引出，接 RS485 转接板/模块 DE/RE 或方向控制端
#define RS485_BAUD    115200

const uint8_t LEFT_MOTOR_ID = 1;
const uint8_t RIGHT_MOTOR_ID = 2;
const float GEAR_RATIO = 10.0f;  // MG4010-i10：电机侧角度/速度 = 输出轴角度/速度 * 10

const int SERVO_COUNT = 4;
const int SERVO_PINS[SERVO_COUNT] = {10, 11, 12, 13};  // 四路舵机信号线，排针均已引出
const int PWM_FREQ = 50;
const int PWM_RESOLUTION = 16;
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;
const int SERVO_PERIOD_US = 20000;

HardwareSerial RS485(2);

// ==================== 2. 电机协议常量 ====================

const uint8_t FRAME_HEAD = 0x3E;
const uint8_t CMD_MOTOR_STOP = 0x81;
const uint8_t CMD_MOTOR_RUN = 0x88;
const uint8_t CMD_SPEED_CLOSED_LOOP = 0xA2;

// ==================== 3. 舵机状态 ====================

int currentAngles[SERVO_COUNT] = {90, 90, 90, 90};
int targetAngles[SERVO_COUNT] = {90, 90, 90, 90};
unsigned long lastServoUpdate = 0;
uint8_t nextServoToUpdate = 0;
const unsigned long servoInterval = 5;

void updateServos();

// ==================== 4. 工具函数 ====================

uint8_t sumBytes(const uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(sum & 0xFF);
}

void clearRs485Rx() {
  while (RS485.available()) {
    RS485.read();
  }
}

void rs485TransmitMode() {
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(20);
}

void rs485ReceiveMode() {
  RS485.flush();
  delayMicroseconds(20);
  digitalWrite(RS485_DE_RE, LOW);
}

bool readMotorResponse(uint8_t expectedId, uint8_t expectedCmd, uint16_t timeoutMs = 30) {
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    if (!RS485.available()) {
      updateServos();
      delayMicroseconds(100);
      continue;
    }

    if (RS485.read() != FRAME_HEAD) {
      continue;
    }

    uint8_t header[4];
    uint8_t index = 0;
    while (index < 4 && millis() - start < timeoutMs) {
      if (RS485.available()) {
        header[index++] = RS485.read();
      } else {
        updateServos();
        delayMicroseconds(100);
      }
    }

    if (index < 4) return false;

    uint8_t cmd = header[0];
    uint8_t id = header[1];
    uint8_t dataLen = header[2];
    uint8_t headerSum = header[3];
    uint8_t expectedHeaderSum = (uint8_t)((FRAME_HEAD + cmd + id + dataLen) & 0xFF);

    if (headerSum != expectedHeaderSum || id != expectedId || cmd != expectedCmd) {
      continue;
    }

    uint8_t payload[32];
    uint8_t payloadLen = dataLen;
    if (payloadLen > sizeof(payload)) return false;

    index = 0;
    while (index < payloadLen && millis() - start < timeoutMs) {
      if (RS485.available()) {
        payload[index++] = RS485.read();
      } else {
        updateServos();
        delayMicroseconds(100);
      }
    }

    if (index < payloadLen) return false;

    if (payloadLen > 0) {
      while (!RS485.available() && millis() - start < timeoutMs) {
        updateServos();
        delayMicroseconds(100);
      }
      if (!RS485.available()) return false;

      uint8_t dataSum = RS485.read();
      if (dataSum != sumBytes(payload, payloadLen)) {
        return false;
      }
    }

    return true;
  }

  return false;
}

bool sendMotorFrame(uint8_t id, uint8_t cmd, const uint8_t *data, uint8_t dataLen, bool waitResponse = true) {
  clearRs485Rx();

  uint8_t header[5];
  header[0] = FRAME_HEAD;
  header[1] = cmd;
  header[2] = id;
  header[3] = dataLen;
  header[4] = (uint8_t)((header[0] + header[1] + header[2] + header[3]) & 0xFF);

  rs485TransmitMode();
  RS485.write(header, 5);
  if (dataLen > 0) {
    RS485.write(data, dataLen);
    uint8_t dataSum = sumBytes(data, dataLen);
    RS485.write(&dataSum, 1);
  }
  rs485ReceiveMode();

  if (!waitResponse) {
    return true;
  }
  return readMotorResponse(id, cmd);
}

bool motorRun(uint8_t id) {
  return sendMotorFrame(id, CMD_MOTOR_RUN, nullptr, 0);
}

bool motorStop(uint8_t id) {
  return sendMotorFrame(id, CMD_MOTOR_STOP, nullptr, 0);
}

bool setMotorOutputSpeed(uint8_t id, float outputDps) {
  float motorDps = outputDps * GEAR_RATIO;
  int32_t rawSpeed = (int32_t)(motorDps * 100.0f);

  uint8_t data[4];
  data[0] = (uint8_t)(rawSpeed & 0xFF);
  data[1] = (uint8_t)((rawSpeed >> 8) & 0xFF);
  data[2] = (uint8_t)((rawSpeed >> 16) & 0xFF);
  data[3] = (uint8_t)((rawSpeed >> 24) & 0xFF);

  bool runOk = motorRun(id);
  delay(2);
  bool speedOk = sendMotorFrame(id, CMD_SPEED_CLOSED_LOOP, data, 4);
  return runOk && speedOk;
}

// ==================== 5. 舵机控制 ====================

uint32_t angleToDuty(int angle) {
  angle = constrain(angle, 0, 180);
  int pulseUs = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  return (uint32_t)((uint64_t)pulseUs * ((1UL << PWM_RESOLUTION) - 1) / SERVO_PERIOD_US);
}

void writeServoAngle(uint8_t index, int angle) {
  if (index >= SERVO_COUNT) return;
  ledcWrite(SERVO_PINS[index], angleToDuty(angle));
}

void setServoTarget(uint8_t index, int angle) {
  if (index >= SERVO_COUNT) return;
  targetAngles[index] = constrain(angle, 0, 180);
}

void setAllServoTargets(int a1, int a2, int a3, int a4) {
  setServoTarget(0, a1);
  setServoTarget(1, a2);
  setServoTarget(2, a3);
  setServoTarget(3, a4);
}

void updateServos() {
  unsigned long now = millis();
  if (now - lastServoUpdate < servoInterval) {
    return;
  }
  lastServoUpdate = now;

  uint8_t i = nextServoToUpdate;
  nextServoToUpdate = (uint8_t)((nextServoToUpdate + 1) % SERVO_COUNT);

  if (currentAngles[i] < targetAngles[i]) {
    currentAngles[i]++;
    writeServoAngle(i, currentAngles[i]);
  } else if (currentAngles[i] > targetAngles[i]) {
    currentAngles[i]--;
    writeServoAngle(i, currentAngles[i]);
  }
}

// ==================== 6. 串口命令 ====================

void printHelp() {
  Serial.println();
  Serial.println("===== 机器人整合控制 =====");
  Serial.println("run                 两个电机进入运行状态（可选，速度命令会自动运行）");
  Serial.println("stop                两个电机停止");
  Serial.println("v 30 30             左右电机输出轴速度，单位 dps");
  Serial.println("s 1 90              舵机 1 转到 90 度");
  Serial.println("pose 90 90 90 90    四个舵机分别转到指定角度");
  Serial.println("home                电机速度归零，舵机回中");
  Serial.println("help                显示帮助");
  Serial.println();
}

void reportMotorResult(const char *name, bool leftOk, bool rightOk) {
  Serial.print(name);
  Serial.print(" 左电机:");
  Serial.print(leftOk ? "OK" : "无回复");
  Serial.print(" 右电机:");
  Serial.println(rightOk ? "OK" : "无回复");
}

void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line == "help") {
    printHelp();
    return;
  }

  if (line == "run") {
    bool leftOk = motorRun(LEFT_MOTOR_ID);
    bool rightOk = motorRun(RIGHT_MOTOR_ID);
    reportMotorResult("运行命令", leftOk, rightOk);
    return;
  }

  if (line == "stop") {
    bool leftOk = motorStop(LEFT_MOTOR_ID);
    bool rightOk = motorStop(RIGHT_MOTOR_ID);
    reportMotorResult("停止命令", leftOk, rightOk);
    return;
  }

  if (line == "home") {
    bool leftOk = setMotorOutputSpeed(LEFT_MOTOR_ID, 0);
    bool rightOk = setMotorOutputSpeed(RIGHT_MOTOR_ID, 0);
    setAllServoTargets(90, 90, 90, 90);
    reportMotorResult("归零命令", leftOk, rightOk);
    Serial.println("四个舵机目标角度: 90 90 90 90");
    return;
  }

  float leftSpeed, rightSpeed;
  if (sscanf(line.c_str(), "v %f %f", &leftSpeed, &rightSpeed) == 2) {
    leftSpeed = constrain(leftSpeed, -360.0f, 360.0f);
    rightSpeed = constrain(rightSpeed, -360.0f, 360.0f);
    bool leftOk = setMotorOutputSpeed(LEFT_MOTOR_ID, leftSpeed);
    bool rightOk = setMotorOutputSpeed(RIGHT_MOTOR_ID, rightSpeed);
    reportMotorResult("速度命令", leftOk, rightOk);
    Serial.printf("输出轴速度 dps: 左 %.1f, 右 %.1f\n", leftSpeed, rightSpeed);
    return;
  }

  int servoIndex, servoAngle;
  if (sscanf(line.c_str(), "s %d %d", &servoIndex, &servoAngle) == 2) {
    if (servoIndex < 1 || servoIndex > SERVO_COUNT) {
      Serial.println("舵机编号应为 1 到 4");
      return;
    }
    setServoTarget((uint8_t)(servoIndex - 1), servoAngle);
    Serial.printf("舵机 %d 目标角度: %d\n", servoIndex, constrain(servoAngle, 0, 180));
    return;
  }

  int a1, a2, a3, a4;
  if (sscanf(line.c_str(), "pose %d %d %d %d", &a1, &a2, &a3, &a4) == 4) {
    setAllServoTargets(a1, a2, a3, a4);
    Serial.printf("四舵机目标角度: %d %d %d %d\n",
                  constrain(a1, 0, 180),
                  constrain(a2, 0, 180),
                  constrain(a3, 0, 180),
                  constrain(a4, 0, 180));
    return;
  }

  Serial.println("无法识别命令，输入 help 查看命令列表。");
}

void pollSerialCommand() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  handleCommand(line);
}

// ==================== 7. Arduino 入口 ====================

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  delay(500);

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    ledcAttach(SERVO_PINS[i], PWM_FREQ, PWM_RESOLUTION);
    writeServoAngle(i, currentAngles[i]);
  }

  Serial.println("ESP32-S3 机器人整合控制已启动");
  Serial.printf("RS485: TX GPIO%d, RX GPIO%d, DE/RE GPIO%d, baud %d\n",
                RS485_TX, RS485_RX, RS485_DE_RE, RS485_BAUD);
  Serial.printf("电机 ID: 左 %d, 右 %d\n", LEFT_MOTOR_ID, RIGHT_MOTOR_ID);
  Serial.print("舵机 GPIO:");
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    Serial.print(" ");
    Serial.print(SERVO_PINS[i]);
  }
  Serial.println();
  printHelp();
}

void loop() {
  pollSerialCommand();
  updateServos();
}
