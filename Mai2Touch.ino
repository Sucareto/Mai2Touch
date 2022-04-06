//#define SerialDevice SerialUSB //32u4,samd21
#define SerialDevice Serial //esp8266

// bitWrite 不支持 uint64_t，以下定义来自 https://forum.arduino.cc/t/bitset-only-sets-bits-from-0-to-31-previously-to-15/193385/5
#define bitSet64(value, bit) ((value) |= (bit<32?1UL:1ULL) <<(bit))
#define bitClear64(value, bit) ((value) &= ~(bit<32?1UL:1ULL) <<(bit))
#define bitWrite64(value, bit, bitvalue) (bitvalue ? bitSet64(value, bit) : bitClear64(value, bit))


#include "Adafruit_MPR121.h"//mpr121定义
Adafruit_MPR121 mprA, mprB, mprC;

uint8_t packet[6];
uint8_t len = 0;//当前接收的包长度
#define sA1(x) bitWrite(TPD, 0, x)//设置 sensor
#define sB1(x) bitWrite(TPD, 8, x)
#define sC1(x) bitWrite(TPD, 16, x)
#define sD1(x) bitWrite(TPD, 18, x)
#define sE1(x) bitWrite(TPD, 26, x)
enum {
  commandRSET  = 0x45,//E
  commandHALT  = 0x4C,//L
  commandSTAT  = 0x41,//A
  commandRatio = 0x72,//r
  commandSens  = 0x6B,//k
};

bool Conditioning = true;
void setup() {
  SerialDevice.begin(9600);
  SerialDevice.setTimeout(0);
  uint8_t TOUCH = 12;//按下敏感度
  uint8_t RELEASE = 10;//松开敏感度
  mprA.begin(0x5A, &Wire, TOUCH, RELEASE);
  mprB.begin(0x5C, &Wire, TOUCH, RELEASE);
  mprC.begin(0x5B, &Wire, TOUCH, RELEASE);
  Wire.setClock(800000);
}

void loop() {
  Recv();
  Conditioning ? void() : TouchSend();//只有不处于设定模式时才发送触摸数据
}

void cmd_RSET() {//Reset
  uint8_t CONFIG1 = 0x10;//电流
  uint8_t CONFIG2 = 0x02;//延迟
  mprA.writeRegister(MPR121_SOFTRESET, 0x63);//MprReset
  mprA.writeRegister(MPR121_CONFIG1, CONFIG1);
  mprA.writeRegister(MPR121_CONFIG2, CONFIG2);
  mprA.writeRegister(MPR121_ECR, 0x0);//MprStop

  mprB.writeRegister(MPR121_SOFTRESET, 0x63);//MprReset
  mprB.writeRegister(MPR121_CONFIG1, CONFIG1);
  mprB.writeRegister(MPR121_CONFIG2, CONFIG2);
  mprB.writeRegister(MPR121_ECR, 0x0);//MprStop

  mprC.writeRegister(MPR121_SOFTRESET, 0x63);//MprReset
  mprC.writeRegister(MPR121_CONFIG1, CONFIG1);
  mprC.writeRegister(MPR121_CONFIG2, CONFIG2);
  mprC.writeRegister(MPR121_ECR, 0x0);//MprStop


}
void cmd_HALT() {//Start Conditioning Mode
  mprA.writeRegister(MPR121_ECR, 0x0);//MprStop
  mprB.writeRegister(MPR121_ECR, 0x0);
  mprC.writeRegister(MPR121_ECR, 0x0);
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
  mprA.writeRegister(MPR121_ECR, B10000000 + 12);//MprRun
  mprB.writeRegister(MPR121_ECR, B10000000 + 12);
  mprC.writeRegister(MPR121_ECR, B10000000 + 12);
}

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

  // 高级方法，读取每个触摸点的 baselineData 和 filteredData，可以单独设置敏感度过滤
  //  for (uint8_t i = 0; i < 12; i++) {
  //    bitWrite64(TouchData, i, (mprA.baselineData(i) - mprA.filteredData(i)) > 50);//另外一种检测方法
  //  }

  SerialDevice.write('(');
  for (uint8_t r = 0; r < 7; r++) {
    SerialDevice.write(TouchData & B11111);
    TouchData >>= 5;
  }
  SerialDevice.write(')');
}
