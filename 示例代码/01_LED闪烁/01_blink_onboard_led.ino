/*
  ESP32-S3 示例 01：板载 WS2812B RGB 彩灯闪烁

  【学习目标】
  1. 理解 Arduino 程序的基本结构（setup 和 loop）
  2. 学会使用第三方库控制智能 RGB LED
  3. 验证 Arduino IDE 上传流程和板卡配置

  【硬件接线】
  本例程使用板载 WS2812B RGB 彩灯，不需要接线。
  - 板载 RGB 彩灯连接到 GPIO48
  - 开发板上电后，彩灯会自动供电

  【库依赖】
  本例程需要安装 Adafruit_NeoPixel 库：
  1. 打开 Arduino IDE 菜单：工具 → 管理库...
  2. 搜索 "Adafruit NeoPixel"
  3. 安装 "Adafruit NeoPixel by Adafruit" 库
  4. 安装完成后关闭库管理器窗口

  【预期现象】
  上传成功后，板载 RGB 彩灯会循环闪烁不同颜色：
  - 红色 1 秒 → 绿色 1 秒 → 蓝色 1 秒 → 熄灭 1 秒 → ...
  - 持续循环，不会停止

  【故障排查】
  如果 RGB 彩灯不亮：
  1. 检查上传是否成功（IDE 底部显示 "Done uploading"）
  2. 确认已安装 Adafruit_NeoPixel 库
  3. 按一下开发板的 EN 或 RST 键，让程序重新运行
  4. 确认板卡设置中 "USB CDC On Boot" 为 Enabled

  【其他板型适配】
  如果你的开发板是 GPIO2 的普通 LED（非 WS2812B），请修改为：
  - 删除 #include <Adafruit_NeoPixel.h>
  - 在 setup() 中使用：pinMode(2, OUTPUT);
  - 在 loop() 中使用：digitalWrite(2, HIGH); delay(500); digitalWrite(2, LOW); delay(500);
*/

#include <Adafruit_NeoPixel.h>  // 引入 WS2812B 控制库

const int RGB_PIN = 48, NUM_PIXELS = 1, BRIGHTNESS = 50;  // 引脚48，1个灯珠，亮度50

Adafruit_NeoPixel rgb(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);  // 创建彩灯对象

void showColor(uint8_t r, uint8_t g, uint8_t b, int delayMs) {  // 显示指定颜色并延时
  rgb.setPixelColor(0, rgb.Color(r, g, b));  // 设置第0个灯珠的RGB颜色
  rgb.show();  // 刷新显示
  delay(delayMs);  // 延时指定毫秒数
}

void setup() {  // 初始化函数，上电运行一次
  rgb.begin();  // 初始化彩灯
  rgb.setBrightness(BRIGHTNESS);  // 设置全局亮度
  rgb.show();  // 初始状态为熄灭
}

void loop() {  // 主循环函数，持续执行
  showColor(255, 0, 0, 1000);    // 红色亮1秒
  showColor(0, 255, 0, 1000);    // 绿色亮1秒
  showColor(0, 0, 255, 1000);    // 蓝色亮1秒
  showColor(0, 0, 0, 1000);      // 熄灭1秒
}
