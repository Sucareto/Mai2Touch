//#define SerialDevice SerialUSB //32u4,samd21
#define SerialDevice Serial //esp8266

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
uint64_t TPD = 0;//触摸数据包

bool ConditioningMode = true;
void setup() {
  pinMode(D3, INPUT);//调试用
  SerialDevice.begin(9600);
  SerialDevice.setTimeout(0);
}

void loop() {
  Recv();
  ConditioningMode ? void() : TouchSend();//只有不处于设定模式时才发送触摸数据
}

void cmd_RSET() {//Reset
  delay(1000);
}
void cmd_HALT() {//StartConditioningMode
  delay(1000);
  ConditioningMode = true;
}

void cmd_Ratio() {//SetSenserRatio
  SerialDevice.write('(');
  SerialDevice.write(packet[1]);//L,R
  SerialDevice.write(packet[2]);//sensor
  SerialDevice.write('r');
  SerialDevice.write(packet[4]);//Ratio
  SerialDevice.write(')');
}

void cmd_Sens() {//SetSenserSensitivity
  SerialDevice.write('(');
  SerialDevice.write(packet[1]);//L,R
  SerialDevice.write(packet[2]);//sensor
  SerialDevice.write('k');
  SerialDevice.write(packet[4]);//Sensitivity
  SerialDevice.write(')');
}

void cmd_STAT() { //EndConditioningMode
  ConditioningMode = false;
  sA1(1);
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

void TouchSend() {//调试用,顺序移动触摸数据
  if (digitalRead(D3)) {
    return;
  }
  SerialDevice.write('(');
  SerialDevice.write(TPD & B11111);
  SerialDevice.write((TPD >> 5)& B11111);
  SerialDevice.write((TPD >> 10)& B11111);
  SerialDevice.write((TPD >> 15)& B11111);
  SerialDevice.write((TPD >> 20)& B11111);
  SerialDevice.write((TPD >> 25)& B11111);
  SerialDevice.write((TPD >> 30)& B11111);
  SerialDevice.write(')');
  TPD < 0x200000000 ? TPD <<= 1 : TPD = 1;
  delay(500);
}
