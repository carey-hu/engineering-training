# 01 LED 闪烁

## 概述

这是学习路线的第一个硬件闭环实验，用于验证 Arduino IDE 环境配置、板卡选择、COM 口识别和程序上传流程是否正确。

**本例程不需要接线**，使用开发板自带的板载 WS2812B RGB 彩灯（GPIO48）。

---

## 文件说明

**文件名：** `01_blink_onboard_led.ino`

**适用开发板：** 板载 WS2812B RGB 彩灯（GPIO48）

**特点：**
- 循环显示红、绿、蓝三种颜色
- 需要安装 `Adafruit NeoPixel` 库
- 适合大多数新款 ESP32-S3 开发板

---

## 使用步骤

### 1. 安装库

打开 Arduino IDE，按以下步骤安装依赖库：

1. 菜单栏：**工具 → 管理库...**
2. 搜索框输入：`Adafruit NeoPixel`
3. 找到 `Adafruit NeoPixel by Adafruit`
4. 点击**安装**
5. 安装完成后关闭库管理器

### 2. 上传程序

1. 打开 `01_blink_onboard_led.ino`
2. 选择正确的板卡和 COM 口
3. 点击上传按钮

### 3. 观察现象

上传成功后，板载 RGB 彩灯会循环显示：
- **红色** 1 秒
- **绿色** 1 秒
- **蓝色** 1 秒
- **熄灭** 1 秒
- 循环重复

---

## 完成标志

完成后你应该能确认：

- [x] Arduino IDE 上传成功（底部显示 "Done uploading"）
- [x] 板载 RGB 彩灯按预期循环显示三种颜色
- [x] 你能区分"上传成功"（程序已写入开发板）和"程序正在运行"（开发板执行代码）

---

## 故障排查

### RGB 彩灯不亮

1. **确认已安装库**：检查是否安装了 `Adafruit NeoPixel` 库
2. **检查上传**：IDE 底部是否显示 "Done uploading"
3. **按复位键**：按开发板上的 EN 或 RST 键
4. **检查 COM 口**：确认在 Arduino IDE 中选择了正确的 COM 口
5. **检查板卡**：确认选择的是 `ESP32S3 Dev Module`
6. **降低上传速度**：工具 → Upload Speed → 改为 `115200`
7. **按住 BOOT 上传**：按住开发板上的 BOOT 键，点击上传，等进度条出现后松开

### 如果你的开发板是 GPIO2 普通 LED

部分开发板使用的是 GPIO2 的普通 LED（非 WS2812B），需要修改代码：

1. 删除第一行：`#include <Adafruit_NeoPixel.h>`
2. 删除所有 RGB 相关配置和对象创建
3. 在 `setup()` 中改为：
   ```cpp
   pinMode(2, OUTPUT);
   ```
4. 在 `loop()` 中改为：
   ```cpp
   digitalWrite(2, HIGH);  // 点亮
   delay(500);
   digitalWrite(2, LOW);   // 熄灭
   delay(500);
   ```

---

## 扩展练习

### 1. 改变颜色

修改 `rgb.Color(R, G, B)` 的值，尝试更多颜色：

- **黄色**：`rgb.Color(255, 255, 0)`
- **紫色**：`rgb.Color(255, 0, 255)`
- **青色**：`rgb.Color(0, 255, 255)`
- **白色**：`rgb.Color(255, 255, 255)`
- **橙色**：`rgb.Color(255, 128, 0)`

### 2. 调整亮度

修改 `BRIGHTNESS` 的值（0-255）：

- `BRIGHTNESS = 10`：很暗，适合夜间
- `BRIGHTNESS = 50`：中等亮度（默认）
- `BRIGHTNESS = 255`：最亮（刺眼，慎用）

### 3. 改变闪烁速度

修改 `delay()` 的值：

- `delay(500)`：0.5 秒
- `delay(100)`：0.1 秒（快闪）
- `delay(2000)`：2 秒（慢闪）

### 4. 渐变效果（进阶）

尝试让 RGB 彩灯在颜色间平滑过渡：

```cpp
void loop() {
  // 红色渐变到绿色
  for (int i = 0; i <= 255; i++) {
    rgb.setPixelColor(0, rgb.Color(255-i, i, 0));
    rgb.show();
    delay(5);
  }
  
  // 绿色渐变到蓝色
  for (int i = 0; i <= 255; i++) {
    rgb.setPixelColor(0, rgb.Color(0, 255-i, i));
    rgb.show();
    delay(5);
  }
  
  // 蓝色渐变到红色
  for (int i = 0; i <= 255; i++) {
    rgb.setPixelColor(0, rgb.Color(i, 0, 255-i));
    rgb.show();
    delay(5);
  }
}
```

---

## 下一步

完成本例程后，继续学习：
- [02_串口调试](../02_串口调试/02_serial_print.ino)：学会用串口监视器查看程序状态

---

[返回学习路线](../../教程文档/00_learning_path.md) · [返回首页](../../README.md)
