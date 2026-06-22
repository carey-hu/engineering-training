/*
  ESP32-S3 示例 08：MG4010-i10 电机网页综合控制

  使用厂家 RS485 私有协议：
  - 0x88 电机运行 / 0x81 电机停止
  - 0xA2 速度闭环 / 0xA4 多圈位置闭环 2
  - 0x9C 读取状态 2 / 0x92 读取多圈角度

  网页中输入的速度和角度均指输出轴。代码按 i10 减速比换算为电机侧协议值。
  RS485 接线：GPIO17(TX) -> 转接板 T，GPIO18(RX) -> R，ESP32 5V/GND -> USB调试 5V/G。
*/

#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

const char *ssid = "ESP32S3_MOTOR", *password = "12345678"; // WiFi热点名称和密码
WebServer server(80); // 创建Web服务器，端口80

#define RS485_TX 17 // RS485发送引脚
#define RS485_RX 18 // RS485接收引脚
#define RS485_DE_RE -1 // 收发切换引脚，-1表示自动
#define MOTOR_ID 0x01 // 电机ID地址
#define RS485_BAUD 115200 // RS485波特率
#define REDUCTION 10.0f // 减速比i10
#define ANGLE_SCALE 1.0f // 角度缩放系数
#define SPEED_SCALE 10.0f // 速度缩放系数（减速比）
#define CMD_MOTOR_RUN 0x88 // 电机运行命令
#define CMD_MOTOR_STOP 0x81 // 电机停止命令
#define CMD_SPEED 0xA2 // 速度闭环控制命令
#define CMD_POSITION 0xA4 // 多圈位置闭环命令
#define CMD_READ_STATE2 0x9C // 读取状态2命令
#define CMD_READ_ANGLE 0x92 // 读取多圈角度命令

HardwareSerial RS485(2); // 使用串口2作为RS485

enum Mode { MODE_STOP, MODE_SPEED, MODE_POSITION }; // 定义控制模式枚举
Mode currentMode = MODE_STOP; // 当前模式，默认停止

float targetOutputDps = 0, targetOutputAngle = 0, maxOutputDps = 90; // 目标输出轴速度、角度、最大速度
float actualOutputDps = 0, actualOutputAngle = 0; // 实际输出轴速度和角度
int motorTemperature = 0; // 电机温度
unsigned long lastReadStateTime = 0, lastReadAngleTime = 0; // 上次读取状态和角度的时间

uint8_t byteSum(const uint8_t *data, uint8_t len) { // 计算字节累加和校验
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum += data[i];
  return sum & 0xFF; // 返回低8位
}

void setRS485Mode(bool tx) { // 设置RS485收发模式
  if (RS485_DE_RE < 0) return; // 如果未定义切换引脚则返回
  if (tx) { digitalWrite(RS485_DE_RE, HIGH); delayMicroseconds(50); } // 发送模式
  else { RS485.flush(); delayMicroseconds(50); digitalWrite(RS485_DE_RE, LOW); } // 接收模式
}

void sendCommand(uint8_t cmd, const uint8_t *data = nullptr, uint8_t dataLen = 0) { // 发送RS485协议命令
  uint8_t header[5] = {0x3E, cmd, MOTOR_ID, dataLen, 0x00}; // 构造帧头：起始字节、命令、ID、数据长度、校验
  header[4] = byteSum(header, 4); // 计算帧头校验和
  while (RS485.available()) RS485.read(); // 清空接收缓冲
  setRS485Mode(true); // 切换到发送模式
  RS485.write(header, 5); // 发送帧头
  if (dataLen > 0 && data) { // 如果有数据
    RS485.write(data, dataLen); // 发送数据
    uint8_t sum = byteSum(data, dataLen); // 计算数据校验和
    RS485.write(&sum, 1); // 发送数据校验和
  }
  setRS485Mode(false); // 切换到接收模式
}

bool waitBytes(uint8_t *buf, uint8_t count, unsigned long deadline) { // 等待接收指定字节数
  uint8_t i = 0;
  while (millis() < deadline && i < count) { // 在超时时间内循环
    if (RS485.available()) buf[i++] = RS485.read(); // 读取字节
  }
  return i == count; // 返回是否接收完成
}

bool readFrame(uint8_t expectedCmd, uint8_t *data, uint8_t *dataLen) { // 读取RS485协议帧
  const uint16_t timeoutMs = 80; // 超时时间80ms
  uint8_t header[5], i = 0;
  unsigned long deadline = millis() + timeoutMs; // 计算超时截止时间
  while (millis() < deadline && i < 5) { // 接收5字节帧头
    if (!RS485.available()) continue;
    uint8_t b = RS485.read();
    if (i == 0 && b != 0x3E) continue; // 等待起始字节0x3E
    header[i++] = b;
  }
  if (i < 5 || header[4] != byteSum(header, 4)) return false; // 校验帧头完整性和校验和
  if (header[1] != expectedCmd || header[2] != MOTOR_ID) return false; // 校验命令和ID
  *dataLen = header[3]; // 获取数据长度
  if (*dataLen == 0) return true; // 无数据则返回
  uint8_t payload[32];
  deadline = millis() + timeoutMs; // 重置超时时间
  if (!waitBytes(payload, *dataLen + 1, deadline)) return false; // 接收数据和校验和
  if (payload[*dataLen] != byteSum(payload, *dataLen)) return false; // 校验数据校验和
  memcpy(data, payload, *dataLen); // 复制数据到输出缓冲
  return true;
}

void writeIntLE(uint8_t *buf, uint64_t value, uint8_t bytes) { // 以小端序写入整数
  for (uint8_t i = 0; i < bytes; i++) buf[i] = (value >> (8 * i)) & 0xFF;
}

void sendMotorCommand(float outputDps, float outputAngle = 0, bool isPosition = false) { // 发送电机控制命令
  sendCommand(CMD_MOTOR_RUN); // 先发送运行命令
  delay(5);
  if (!isPosition) { // 速度模式
    outputDps = constrain(outputDps, -360.0f, 360.0f); // 限制速度范围
    uint8_t data[4];
    writeIntLE(data, (uint32_t)(int32_t)(outputDps * SPEED_SCALE * 100.0f), 4); // 输出轴速度转换为电机侧协议值
    sendCommand(CMD_SPEED, data, 4); // 发送速度命令
  } else { // 位置模式
    outputAngle = constrain(outputAngle, -720.0f, 720.0f); // 限制角度范围
    outputDps = constrain(outputDps, 10.0f, 360.0f); // 限制速度范围
    uint8_t data[12];
    writeIntLE(data, (uint64_t)(int64_t)(outputAngle * ANGLE_SCALE * 100.0f), 8); // 输出轴角度转换为电机侧协议值
    writeIntLE(data + 8, (uint32_t)(outputDps * SPEED_SCALE * 100.0f), 4); // 输出轴速度转换为电机侧协议值
    sendCommand(CMD_POSITION, data, 12); // 发送位置命令
  }
}

void readState2() { // 读取电机状态2
  sendCommand(CMD_READ_STATE2); // 发送读取状态命令
  uint8_t data[16], len = 0;
  if (!readFrame(CMD_READ_STATE2, data, &len) || len < 7) return; // 接收响应帧
  motorTemperature = (int8_t)data[0]; // 提取温度（有符号）
  int16_t motorDps = (int16_t)(data[3] | (data[4] << 8)); // 提取电机侧速度（小端序）
  actualOutputDps = motorDps / SPEED_SCALE; // 转换为输出轴速度
}

void readAngle() { // 读取多圈角度
  sendCommand(CMD_READ_ANGLE); // 发送读取角度命令
  uint8_t data[16], len = 0;
  if (!readFrame(CMD_READ_ANGLE, data, &len) || len < 8) return; // 接收响应帧
  uint64_t rawUnsigned = 0;
  for (uint8_t i = 0; i < 8; i++) rawUnsigned |= ((uint64_t)data[i]) << (8 * i); // 小端序组装8字节
  actualOutputAngle = (int64_t)rawUnsigned / 100.0f / ANGLE_SCALE; // 转换为输出轴角度
}

const char HTML_PAGE[] PROGMEM = R"(
<!DOCTYPE html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>MG4010-i10 电机控制</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;}
body{font-family:Arial,'Microsoft YaHei',sans-serif;background:#f4f6f8;color:#1f2933;padding:16px;}
.container{max-width:560px;margin:0 auto;}
h2{text-align:center;margin:8px 0 16px;color:#102a43;font-size:24px;}
.card{background:#fff;border:1px solid #d9e2ec;border-radius:8px;padding:16px;margin-bottom:12px;}
.card-title{font-size:15px;color:#243b53;margin-bottom:12px;font-weight:bold;}
.status-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;}
.status-item{background:#eef2f7;padding:10px;border-radius:6px;text-align:center;min-width:0;}
.status-val{font-size:20px;font-weight:bold;color:#0b7285;word-break:break-word;}
.status-label{font-size:12px;color:#52606d;margin-top:4px;}
label{display:block;font-size:13px;color:#52606d;margin:10px 0 5px;}
input[type=range]{width:100%;}
input[type=number]{width:100%;padding:9px;border:1px solid #bcccdc;border-radius:6px;font-size:16px;}
.range-val{text-align:center;color:#0b7285;font-weight:bold;margin:6px 0;}
button{width:100%;padding:11px;font-size:15px;border:0;border-radius:6px;cursor:pointer;margin:5px 0;font-weight:bold;}
.btn-speed{background:#0b7285;color:#fff;}
.btn-pos{background:#334e68;color:#fff;}
.btn-stop{background:#b42318;color:#fff;}
.btn-group{display:grid;grid-template-columns:1fr 1fr;gap:8px;}
.mode{margin-top:10px;padding:9px;border-radius:6px;background:#eef2f7;text-align:center;font-weight:bold;color:#334e68;}
</style></head><body><div class='container'>
<h2>MG4010-i10 电机控制</h2>
<div class='card'><div class='card-title'>实时状态</div><div class='status-grid'>
<div class='status-item'><div class='status-val' id='spd'>--</div><div class='status-label'>输出轴 dps</div></div>
<div class='status-item'><div class='status-val' id='pos'>--</div><div class='status-label'>输出轴角度</div></div>
<div class='status-item'><div class='status-val' id='tmp'>--</div><div class='status-label'>温度 C</div></div>
</div><div id='modeInd' class='mode'>已停止</div></div>
<div class='card'><div class='card-title'>速度模式</div>
<input type='range' id='spdRange' min='-360' max='360' value='0' oninput='upd(this,"spd"," dps")'>
<div class='range-val' id='spdVal'>0 dps</div>
<input type='number' id='spdNum' value='0'>
<button class='btn-speed' onclick='setSpeed()'>启动速度模式</button></div>
<div class='card'><div class='card-title'>位置模式</div>
<input type='range' id='posRange' min='-720' max='720' value='0' oninput='upd(this,"pos","°")'>
<div class='range-val' id='posVal'>0°</div>
<input type='number' id='posNum' value='0'>
<label>输出轴最大速度 dps</label>
<input type='number' id='pspd' value='90' min='10' max='360'>
<div class='btn-group'>
<button class='btn-pos' onclick='quickPos(-90)'>-90°</button>
<button class='btn-pos' onclick='quickPos(0)'>归零</button>
<button class='btn-pos' onclick='quickPos(90)'>+90°</button>
<button class='btn-pos' onclick='setPosition()'>转到位置</button>
</div></div>
<div class='card'><button class='btn-stop' onclick='emergencyStop()'>停止电机</button></div>
</div>
<script>
function upd(e,p,u){let v=e.value;document.getElementById(p+'Val').textContent=v+u;document.getElementById(p+'Num').value=v;}
function setSpeed(){let v=document.getElementById('spdNum').value;document.getElementById('spdRange').value=v;upd({value:v},'spd',' dps');fetch('/mode?m=speed&spd='+v);}
function setPosition(){let a=document.getElementById('posNum').value,s=document.getElementById('pspd').value;document.getElementById('posRange').value=a;upd({value:a},'pos','°');fetch('/mode?m=pos&ang='+a+'&spd='+s);}
function quickPos(v){document.getElementById('posNum').value=v;document.getElementById('posRange').value=v;upd({value:v},'pos','°');setPosition();}
function emergencyStop(){fetch('/mode?m=stop');}
function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{
document.getElementById('spd').textContent=d.spd.toFixed(1);
document.getElementById('pos').textContent=d.pos.toFixed(1);
document.getElementById('tmp').textContent=d.tmp;
document.getElementById('modeInd').textContent=d.mode==1?'速度模式':(d.mode==2?'位置模式':'已停止');
}).catch(e=>{});}
setInterval(updateStatus,500);updateStatus();
</script></body></html>
)";

void handleRoot() { server.send_P(200, "text/html", HTML_PAGE); } // 处理根路径请求，返回网页

void handleMode() { // 处理模式切换请求
  if (!server.hasArg("m")) { server.send(200, "text/plain", "OK"); return; } // 无参数则返回
  String m = server.arg("m"); // 获取模式参数
  if (m == "stop") { // 停止模式
    currentMode = MODE_STOP;
    targetOutputDps = 0;
    sendCommand(CMD_MOTOR_STOP); // 发送停止命令
  } else if (m == "speed") { // 速度模式
    targetOutputDps = constrain(server.arg("spd").toFloat(), -360.0f, 360.0f); // 限制速度范围
    currentMode = MODE_SPEED;
    sendMotorCommand(targetOutputDps); // 发送速度控制命令
  } else if (m == "pos") { // 位置模式
    targetOutputAngle = constrain(server.arg("ang").toFloat(), -720.0f, 720.0f); // 限制角度范围
    maxOutputDps = constrain(server.arg("spd").toFloat(), 10.0f, 360.0f); // 限制速度范围
    currentMode = MODE_POSITION;
    sendMotorCommand(maxOutputDps, targetOutputAngle, true); // 发送位置控制命令
  }
  server.send(200, "text/plain", "OK"); // 返回成功响应
}

void handleStatus() { // 处理状态查询请求
  char json[80];
  snprintf(json, sizeof(json), "{\"spd\":%.1f,\"pos\":%.1f,\"tmp\":%d,\"mode\":%d}",
           actualOutputDps, actualOutputAngle, motorTemperature, (int)currentMode); // 构造JSON响应
  server.send(200, "application/json", json); // 返回JSON格式状态
}

void setup() {
  Serial.begin(115200); // 初始化串口
  delay(1000);
  if (RS485_DE_RE >= 0) { // 如果定义了收发切换引脚
    pinMode(RS485_DE_RE, OUTPUT);
    digitalWrite(RS485_DE_RE, LOW); // 默认接收模式
  }
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX); // 初始化RS485串口
  WiFi.softAP(ssid, password); // 创建WiFi热点
  Serial.println("\nMG4010-i10 网页控制");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP()); // 打印热点IP地址
  server.on("/", handleRoot); // 注册根路径处理函数
  server.on("/mode", handleMode); // 注册模式切换处理函数
  server.on("/status", handleStatus); // 注册状态查询处理函数
  server.begin(); // 启动Web服务器
}

void loop() {
  server.handleClient(); // 处理Web客户端请求
  if (millis() - lastReadStateTime > 500) { // 每500ms读取一次状态
    lastReadStateTime = millis();
    readState2(); // 读取电机状态
  }
  if (millis() - lastReadAngleTime > 1000) { // 每1000ms读取一次角度
    lastReadAngleTime = millis();
    readAngle(); // 读取电机角度
  }
}
