/*
  ESP32-S3 示例 09：MG4010 电机网页综合控制（Modbus-RTU）

  功能：
  - WiFi AP 模式，手机/电脑连上后打开网页控制电机
  - 闭环速度模式 + 闭环位置模式切换
  - 网页实时显示电机状态（速度、位置）
  - 滑块 + 数字输入，操作直观

  硬件连接：同 06_电机基础控制

  使用：
  1. 上传程序后，手机连 WiFi：ESP32S3_MOTOR  密码：12345678
  2. 浏览器打开 http://192.168.4.1
  3. 选择控制模式，设置目标值并执行
*/

#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

// ==================== WiFi ====================
const char *ssid     = "ESP32S3_MOTOR";
const char *password = "12345678";
WebServer server(80);

// ==================== 引脚 ====================
#define RS485_TX      17
#define RS485_RX      18
#define RS485_DE_RE   16

// ==================== Modbus ====================
#define SLAVE_ID       0x01
#define BAUD_RATE      9600
#define FUNC_WRITE     0x06
#define FUNC_WRITE_MULTI 0x10
#define FUNC_READ      0x03

#define REG_CONTROL    0x0001
#define REG_SPEED      0x0002
#define REG_POS_HI     0x0003
#define REG_POS_LO     0x0004
#define REG_POS_SPEED  0x0005

#define CTRL_STOP      0x0000
#define CTRL_RUN       0x0001
#define CTRL_POS_MODE  0x0003

// ==================== 全局变量 ====================
HardwareSerial RS485(2);

enum Mode { MODE_STOP, MODE_SPEED, MODE_POSITION };
Mode currentMode = MODE_STOP;

int   targetSpeed = 0;
float targetAngle = 0;
int   posSpeed = 200;

float actualSpeed = 0;
float actualPosition = 0;

unsigned long lastControlTime = 0;
unsigned long lastReadTime = 0;
const unsigned long controlInterval = 50;   // 20Hz
const unsigned long readInterval = 200;     // 5Hz

// ==================== Modbus CRC16 ====================
uint16_t modbusCRC(uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= buf[i];
    for (int j = 0; j < 8; j++)
      crc = (crc & 0x0001) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
  }
  return crc;
}

// ==================== RS485 ====================
void rs485Tx() { digitalWrite(RS485_DE_RE, HIGH); delayMicroseconds(20); }
void rs485Rx() { digitalWrite(RS485_DE_RE, LOW);  delayMicroseconds(20); }

void sendModbus(uint8_t *frame, int len) {
  rs485Tx();
  RS485.write(frame, len);
  RS485.flush();
  rs485Rx();
}

void writeRegister(uint16_t reg, uint16_t value) {
  uint8_t f[8];
  f[0]=SLAVE_ID; f[1]=FUNC_WRITE;
  f[2]=(reg>>8)&0xFF; f[3]=reg&0xFF;
  f[4]=(value>>8)&0xFF; f[5]=value&0xFF;
  uint16_t crc=modbusCRC(f,6);
  f[6]=crc&0xFF; f[7]=(crc>>8)&0xFF;
  sendModbus(f,8);
}

// ==================== 电机控制 ====================
void motorEnable()  { writeRegister(REG_CONTROL, CTRL_RUN); }
void motorDisable() { writeRegister(REG_CONTROL, CTRL_STOP); }

void sendSpeedCommand() {
  writeRegister(REG_SPEED, targetSpeed);
}

void sendPositionCommand() {
  writeRegister(REG_POS_SPEED, posSpeed);
  delay(5);

  int32_t raw = (int32_t)(targetAngle * 100.0f);
  uint16_t hi = (uint32_t)raw >> 16;
  uint16_t lo = (uint32_t)raw & 0xFFFF;

  uint8_t f[15];
  f[0]=SLAVE_ID; f[1]=FUNC_WRITE_MULTI;
  f[2]=0x00; f[3]=0x03;   // 起始地址 0x0003
  f[4]=0x00; f[5]=0x02;   // 寄存器数量 2
  f[6]=0x04;               // 字节数 4
  f[7]=(hi>>8)&0xFF; f[8]=hi&0xFF;
  f[9]=(lo>>8)&0xFF; f[10]=lo&0xFF;
  uint16_t crc=modbusCRC(f,11);
  f[11]=crc&0xFF; f[12]=(crc>>8)&0xFF;
  sendModbus(f,13);
  delay(5);

  writeRegister(REG_CONTROL, CTRL_POS_MODE);
}

void readFeedback() {
  uint8_t f[8];
  f[0]=SLAVE_ID; f[1]=FUNC_READ;
  f[2]=0x00; f[3]=0x01; f[4]=0x00; f[5]=0x04;
  uint16_t crc=modbusCRC(f,6);
  f[6]=crc&0xFF; f[7]=(crc>>8)&0xFF;
  sendModbus(f,8);

  unsigned long t=millis()+100;
  while(millis()<t) {
    if(RS485.available()>=7) {
      uint8_t rx[32]; int len=0;
      while(RS485.available()&&len<32) rx[len++]=RS485.read();
      // 简单解析：从返回数据中读取状态值
      if(len>=9&&rx[0]==SLAVE_ID&&rx[1]==FUNC_READ) {
        // 按协议文档解析实际速度和位置
        // （具体偏移依协议文档而定，此处展示框架）
      }
      return;
    }
  }
}

// ==================== 控制更新 ====================
void updateControl() {
  if(millis()-lastControlTime<controlInterval) return;
  lastControlTime=millis();

  switch(currentMode) {
    case MODE_SPEED:    sendSpeedCommand();    break;
    case MODE_POSITION: sendPositionCommand(); break;
    case MODE_STOP:
      if(targetSpeed!=0) { targetSpeed=0; sendSpeedCommand(); }
      break;
  }
}

void updateRead() {
  if(millis()-lastReadTime<readInterval) return;
  lastReadTime=millis();
  readFeedback();
}

// ==================== 网页 ====================
String htmlPage() {
  String h;
  h+="<!DOCTYPE html><html><head><meta charset='utf-8'>";
  h+="<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h+="<title>MG4010 电机控制</title>";
  h+="<style>";
  h+="*{box-sizing:border-box;margin:0;padding:0;}";
  h+="body{font-family:Arial;background:#1a1a2e;color:#eee;padding:15px;}";
  h+=".container{max-width:500px;margin:0 auto;}";
  h+="h2{text-align:center;margin-bottom:15px;color:#e94560;}";
  h+=".card{background:#16213e;border-radius:10px;padding:15px;margin-bottom:12px;}";
  h+=".card-title{font-size:14px;color:#0f3460;margin-bottom:10px;font-weight:bold;"
  h+="background:#e94560;display:inline-block;padding:3px 10px;border-radius:4px;}";
  h+=".status-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;}";
  h+=".status-item{background:#0f3460;padding:8px;border-radius:6px;text-align:center;}";
  h+=".status-val{font-size:20px;font-weight:bold;color:#e94560;}";
  h+=".status-label{font-size:11px;color:#aaa;margin-top:2px;}";
  h+="label{display:block;font-size:13px;color:#aaa;margin:8px 0 4px;}";
  h+="input[type=range]{width:100%;-webkit-appearance:none;height:8px;"
  h+="background:#0f3460;border-radius:4px;outline:none;}";
  h+="input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;"
  h+="width:24px;height:24px;background:#e94560;border-radius:50%;cursor:pointer;}";
  h+="input[type=number]{width:100%;padding:8px;background:#0f3460;color:#eee;"
  h+="border:none;border-radius:6px;font-size:16px;}";
  h+=".range-val{text-align:center;color:#e94560;font-weight:bold;margin:4px 0;}";
  h+="button{width:100%;padding:12px;font-size:16px;border:none;border-radius:8px;"
  h+="cursor:pointer;margin:4px 0;font-weight:bold;transition:opacity 0.2s;}";
  h+="button:active{opacity:0.7;}";
  h+=".btn-speed{background:#e94560;color:#fff;}";
  h+=".btn-pos{background:#0f3460;color:#fff;border:2px solid #e94560;}";
  h+=".btn-stop{background:#533483;color:#fff;}";
  h+=".btn-group{display:grid;grid-template-columns:1fr 1fr;gap:8px;}";
  h+=".mode-indicator{text-align:center;padding:8px;border-radius:6px;font-weight:bold;margin:8px 0;}";
  h+=".mode-stop{background:#533483;}";
  h+=".mode-speed{background:#e94560;}";
  h+=".mode-pos{background:#0f3460;border:2px solid #e94560;}";
  h+="</style></head><body><div class='container'>";
  h+="<h2>MG4010 电机控制</h2>";

  // 状态显示
  h+="<div class='card'>";
  h+="<div class='card-title'>实时状态</div>";
  h+="<div class='status-grid'>";
  h+="<div class='status-item'><div class='status-val' id='spd'>--</div><div class='status-label'>速度 RPM</div></div>";
  h+="<div class='status-item'><div class='status-val' id='pos'>--</div><div class='status-label'>位置 °</div></div>";
  h+="</div>";
  h+="<div id='modeInd' class='mode-indicator mode-stop'>已停止</div>";
  h+="</div>";

  // 速度模式
  h+="<div class='card'>";
  h+="<div class='card-title'>速度模式</div>";
  h+="<input type='range' id='spdRange' min='0' max='3000' value='0' oninput='updSpd(this.value)'>";
  h+="<div class='range-val' id='spdVal'>0 RPM</div>";
  h+="<input type='number' id='spdNum' value='0' placeholder='输入 RPM'>";
  h+="<button class='btn-speed' onclick='setSpeed()'>启动速度模式</button>";
  h+="</div>";

  // 位置模式
  h+="<div class='card'>";
  h+="<div class='card-title'>位置模式</div>";
  h+="<input type='range' id='posRange' min='-4500' max='4500' value='0' oninput='updPos(this.value)'>";
  h+="<div class='range-val' id='posVal'>0°</div>";
  h+="<input type='number' id='posNum' value='0' placeholder='输入角度'>";
  h+="<label>最大速度 RPM</label>";
  h+="<input type='number' id='pspd' value='200' min='10' max='400'>";
  h+="<div class='btn-group'>";
  h+="<button class='btn-pos' onclick='quickPos(-360)'>-360°</button>";
  h+="<button class='btn-pos' onclick='quickPos(0)'>归零</button>";
  h+="<button class='btn-pos' onclick='quickPos(360)'>+360°</button>";
  h+="<button class='btn-pos' onclick='setPosition()'>转到位置</button>";
  h+="</div>";
  h+="</div>";

  // 停止
  h+="<div class='card'>";
  h+="<button class='btn-stop' onclick='emergencyStop()'>紧急停止</button>";
  h+="</div>";

  h+="</div>";

  // JavaScript
  h+="<script>";
  h+="function updSpd(v){document.getElementById('spdVal').textContent=v+' RPM';"
  h+="document.getElementById('spdNum').value=v;}";
  h+="function updPos(v){document.getElementById('posVal').textContent=v+'°';"
  h+="document.getElementById('posNum').value=v;}";
  h+="function setSpeed(){let v=document.getElementById('spdNum').value;"
  h+="document.getElementById('spdRange').value=v;updSpd(v);"
  h+="fetch('/mode?m=speed&spd='+v);}";
  h+="function setPosition(){let a=document.getElementById('posNum').value;"
  h+="let s=document.getElementById('pspd').value;"
  h+="document.getElementById('posRange').value=a;updPos(a);"
  h+="fetch('/mode?m=pos&ang='+a+'&spd='+s);}";
  h+="function quickPos(off){let c=parseFloat(document.getElementById('posNum').value||0);"
  h+="let n=c+off;n=Math.max(-4500,Math.min(4500,n));"
  h+="document.getElementById('posNum').value=n;"
  h+="document.getElementById('posRange').value=n;updPos(n);setPosition();}";
  h+="function emergencyStop(){fetch('/mode?m=stop');}";
  h+="function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
  h+="document.getElementById('spd').textContent=d.spd;";
  h+="document.getElementById('pos').textContent=d.pos;";
  h+="let m=d.mode,ind=document.getElementById('modeInd');";
  h+="if(m==0){ind.textContent='已停止';ind.className='mode-indicator mode-stop';}";
  h+="else if(m==1){ind.textContent='速度模式';ind.className='mode-indicator mode-speed';}";
  h+="else{ind.textContent='位置模式';ind.className='mode-indicator mode-pos';}";
  h+="}).catch(e=>{});}";
  h+="setInterval(updateStatus,300);";
  h+="</script></body></html>";
  return h;
}

void handleRoot() { server.send(200,"text/html",htmlPage()); }

void handleMode() {
  if(server.hasArg("m")) {
    String m=server.arg("m");
    if(m=="stop") {
      currentMode=MODE_STOP; targetSpeed=0;
      motorDisable();
    } else if(m=="speed") {
      targetSpeed=server.arg("spd").toInt();
      targetSpeed=constrain(targetSpeed,0,3000);
      currentMode=MODE_SPEED;
      motorEnable();
      delay(10);
      sendSpeedCommand();
    } else if(m=="pos") {
      targetAngle=server.arg("ang").toFloat();
      targetAngle=constrain(targetAngle,-4500.0f,4500.0f);
      posSpeed=server.arg("spd").toInt();
      posSpeed=constrain(posSpeed,10,400);
      currentMode=MODE_POSITION;
      motorEnable();
      delay(10);
      sendPositionCommand();
    }
  }
  server.send(200,"text/plain","OK");
}

void handleStatus() {
  String j="{";
  j+="\"spd\":"+String((int)actualSpeed)+",";
  j+="\"pos\":"+String((int)actualPosition)+",";
  j+="\"mode\":"+String((int)currentMode);
  j+="}";
  server.send(200,"application/json",j);
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== MG4010 网页控制 ===");

  pinMode(RS485_DE_RE, OUTPUT);
  rs485Rx();
  RS485.begin(BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);

  WiFi.softAP(ssid, password);
  Serial.print("WiFi: "); Serial.println(ssid);
  Serial.print("IP:   "); Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("就绪");
}

void loop() {
  server.handleClient();
  updateControl();
  updateRead();
}
