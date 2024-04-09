# Mai2Touch
使用 Arduino 制作的 mai2 触摸输入和 LED 控制板（837-15070-04）示例程序。  
触摸设备通信数据格式可参考 [Mai2Touch 数据分析](Mai2Touch/README.md)，实际数据参考 [Mai2Touch-Data.txt](Mai2Touch/Mai2Touch-Data.txt)  
LED 控制板通信数据格式可参考 [Mai2LED 数据分析](Mai2LED/README.md)，实际数据参考 [Mai2LED-Data.txt](Mai2LED/Mai2LED-Data.txt)  

### 待完成：  
- [ ] 收集官方设备串口数据
- [ ] 收集硬件测试结果
- [ ] 等待错误报告

### Mai2Touch 使用方法：  
- 上传程序
- 打开设备管理器，设置 Arduino 的 COM 号，1P = COM3，2P = COM4
- `mai2.ini`内，在`[AM]`下添加`DummyTouchPanel=1`或`IgnoreError=1`
  - 添加`DummyTouchPanel=1`是为了屏蔽缺少 2P 触摸设备的错误报告，在没有接 HID 触摸设备的情况下，该设置不会影响触摸状态
  - 如果发现有影响，则可以改成`DummyTouchPanel=0`然后添加`IgnoreError=1`
  - 如果有接了两个触摸设备，可以不添加
- 启动游戏

### Mai2LED 使用方法：  
- 上传程序
- 打开设备管理器，设置 Arduino 的 COM 号，1P = COM21，2P = COM23
- `mai2.ini`内，在`[AM]`下添加`DummyLED=1`(如无报错可以不加)
- 启动游戏

### 已测试开发板：
- SparkFun Pro Micro & Arduino Leonardo (ATmega32U4)
- SparkFun SAMD21 Dev Breakout & Arduino Zero (ATSAMD21G18)
- NodeMCU 1.0 (ESP-12E + CP2102 & CH340)
- NodeMCU-32S (ESP32-S + CH340)

### 引用库：
- 驱动 mpr121：[Adafruit_MPR121](https://github.com/adafruit/Adafruit_MPR121)

