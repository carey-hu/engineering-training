# 08 电机使用指南

## 1. MG4010-i10 电机介绍

MG4010-i10 是一款带编码器的直流无刷电机，内置驱动器和控制器，支持 RS485 通信。

主要参数：

```text
工作电压：12V ~ 24V
通信方式：RS485
波特率：115200（出厂默认）
电机 ID：1 ~ 127（出厂默认 ID=1）
减速比：i10（1:10）
编码器分辨率：16384 脉冲/圈（输出轴）
```

适合用于：

- 机械臂关节
- 云台控制
- 精密定位应用

---

## 2. RS485 接线方法

### 2.1 电机端接线

MG4010-i10 引出 4 根线：

```text
红色   → 电源正（12V~24V）
黑色   → 电源负（GND）
黄色   → RS485-A
绿色   → RS485-B
```

### 2.2 ESP32-S3 连接方式

使用 MAX485 或 SP3485 模块：

```text
MAX485 模块         ESP32-S3
---------------------------------
VCC      →         3.3V
GND      →         GND
RO       →         GPIO18（RX）
DI       →         GPIO17（TX）
DE/RE    →         GPIO16（方向控制）
A        →         电机黄色线（RS485-A）
B        →         电机绿色线（RS485-B）
```

### 2.3 供电注意事项

```text
电机供电：12V~24V 独立电源（推荐 24V 2A 以上）
ESP32 供电：5V USB 或独立 5V 电源
共地：电机 GND 和 ESP32 GND 必须连接
```

**重要：** 电机和 ESP32 不能共用同一个电源，否则可能因电流波动导致 ESP32 复位或死机。

---

## 3. 协议基础（简化版）

### 3.1 通信格式

MG4010-i10 使用标准 Modbus-RTU 协议：

```text
[设备ID] [功能码] [数据...] [CRC校验低] [CRC校验高]
```

示例：读取电机当前位置

```text
发送：01 03 00 74 00 02 85 CA
      │  │  │     │  │  └─ CRC
      │  │  │     │  └─ 读取 2 个寄存器
      │  │  │     └─ 寄存器地址 0x0074
      │  │  └─ 功能码 03（读保持寄存器）
      │  └─ 电机 ID=1
```

### 3.2 常用寄存器地址

```text
寄存器地址    功能              读写
------------------------------------------
0x0074       当前位置          只读
0x007A       目标位置          读写
0x007C       运行模式          读写
0x007D       最大速度          读写
0x0080       使能状态          读写
```

### 3.3 运行模式

```text
模式值    说明
--------------------------
0        位置模式（常用）
1        速度模式
2        电流（力矩）模式
```

### 3.4 使用示例

控制电机转到指定位置：

```cpp
// 1. 使能电机
writeRegister(motorID, 0x0080, 1);

// 2. 设置位置模式
writeRegister(motorID, 0x007C, 0);

// 3. 设置最大速度（单位：0.01度/秒）
writeRegister(motorID, 0x007D, 5000);  // 50度/秒

// 4. 写入目标位置（单位：0.01度）
writeRegister(motorID, 0x007A, 9000);  // 90度
```

---

## 4. 常见问题

### 4.1 通信失败

现象：

```text
电机不响应
读取寄存器超时
返回数据 CRC 校验错误
```

排查顺序：

```text
1. 检查 RS485 A/B 是否接反
2. 确认电机 ID 是否正确（默认 ID=1）
3. 确认波特率是否匹配（默认 115200）
4. 测量 RS485 模块供电是否正常（3.3V）
5. 检查 DE/RE 引脚控制逻辑是否正确
6. 多个电机时，确认每个电机 ID 唯一
```

调试技巧：

```cpp
// 打开调试输出
Serial.println("Send: 01 03 00 74 00 02 85 CA");
Serial.print("Recv: ");
for(int i=0; i<len; i++) {
  Serial.printf("%02X ", buf[i]);
}
Serial.println();
```

### 4.2 供电问题

现象：

```text
电机转动时 ESP32 复位
电机抖动或无力
电机偶尔失控
```

处理：

```text
1. 使用独立电源给电机供电（24V 2A 以上）
2. 电机 GND 和 ESP32 GND 必须连接
3. 电机电源线加粗，减少压降
4. 电源输出端并联 100uF~470uF 电解电容
5. 不要用 USB 电源给电机供电
```

建议供电方案：

```text
24V 电源
  ├─ 电机（红黑线）
  └─ DC-DC 降压模块（24V→5V）
       └─ ESP32（VIN 或 USB）
```

### 4.3 电机 ID 冲突

现象：

```text
多个电机时，控制一个电机，其他电机也动
```

处理：

```text
1. 逐个连接电机，修改 ID
2. 使用厂家提供的调试工具修改 ID
3. 或通过代码修改（写 0x0001 寄存器）
```

示例代码：

```cpp
// 修改电机 ID：将 ID=1 改为 ID=2
writeRegister(1, 0x0001, 2);
delay(100);
// 断电重启电机生效
```

### 4.4 电机不转或抖动

现象：

```text
发送命令后电机不响应
电机抖动但不转动
电机转动角度不准确
```

排查：

```text
1. 确认已发送使能命令（寄存器 0x0080 = 1）
2. 检查运行模式是否正确（位置模式 = 0）
3. 检查最大速度设置是否过小
4. 电机供电电压是否低于 12V
5. 负载是否过大导致堵转
```

建议初始化流程：

```cpp
void initMotor(uint8_t id) {
  // 1. 清除错误
  writeRegister(id, 0x0082, 0);
  delay(50);
  
  // 2. 设置位置模式
  writeRegister(id, 0x007C, 0);
  delay(50);
  
  // 3. 设置合理速度
  writeRegister(id, 0x007D, 3000);
  delay(50);
  
  // 4. 使能电机
  writeRegister(id, 0x0080, 1);
  delay(100);
  
  Serial.println("Motor initialized");
}
```

### 4.5 CRC 校验错误

现象：

```text
发送命令后电机无响应
或返回数据格式错误
```

常见原因：

```text
1. CRC 计算函数有误
2. 高低字节顺序错误
3. 数据帧长度计算错误
```

标准 CRC16-Modbus 计算：

```cpp
uint16_t calculateCRC(uint8_t* data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for(uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for(uint8_t j = 0; j < 8; j++) {
      if(crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc;  // 返回值：低字节在前，高字节在后
}
```

---

## 5. 快速测试代码

```cpp
#include <HardwareSerial.h>

HardwareSerial RS485(1);  // 使用 UART1
#define DE_PIN 16

void setup() {
  Serial.begin(115200);
  RS485.begin(115200, SERIAL_8N1, 18, 17);  // RX=18, TX=17
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);  // 默认接收模式
  
  delay(2000);
  Serial.println("Motor Test Start");
  
  // 使能电机
  uint8_t cmd[] = {0x01, 0x06, 0x00, 0x80, 0x00, 0x01, 0x48, 0x0A};
  sendCommand(cmd, 8);
  delay(500);
  
  Serial.println("Motor enabled");
}

void loop() {
  // 读取电机位置
  uint8_t readCmd[] = {0x01, 0x03, 0x00, 0x74, 0x00, 0x02, 0x85, 0xCA};
  sendCommand(readCmd, 8);
  
  uint8_t response[16];
  int len = receiveResponse(response, 16, 100);
  
  if(len > 0) {
    Serial.print("Response: ");
    for(int i=0; i<len; i++) {
      Serial.printf("%02X ", response[i]);
    }
    Serial.println();
  } else {
    Serial.println("No response");
  }
  
  delay(1000);
}

void sendCommand(uint8_t* cmd, uint8_t len) {
  digitalWrite(DE_PIN, HIGH);  // 发送模式
  delayMicroseconds(100);
  RS485.write(cmd, len);
  RS485.flush();
  delayMicroseconds(100);
  digitalWrite(DE_PIN, LOW);   // 接收模式
}

int receiveResponse(uint8_t* buf, uint8_t maxLen, uint32_t timeout) {
  uint32_t startTime = millis();
  uint8_t index = 0;
  
  while(millis() - startTime < timeout) {
    if(RS485.available()) {
      buf[index++] = RS485.read();
      if(index >= maxLen) break;
      startTime = millis();  // 收到数据后重置超时
    }
  }
  
  return index;
}
```

---

## 6. 参考资料

- MG4010-i10 用户手册（厂家提供）
- Modbus-RTU 协议标准
- 示例代码：`examples/motor_control/`

有问题可以查看示例代码或联系技术支持。
