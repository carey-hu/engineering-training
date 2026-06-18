/*
  ESP32-S3 示例 09：MG4010-i10 电机网页综合控制

  使用厂家 RS485 私有协议：
  - 0x88 电机运行
  - 0x81 电机停止
  - 0xA2 速度闭环
  - 0xA4 多圈位置闭环 2（位置 + 速度限制）
  - 0x9C 读取状态 2
  - 0x92 读取多圈角度

  网页中输入的速度和角度均指输出轴。代码按 i10 减速比换算为电机侧协议值。

  RS485 接线：
  GPIO17 -> RS485 DI，GPIO18 -> RS485 RO，GPIO16 -> RS485 DE 和 RE。
  这三个 GPIO 均已在本教程使用的 ESP32 开发板排针上引出。
*/

#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

const char *ssid = "ESP32S3_MOTOR";
const char *password = "12345678";
WebServer server(80);

#define RS485_TX      17   // 排针已引出，接 RS485 模块 DI
#define RS485_RX      18   // 排针已引出，接 RS485 模块 RO
#define RS485_DE_RE   16   // 排针已引出，接 RS485 模块 DE 和 RE

#define MOTOR_ID      0x01
#define RS485_BAUD    115200
#define REDUCTION     10.0f

#define CMD_MOTOR_RUN    0x88
#define CMD_MOTOR_STOP   0x81
#define CMD_SPEED        0xA2
#define CMD_POSITION     0xA4
#define CMD_READ_STATE2  0x9C
#define CMD_READ_ANGLE   0x92

HardwareSerial RS485(2);

enum Mode { MODE_STOP, MODE_SPEED, MODE_POSITION };
Mode currentMode = MODE_STOP;

float targetOutputDps = 0;
float targetOutputAngle = 0;
float maxOutputDps = 90;

float actualOutputDps = 0;
float actualOutputAngle = 0;
int motorTemperature = 0;

unsigned long lastReadStateTime = 0;
unsigned long lastReadAngleTime = 0;

uint8_t byteSum(const uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum += data[i];
  return sum & 0xFF;
}

void rs485Tx() {
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(50);
}

void rs485Rx() {
  RS485.flush();
  delayMicroseconds(50);
  digitalWrite(RS485_DE_RE, LOW);
}

void clearRx() {
  while (RS485.available()) RS485.read();
}

void sendCommand(uint8_t cmd, const uint8_t *data, uint8_t dataLen) {
  uint8_t header[5] = {0x3E, cmd, MOTOR_ID, dataLen, 0x00};
  header[4] = byteSum(header, 4);

  clearRx();
  rs485Tx();
  RS485.write(header, 5);
  if (dataLen > 0 && data != nullptr) {
    RS485.write(data, dataLen);
    uint8_t sum = byteSum(data, dataLen);
    RS485.write(&sum, 1);
  }
  rs485Rx();
}

void sendCommand(uint8_t cmd) {
  sendCommand(cmd, nullptr, 0);
}

bool readFrame(uint8_t expectedCmd, uint8_t *data, uint8_t *dataLen) {
  const uint16_t timeoutMs = 80;
  uint8_t header[5];
  uint8_t i = 0;
  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline && i < 5) {
    if (!RS485.available()) continue;
    uint8_t b = RS485.read();
    if (i == 0 && b != 0x3E) continue;
    header[i++] = b;
  }
  if (i < 5 || header[4] != byteSum(header, 4)) return false;
  if (header[1] != expectedCmd || header[2] != MOTOR_ID) return false;

  *dataLen = header[3];
  uint8_t need = *dataLen + ((*dataLen > 0) ? 1 : 0);
  uint8_t payload[32];
  i = 0;
  deadline = millis() + timeoutMs;
  while (millis() < deadline && i < need && i < sizeof(payload)) {
    if (RS485.available()) payload[i++] = RS485.read();
  }
  if (i < need) return false;
  if (*dataLen > 0 && payload[*dataLen] != byteSum(payload, *dataLen)) return false;
  for (uint8_t n = 0; n < *dataLen; n++) data[n] = payload[n];
  return true;
}

void writeInt64LE(uint8_t *buf, int64_t value) {
  uint64_t raw = (uint64_t)value;
  for (uint8_t i = 0; i < 8; i++) buf[i] = (raw >> (8 * i)) & 0xFF;
}

void writeUint32LE(uint8_t *buf, uint32_t value) {
  buf[0] = value & 0xFF;
  buf[1] = (value >> 8) & 0xFF;
  buf[2] = (value >> 16) & 0xFF;
  buf[3] = (value >> 24) & 0xFF;
}

void motorRun() {
  sendCommand(CMD_MOTOR_RUN);
}

void motorStop() {
  sendCommand(CMD_MOTOR_STOP);
}

void sendSpeedCommand(float outputDps) {
  outputDps = constrain(outputDps, -360.0f, 360.0f);
  int32_t raw = (int32_t)(outputDps * REDUCTION * 100.0f);
  uint32_t rawBytes = (uint32_t)raw;
  uint8_t data[4] = {
    (uint8_t)(rawBytes & 0xFF),
    (uint8_t)((rawBytes >> 8) & 0xFF),
    (uint8_t)((rawBytes >> 16) & 0xFF),
    (uint8_t)((rawBytes >> 24) & 0xFF)
  };
  motorRun();
  delay(5);
  sendCommand(CMD_SPEED, data, 4);
}

void sendPositionCommand(float outputAngle, float outputDps) {
  outputAngle = constrain(outputAngle, -720.0f, 720.0f);
  outputDps = constrain(outputDps, 10.0f, 360.0f);

  int64_t angleRaw = (int64_t)(outputAngle * REDUCTION * 100.0f);
  uint32_t speedRaw = (uint32_t)(outputDps * REDUCTION * 100.0f);

  uint8_t data[12];
  writeInt64LE(data, angleRaw);
  writeUint32LE(data + 8, speedRaw);

  motorRun();
  delay(5);
  sendCommand(CMD_POSITION, data, 12);
}

void readState2() {
  sendCommand(CMD_READ_STATE2);
  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(CMD_READ_STATE2, data, &len) || len < 7) return;

  motorTemperature = (int8_t)data[0];
  int16_t motorDps = (int16_t)(data[3] | (data[4] << 8));
  actualOutputDps = motorDps / REDUCTION;
}

void readAngle() {
  sendCommand(CMD_READ_ANGLE);
  uint8_t data[16];
  uint8_t len = 0;
  if (!readFrame(CMD_READ_ANGLE, data, &len) || len < 8) return;

  uint64_t rawUnsigned = 0;
  for (uint8_t i = 0; i < 8; i++) rawUnsigned |= ((uint64_t)data[i]) << (8 * i);
  int64_t raw = (int64_t)rawUnsigned;
  actualOutputAngle = (raw / 100.0f) / REDUCTION;
}

String htmlPage() {
  String h;
  h += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>MG4010-i10 电机控制</title>";
  h += "<style>";
  h += "*{box-sizing:border-box;margin:0;padding:0;}";
  h += "body{font-family:Arial,'Microsoft YaHei',sans-serif;background:#f4f6f8;color:#1f2933;padding:16px;}";
  h += ".container{max-width:560px;margin:0 auto;}";
  h += "h2{text-align:center;margin:8px 0 16px;color:#102a43;font-size:24px;}";
  h += ".card{background:#fff;border:1px solid #d9e2ec;border-radius:8px;padding:16px;margin-bottom:12px;}";
  h += ".card-title{font-size:15px;color:#243b53;margin-bottom:12px;font-weight:bold;}";
  h += ".status-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;}";
  h += ".status-item{background:#eef2f7;padding:10px;border-radius:6px;text-align:center;min-width:0;}";
  h += ".status-val{font-size:20px;font-weight:bold;color:#0b7285;word-break:break-word;}";
  h += ".status-label{font-size:12px;color:#52606d;margin-top:4px;}";
  h += "label{display:block;font-size:13px;color:#52606d;margin:10px 0 5px;}";
  h += "input[type=range]{width:100%;}";
  h += "input[type=number]{width:100%;padding:9px;border:1px solid #bcccdc;border-radius:6px;font-size:16px;}";
  h += ".range-val{text-align:center;color:#0b7285;font-weight:bold;margin:6px 0;}";
  h += "button{width:100%;padding:11px;font-size:15px;border:0;border-radius:6px;cursor:pointer;margin:5px 0;font-weight:bold;}";
  h += ".btn-speed{background:#0b7285;color:#fff;}";
  h += ".btn-pos{background:#334e68;color:#fff;}";
  h += ".btn-stop{background:#b42318;color:#fff;}";
  h += ".btn-group{display:grid;grid-template-columns:1fr 1fr;gap:8px;}";
  h += ".mode{margin-top:10px;padding:9px;border-radius:6px;background:#eef2f7;text-align:center;font-weight:bold;color:#334e68;}";
  h += "</style></head><body><div class='container'>";
  h += "<h2>MG4010-i10 电机控制</h2>";

  h += "<div class='card'><div class='card-title'>实时状态</div><div class='status-grid'>";
  h += "<div class='status-item'><div class='status-val' id='spd'>--</div><div class='status-label'>输出轴 dps</div></div>";
  h += "<div class='status-item'><div class='status-val' id='pos'>--</div><div class='status-label'>输出轴角度</div></div>";
  h += "<div class='status-item'><div class='status-val' id='tmp'>--</div><div class='status-label'>温度 C</div></div>";
  h += "</div><div id='modeInd' class='mode'>已停止</div></div>";

  h += "<div class='card'><div class='card-title'>速度模式</div>";
  h += "<input type='range' id='spdRange' min='-360' max='360' value='0' oninput='updSpd(this.value)'>";
  h += "<div class='range-val' id='spdVal'>0 dps</div>";
  h += "<input type='number' id='spdNum' value='0'>";
  h += "<button class='btn-speed' onclick='setSpeed()'>启动速度模式</button></div>";

  h += "<div class='card'><div class='card-title'>位置模式</div>";
  h += "<input type='range' id='posRange' min='-720' max='720' value='0' oninput='updPos(this.value)'>";
  h += "<div class='range-val' id='posVal'>0°</div>";
  h += "<input type='number' id='posNum' value='0'>";
  h += "<label>输出轴最大速度 dps</label>";
  h += "<input type='number' id='pspd' value='90' min='10' max='360'>";
  h += "<div class='btn-group'>";
  h += "<button class='btn-pos' onclick='quickPos(-90)'>-90°</button>";
  h += "<button class='btn-pos' onclick='quickPos(0)'>归零</button>";
  h += "<button class='btn-pos' onclick='quickPos(90)'>+90°</button>";
  h += "<button class='btn-pos' onclick='setPosition()'>转到位置</button>";
  h += "</div></div>";

  h += "<div class='card'><button class='btn-stop' onclick='emergencyStop()'>停止电机</button></div>";
  h += "</div>";

  h += "<script>";
  h += "function updSpd(v){document.getElementById('spdVal').textContent=v+' dps';document.getElementById('spdNum').value=v;}";
  h += "function updPos(v){document.getElementById('posVal').textContent=v+'°';document.getElementById('posNum').value=v;}";
  h += "function setSpeed(){let v=document.getElementById('spdNum').value;document.getElementById('spdRange').value=v;updSpd(v);fetch('/mode?m=speed&spd='+v);}";
  h += "function setPosition(){let a=document.getElementById('posNum').value;let s=document.getElementById('pspd').value;document.getElementById('posRange').value=a;updPos(a);fetch('/mode?m=pos&ang='+a+'&spd='+s);}";
  h += "function quickPos(v){document.getElementById('posNum').value=v;document.getElementById('posRange').value=v;updPos(v);setPosition();}";
  h += "function emergencyStop(){fetch('/mode?m=stop');}";
  h += "function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
  h += "document.getElementById('spd').textContent=d.spd.toFixed(1);";
  h += "document.getElementById('pos').textContent=d.pos.toFixed(1);";
  h += "document.getElementById('tmp').textContent=d.tmp;";
  h += "let ind=document.getElementById('modeInd');";
  h += "ind.textContent=d.mode==1?'速度模式':(d.mode==2?'位置模式':'已停止');";
  h += "}).catch(e=>{});}";
  h += "setInterval(updateStatus,500);updateStatus();";
  h += "</script></body></html>";
  return h;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleMode() {
  if (server.hasArg("m")) {
    String m = server.arg("m");
    if (m == "stop") {
      currentMode = MODE_STOP;
      targetOutputDps = 0;
      motorStop();
    } else if (m == "speed") {
      targetOutputDps = constrain(server.arg("spd").toFloat(), -360.0f, 360.0f);
      currentMode = MODE_SPEED;
      sendSpeedCommand(targetOutputDps);
    } else if (m == "pos") {
      targetOutputAngle = constrain(server.arg("ang").toFloat(), -720.0f, 720.0f);
      maxOutputDps = constrain(server.arg("spd").toFloat(), 10.0f, 360.0f);
      currentMode = MODE_POSITION;
      sendPositionCommand(targetOutputAngle, maxOutputDps);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String j = "{";
  j += "\"spd\":" + String(actualOutputDps, 1) + ",";
  j += "\"pos\":" + String(actualOutputAngle, 1) + ",";
  j += "\"tmp\":" + String(motorTemperature) + ",";
  j += "\"mode\":" + String((int)currentMode);
  j += "}";
  server.send(200, "application/json", j);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);

  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.println("MG4010-i10 网页控制");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() - lastReadStateTime > 500) {
    lastReadStateTime = millis();
    readState2();
  }

  if (millis() - lastReadAngleTime > 1000) {
    lastReadAngleTime = millis();
    readAngle();
  }
}
