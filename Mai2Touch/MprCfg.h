#include "Adafruit_MPR121.h"
Adafruit_MPR121 mprA, mprB, mprC;
#define mprA_ADD 0x5A // mpr121 address
#define mprB_ADD 0x5B
#define mprC_ADD 0x5C

void MprReset(Adafruit_MPR121 cap) {
  cap.writeRegister(MPR121_SOFTRESET, 0x63);
}

void MprStop(Adafruit_MPR121 cap) {
  cap.writeRegister(MPR121_ECR, 0x0);
}
void MprRun(Adafruit_MPR121 cap) {
  cap.writeRegister(MPR121_ECR, B10000000 + 12);//启用的电极数
}

void MprConfig(Adafruit_MPR121 cap) {
  cap.writeRegister(MPR121_MHDR, 1);
  cap.writeRegister(MPR121_NHDR, 8);
  cap.writeRegister(MPR121_NCLR, 1);
  cap.writeRegister(MPR121_FDLR, 0);
  cap.writeRegister(MPR121_MHDF, 1);
  cap.writeRegister(MPR121_NHDF, 1);
  cap.writeRegister(MPR121_NCLF, 16);
  cap.writeRegister(MPR121_FDLF, 2);
  cap.writeRegister(MPR121_NHDT, 0);
  cap.writeRegister(MPR121_NCLT, 0);
  cap.writeRegister(MPR121_FDLT, 0);
  cap.setThresholds(10, 10); // 默认敏感度，会被 MprSetTouch 和 MprSetRelease 修改
  cap.writeRegister(MPR121_DEBOUNCE, (4 << 4) | 2);
  cap.writeRegister(MPR121_CONFIG1, 16);
  cap.writeRegister(MPR121_CONFIG2, 1 << 5);
  cap.writeRegister(MPR121_AUTOCONFIG0, 0x0B);
  cap.writeRegister(MPR121_AUTOCONFIG1, (1 << 7));
  cap.writeRegister(MPR121_UPLIMIT, 202);
  cap.writeRegister(MPR121_TARGETLIMIT, 182);
  cap.writeRegister(MPR121_LOWLIMIT, 131);
}

void MprSetTouch(uint8_t sensor, uint8_t value) {
  if (sensor < 0x41 | sensor > 0x62) {
    return;
  } else if (sensor < 0x4D) { // A1 ~ B4
    mprA.writeRegister(MPR121_TOUCHTH_0 + 2 * (sensor - 0x41), value);
  } else if (sensor < 0x59) { // B5 ~ D6
    mprB.writeRegister(MPR121_TOUCHTH_0 + 2 * (sensor - 0x4D), value);
  } else { // D7 ~ E8
    mprC.writeRegister(MPR121_TOUCHTH_0 + 2 * (sensor - 0x59), value);
  }
}

void MprSetRelease(uint8_t sensor, uint8_t value) {
  if (sensor < 0x41 | sensor > 0x62) {
    return;
  } else if (sensor < 0x4D) { // A1 ~ B4
    mprA.writeRegister(MPR121_RELEASETH_0 + 2 * (sensor - 0x41), value);
  } else if (sensor < 0x59) { // B5 ~ D6
    mprB.writeRegister(MPR121_RELEASETH_0 + 2 * (sensor - 0x4D), value);
  } else { // D7 ~ E8
    mprC.writeRegister(MPR121_RELEASETH_0 + 2 * (sensor - 0x59), value);
  }
}
