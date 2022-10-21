#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_ZERO)
#pragma message "当前的开发板是 ATmega32U4 或 SAMD_ZERO"
#define SerialDevice SerialUSB

#elif defined(ARDUINO_ESP8266_NODEMCU_ESP12E)
#pragma message "当前的开发板是 NODEMCU_ESP12E"
#define SerialDevice Serial

#elif defined(ARDUINO_NodeMCU_32S)
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial

#else
#error "未经测试的开发板，请检查串口和阵脚定义"
#endif

// bitWrite 不支持 uint64_t，以下定义来自 https://forum.arduino.cc/t/bitset-only-sets-bits-from-0-to-31-previously-to-15/193385/5
#define bitSet64(value, bit) ((value) |= (bit<32?1UL:1ULL) <<(bit))
#define bitClear64(value, bit) ((value) &= ~(bit<32?1UL:1ULL) <<(bit))
#define bitWrite64(value, bit, bitvalue) (bitvalue ? bitSet64(value, bit) : bitClear64(value, bit))


#include "MprCfg.h"
Adafruit_MPR121 mprA, mprB, mprC;

enum {
  commandRSET  = 0x45,//E
  commandHALT  = 0x4C,//L
  commandSTAT  = 0x41,//A
  commandRatio = 0x72,//r
  commandSens  = 0x6B,//k
};
uint8_t packet[6];
bool Conditioning = true;

void setup() {
  SerialDevice.begin(9600);
  SerialDevice.setTimeout(0);
  mprA.begin(0x5A);
  mprB.begin(0x5B);
  mprC.begin(0x5C);
  Wire.setClock(800000);
}

void loop() {
  Recv();
  Conditioning ? void() : TouchSend();//只有不处于设定模式时才发送触摸数据
}

void cmd_RSET() {//Reset
  MprReset(mprA);
  MprReset(mprB);
  MprReset(mprC);
}
void cmd_HALT() {//Start Conditioning Mode
  MprStop(mprA);
  MprStop(mprB);
  MprStop(mprC);
  MprConfig(mprA);
  MprConfig(mprB);
  MprConfig(mprC);
  Conditioning = true;
}

void cmd_Ratio() {//Set Touch Panel Ratio
  SerialDevice.write('(');
  SerialDevice.write(packet[1]);//L,R
  SerialDevice.write(packet[2]);//sensor
  SerialDevice.write('r');
  SerialDevice.write(packet[4]);//Ratio
  SerialDevice.write(')');
}

void cmd_Sens() {//Set Touch Panel Sensitivity
  SerialDevice.write('(');
  SerialDevice.write(packet[1]);//L,R
  SerialDevice.write(packet[2]);//sensor
  SerialDevice.write('k');
  SerialDevice.write(packet[4]);//Sensitivity
  SerialDevice.write(')');
}

void cmd_STAT() { //End Conditioning Mode
  Conditioning = false;
  MprRun(mprA);
  MprRun(mprB);
  MprRun(mprC);
}

uint8_t len = 0;//当前接收的包长度
void Recv() {
  while (SerialDevice.available()) {
    uint8_t r = SerialDevice.read();
    if (r == '{') {
      len = 0;
    }
    if (r == '}') {
      break;
    }
    packet[len++] = r;
  }
  if (len == 5) {
    switch (packet[3]) {
      case commandRSET:
        cmd_RSET();
        break;
      case commandHALT:
        cmd_HALT();
        break;
      case commandRatio:
        cmd_Ratio();
        break;
      case commandSens:
        cmd_Sens();
        break;
      case commandSTAT:
        cmd_STAT();
        break;
    }
    len = 0;
    memset(packet, 0, 6);
  }
}

void TouchSend() {
  uint64_t TouchData = 0; //触摸数据包
  // 简单方法，从 mpr.touched() 一次读取 12个触摸点的按下状态，需要正确配置 mpr121 的各种参数值才能获取准确的状态
  TouchData = (TouchData | mprC.touched()) << 12;
  TouchData = (TouchData | mprB.touched()) << 12;
  TouchData = (TouchData | mprA.touched());

  // 高级方法，读取每个触摸点的 baselineData 和 filteredData，可以精确控制触发敏感度。因为读取和判断的耗时，延迟可能会变高
  //  #define Threshold 10 //触摸触发阈值
  // bitWrite64(TouchData, i, (int)(mprA.baselineData(i) - mprA.filteredData(i)) > Threshold);//另外一种检测方法
  //  for (uint8_t i = 0; i < 10; i++) {// E8 - D7
  //    TouchData = (TouchData | (int)(mprC.baselineData(i) - mprC.filteredData(i)) > Threshold) << 1;
  //  }
  //  for (uint8_t i = 0; i < 12; i++) {// D6 - B5
  //    TouchData = (TouchData | (int)(mprB.baselineData(i) - mprB.filteredData(i)) > Threshold) << 1;
  //  }
  //  for (uint8_t i = 0; i < 12; i++) {// B4 - A1
  //    TouchData = (TouchData | (int)(mprA.baselineData(i) - mprA.filteredData(i)) > Threshold) << 1;
  //  }
  //  TouchData >>= 1;


  SerialDevice.write('(');
  for (uint8_t r = 0; r < 7; r++) {
    SerialDevice.write((uint8_t)(TouchData & B11111));
    TouchData >>= 5;
  }
  SerialDevice.write(')');
}
