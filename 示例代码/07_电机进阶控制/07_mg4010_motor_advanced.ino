/*
  ESP32-S3 示例 07：MG4010 电机进阶控制

  功能：
  - 位置控制模式（角度控制）
  - 速度控制模式
  - 速度 + 位置组合控制
  - 实时读取电机反馈（位置、速度、电流）
  - 网页控制界面，实时显示电机状态

  接线（CAN 通信）：
  MG4010 CAN_H → CAN 收发器 CANH → ESP32 GPIO4 (CAN TX)
  MG4010 CAN_L → CAN 收发器 CANL → ESP32 GPIO5 (CAN RX)
  CAN 收发器电源 → 3.3V
  MG4010 电源 → 12-24V 外部电源
  所有 GND 共地

  MG4010 默认参数：
  - CAN 波特率：1Mbps
  - 默认 ID：1
  - 位置范围：-12.5圈 到 +12.5圈（-4500° 到 +4500°）
  - 最大速度：约 400 RPM

  使用：
  1. 上传程序
  2. 手机连接 Wi-Fi：ESP32S3_MOTOR，密码：12345678
  3. 浏览器打开：http://192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

const char* ssid = "ESP32S3_MOTOR";
const char* password = "12345678";

WebServer server(80);

// CAN 配置
CAN_device_t CAN_cfg;
const int CAN_TX_PIN = 4;
const int CAN_RX_PIN = 5;

// 电机参数
const uint8_t MOTOR_ID = 1;

// 控制模式
enum ControlMode {
  MODE_STOP = 0,
  MODE_POSITION = 1,
  MODE_SPEED = 2,
  MODE_POSITION_SPEED = 3
};

// 电机状态
struct MotorState {
  float position;      // 当前位置（度）
  float speed;         // 当前速度（RPM）
  float torque;        // 当前力矩（Nm）
  uint8_t temperature; // 温度（℃）
  uint8_t errorCode;   // 错误代码
  unsigned long lastUpdate;
};

MotorState motorState = {0, 0, 0, 0, 0, 0};

// 控制参数
ControlMode currentMode = MODE_STOP;
float targetPosition = 0;    // 目标位置（度）
float targetSpeed = 0;       // 目标速度（RPM）
float maxSpeed = 200;        // 位置模式最大速度（RPM）

unsigned long lastMotorCmd = 0;
const unsigned long motorCmdInterval = 20;  // 50Hz 控制频率

unsigned long lastMotorRead = 0;
const unsigned long motorReadInterval = 100; // 10Hz 读取频率

unsigned long lastStatusSend = 0;
const unsigned long statusSendInterval = 200; // 5Hz 状态发送频率

// CAN 消息帧
CAN_frame_t tx_frame;
CAN_frame_t rx_frame;

// ========== MG4010 CAN 协议函数 ==========

// 初始化 CAN 总线
void initCAN() {
  CAN_cfg.speed = CAN_SPEED_1000KBPS;
  CAN_cfg.tx_pin_id = (gpio_num_t)CAN_TX_PIN;
  CAN_cfg.rx_pin_id = (gpio_num_t)CAN_RX_PIN;
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));

  ESP32Can.CANInit();
  Serial.println("CAN bus initialized");
}

// 发送 CAN 帧
void sendCANFrame(uint32_t id, uint8_t* data, uint8_t len) {
  tx_frame.FIR.B.FF = CAN_frame_std;
  tx_frame.FIR.B.DLC = len;
  tx_frame.MsgID = id;
  memcpy(tx_frame.data.u8, data, len);
  ESP32Can.CANWriteFrame(&tx_frame);
}

// 电机使能
void motorEnable() {
  uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
  sendCANFrame(MOTOR_ID, data, 8);
  Serial.println("Motor enabled");
}

// 电机失能
void motorDisable() {
  uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
  sendCANFrame(MOTOR_ID, data, 8);
  Serial.println("Motor disabled");
}

// 电机归零
void motorZero() {
  uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
  sendCANFrame(MOTOR_ID, data, 8);
  Serial.println("Motor zero set");
}

// 位置控制（角度）
void motorPositionControl(float angleDeg, float speedRPM) {
  // 角度转换：度 → 0.01度单位
  int32_t angleRaw = (int32_t)(angleDeg * 100.0f);

  // 速度限制：0-400 RPM
  speedRPM = constrain(speedRPM, 0, 400);
  uint16_t speedRaw = (uint16_t)speedRPM;

  uint8_t data[8];
  data[0] = 0xA3;
  data[1] = 0x00;
  data[2] = (uint8_t)(speedRaw & 0xFF);
  data[3] = (uint8_t)((speedRaw >> 8) & 0xFF);
  data[4] = (uint8_t)(angleRaw & 0xFF);
  data[5] = (uint8_t)((angleRaw >> 8) & 0xFF);
  data[6] = (uint8_t)((angleRaw >> 16) & 0xFF);
  data[7] = (uint8_t)((angleRaw >> 24) & 0xFF);

  sendCANFrame(MOTOR_ID, data, 8);
}

// 速度控制（RPM）
void motorSpeedControl(float speedRPM) {
  // 速度限制：-400 到 400 RPM
  speedRPM = constrain(speedRPM, -400, 400);
  int32_t speedRaw = (int32_t)(speedRPM * 100.0f);

  uint8_t data[8];
  data[0] = 0xA2;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = (uint8_t)(speedRaw & 0xFF);
  data[5] = (uint8_t)((speedRaw >> 8) & 0xFF);
  data[6] = (uint8_t)((speedRaw >> 16) & 0xFF);
  data[7] = (uint8_t)((speedRaw >> 24) & 0xFF);

  sendCANFrame(MOTOR_ID, data, 8);
}

// 读取电机状态
void motorReadStatus() {
  uint8_t data[8] = {0x9C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendCANFrame(MOTOR_ID, data, 8);
}

// 解析电机反馈
void parseMotorFeedback(CAN_frame_t* frame) {
  if (frame->FIR.B.DLC < 8) return;

  uint8_t* data = frame->data.u8;

  // 根据命令字解析
  if (data[0] == 0x9C) {
    // 状态反馈
    int32_t posRaw = (int32_t)(data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24));
    int16_t spdRaw = (int16_t)(data[5] | (data[6] << 8));

    motorState.position = posRaw * 0.01f;  // 0.01度单位转度
    motorState.speed = spdRaw * 1.0f;      // RPM
    motorState.temperature = data[7];
    motorState.lastUpdate = millis();

  } else if (data[0] == 0xA1 || data[0] == 0xA2 || data[0] == 0xA3) {
    // 控制命令反馈
    int16_t posRaw = (int16_t)(data[1] | (data[2] << 8));
    int16_t spdRaw = (int16_t)(data[3] | (data[4] << 8));
    int16_t torqueRaw = (int16_t)(data[5] | (data[6] << 8));

    motorState.position = posRaw * 0.01f;
    motorState.speed = spdRaw * 1.0f;
    motorState.torque = torqueRaw * 0.01f;
    motorState.temperature = data[7];
    motorState.lastUpdate = millis();
  }
}

// 读取 CAN 消息
void processCAN() {
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 0) == pdTRUE) {
    if (rx_frame.MsgID == MOTOR_ID) {
      parseMotorFeedback(&rx_frame);
    }
  }
}

// ========== 控制逻辑 ==========

void updateMotorControl() {
  unsigned long now = millis();

  // 控制命令发送
  if (now - lastMotorCmd >= motorCmdInterval) {
    lastMotorCmd = now;

    switch (currentMode) {
      case MODE_POSITION:
        motorPositionControl(targetPosition, maxSpeed);
        break;

      case MODE_SPEED:
        motorSpeedControl(targetSpeed);
        break;

      case MODE_POSITION_SPEED:
        motorPositionControl(targetPosition, maxSpeed);
        break;

      case MODE_STOP:
        motorSpeedControl(0);
        break;
    }
  }

  // 状态读取
  if (now - lastMotorRead >= motorReadInterval) {
    lastMotorRead = now;
    motorReadStatus();
  }
}

// ========== Web 服务器 ==========

String htmlPage() {
  String html = "";
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32-S3 MG4010 进阶控制</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:#f5f5f5;}";
  html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "h2{text-align:center;color:#333;margin-top:0;}";
  html += ".section{margin:20px 0;padding:15px;background:#f9f9f9;border-radius:5px;}";
  html += ".section-title{font-weight:bold;margin-bottom:10px;color:#555;}";
  html += ".status{display:flex;justify-content:space-between;margin:8px 0;font-size:14px;}";
  html += ".status-label{color:#666;}";
  html += ".status-value{font-weight:bold;color:#007bff;}";
  html += ".control-group{margin:15px 0;}";
  html += "label{display:block;margin-bottom:5px;color:#555;font-size:14px;}";
  html += "input[type='number'],input[type='range']{width:100%;padding:8px;margin-bottom:5px;box-sizing:border-box;}";
  html += "input[type='range']{padding:0;}";
  html += ".range-value{text-align:center;font-weight:bold;color:#007bff;margin:5px 0;}";
  html += "button{width:100%;padding:12px;margin:5px 0;font-size:16px;border:none;border-radius:5px;cursor:pointer;transition:all 0.3s;}";
  html += ".btn-primary{background:#007bff;color:white;}.btn-primary:hover{background:#0056b3;}";
  html += ".btn-success{background:#28a745;color:white;}.btn-success:hover{background:#1e7e34;}";
  html += ".btn-warning{background:#ffc107;color:#000;}.btn-warning:hover{background:#e0a800;}";
  html += ".btn-danger{background:#dc3545;color:white;}.btn-danger:hover{background:#c82333;}";
  html += ".btn-secondary{background:#6c757d;color:white;}.btn-secondary:hover{background:#545b62;}";
  html += ".btn-group{display:grid;grid-template-columns:1fr 1fr;gap:10px;}";
  html += ".offline{color:#dc3545;font-weight:bold;}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h2>🔧 MG4010 电机进阶控制</h2>";

  // 状态显示
  html += "<div class='section'>";
  html += "<div class='section-title'>📊 电机状态</div>";
  html += "<div class='status'><span class='status-label'>位置：</span><span class='status-value' id='pos'>--</span></div>";
  html += "<div class='status'><span class='status-label'>速度：</span><span class='status-value' id='spd'>--</span></div>";
  html += "<div class='status'><span class='status-label'>力矩：</span><span class='status-value' id='trq'>--</span></div>";
  html += "<div class='status'><span class='status-label'>温度：</span><span class='status-value' id='temp'>--</span></div>";
  html += "<div class='status'><span class='status-label'>模式：</span><span class='status-value' id='mode'>停止</span></div>";
  html += "</div>";

  // 位置控制
  html += "<div class='section'>";
  html += "<div class='section-title'>📍 位置控制</div>";
  html += "<label>目标位置（度）</label>";
  html += "<input type='range' id='posSlider' min='-4500' max='4500' value='0' oninput='updatePosValue(this.value)'>";
  html += "<div class='range-value' id='posValue'>0°</div>";
  html += "<label>最大速度（RPM）</label>";
  html += "<input type='number' id='maxSpd' value='200' min='10' max='400'>";
  html += "<button class='btn-primary' onclick='setPosition()'>设置位置</button>";
  html += "<div class='btn-group'>";
  html += "<button class='btn-secondary' onclick='quickPos(-360)'>-360°</button>";
  html += "<button class='btn-secondary' onclick='quickPos(360)'>+360°</button>";
  html += "</div>";
  html += "</div>";

  // 速度控制
  html += "<div class='section'>";
  html += "<div class='section-title'>⚡ 速度控制</div>";
  html += "<label>目标速度（RPM）</label>";
  html += "<input type='range' id='spdSlider' min='-400' max='400' value='0' oninput='updateSpdValue(this.value)'>";
  html += "<div class='range-value' id='spdValue'>0 RPM</div>";
  html += "<button class='btn-success' onclick='setSpeed()'>设置速度</button>";
  html += "</div>";

  // 系统控制
  html += "<div class='section'>";
  html += "<div class='section-title'>⚙️ 系统控制</div>";
  html += "<div class='btn-group'>";
  html += "<button class='btn-success' onclick='motorCmd(\"enable\")'>使能</button>";
  html += "<button class='btn-danger' onclick='motorCmd(\"disable\")'>失能</button>";
  html += "</div>";
  html += "<div class='btn-group'>";
  html += "<button class='btn-warning' onclick='motorCmd(\"zero\")'>设置零点</button>";
  html += "<button class='btn-danger' onclick='motorCmd(\"stop\")'>紧急停止</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>";

  // JavaScript
  html += "<script>";
  html += "function updatePosValue(v){document.getElementById('posValue').textContent=v+'°';}";
  html += "function updateSpdValue(v){document.getElementById('spdValue').textContent=v+' RPM';}";
  html += "function setPosition(){let p=document.getElementById('posSlider').value;let s=document.getElementById('maxSpd').value;fetch('/position?pos='+p+'&spd='+s);}";
  html += "function setSpeed(){let s=document.getElementById('spdSlider').value;fetch('/speed?spd='+s);}";
  html += "function quickPos(offset){let current=parseFloat(document.getElementById('posSlider').value);let newPos=current+offset;newPos=Math.max(-4500,Math.min(4500,newPos));document.getElementById('posSlider').value=newPos;updatePosValue(newPos);setPosition();}";
  html += "function motorCmd(cmd){fetch('/cmd?action='+cmd);}";

  // 状态更新
  html += "function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('pos').textContent=d.position.toFixed(1)+'°';";
  html += "document.getElementById('spd').textContent=d.speed.toFixed(0)+' RPM';";
  html += "document.getElementById('trq').textContent=d.torque.toFixed(2)+' Nm';";
  html += "document.getElementById('temp').textContent=d.temperature+'°C';";
  html += "let modes=['停止','位置控制','速度控制','位置+速度'];";
  html += "document.getElementById('mode').textContent=modes[d.mode];";
  html += "}).catch(e=>console.log(e));}";
  html += "setInterval(updateStatus,200);";
  html += "updateStatus();";
  html += "</script>";

  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handlePosition() {
  if (server.hasArg("pos") && server.hasArg("spd")) {
    targetPosition = server.arg("pos").toFloat();
    maxSpeed = server.arg("spd").toFloat();
    maxSpeed = constrain(maxSpeed, 10, 400);
    currentMode = MODE_POSITION;

    Serial.print("Position mode: ");
    Serial.print(targetPosition);
    Serial.print("° @ ");
    Serial.print(maxSpeed);
    Serial.println(" RPM");
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("spd")) {
    targetSpeed = server.arg("spd").toFloat();
    targetSpeed = constrain(targetSpeed, -400, 400);
    currentMode = MODE_SPEED;

    Serial.print("Speed mode: ");
    Serial.print(targetSpeed);
    Serial.println(" RPM");
  }
  server.send(200, "text/plain", "OK");
}

void handleCmd() {
  if (server.hasArg("action")) {
    String action = server.arg("action");

    if (action == "enable") {
      motorEnable();
      delay(100);
    } else if (action == "disable") {
      motorDisable();
      currentMode = MODE_STOP;
    } else if (action == "zero") {
      motorZero();
      delay(100);
      motorState.position = 0;
    } else if (action == "stop") {
      currentMode = MODE_STOP;
      targetSpeed = 0;
    }

    Serial.println("Command: " + action);
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"position\":" + String(motorState.position, 1) + ",";
  json += "\"speed\":" + String(motorState.speed, 0) + ",";
  json += "\"torque\":" + String(motorState.torque, 2) + ",";
  json += "\"temperature\":" + String(motorState.temperature) + ",";
  json += "\"mode\":" + String((int)currentMode);
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-S3 MG4010 Advanced Control ===");

  // 初始化 CAN
  initCAN();
  delay(500);

  // 电机初始化
  motorEnable();
  delay(100);

  // 启动 Wi-Fi AP
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("Wi-Fi AP started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // 配置 Web 服务器
  server.on("/", handleRoot);
  server.on("/position", handlePosition);
  server.on("/speed", handleSpeed);
  server.on("/cmd", handleCmd);
  server.on("/status", handleStatus);
  server.begin();

  Serial.println("Web server started");
  Serial.println("Ready!");
}

void loop() {
  server.handleClient();
  processCAN();
  updateMotorControl();
}
