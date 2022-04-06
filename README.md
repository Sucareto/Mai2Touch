# Mai2Touch
使用 Arduino 制作的 mai2 触摸和按键灯。  
触摸通信数据格式可参考 [Mai2Touch-Data.txt](Mai2Touch-Data.txt)。  
按键灯程序**暂未完全实现**，数据可参考[Mai2LED-Data.txt](Mai2LED/Mai2LED-Data.txt)。  
**mpr121 支持仅简单测试支持，未完整游玩测试。**

### 待完成：  
- [x] 添加 mpr121 触摸支持
- [ ] 添加敏感度调节实现方式
- [ ] ...

### Mai2Touch 使用方法：  
- 上传程序
- 打开设备管理器，设置 Arduino 的 COM 号，1P = COM3，2P = COM4
- `mai2.ini`内，在`[AM]`下添加`DummyTouchPanel=0`
- 如果仅有 1P 触摸，可以设置`DummyTouchPanel=1`跳过 2P 触摸检测报错
- 启动游戏

### Mai2LED 使用方法：  
- 上传程序
- 打开设备管理器，设置 Arduino 的 COM 号，1P = COM21，2P = COM23
- `mai2.ini`内，在`[AM]`下添加`DummyLED=1`
- 启动游戏

### 已测试开发板：
- SparkFun Pro Micro（ATmega32U4）
- NodeMCU 1.0（ESP-12E + CP2102 & CH340），SDA=D2，SCL=D1

### 引用 & 参考：
- [mpr121操作 Adafruit_MPR121](https://github.com/adafruit/Adafruit_MPR121)
- [Mai2LED灯光数据结构参考](https://github.com/xiaopeng12138/MaiDXR/blob/6bb6d50c359bd7a7d8de964e3fed06a3e218e37e/Assets/Scripts/LedSerial.cs)
- [bitWrite64](https://forum.arduino.cc/t/bitset-only-sets-bits-from-0-to-31-previously-to-15/193385/5)
