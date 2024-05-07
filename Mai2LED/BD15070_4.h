#include "FastLED.h"
#define NUM_LEDS 11
CRGBArray<NUM_LEDS> leds;

enum {
  Sync = 0xE0,
  Marker = 0xD0,

  // ResetCommand = 0x10,
  // SetTimeout = 0x11,
  SetLedGs8Bit = 0x31,
  SetLedGs8BitMulti = 0x32,
  SetLedGs8BitMultiFade = 0x33,
  SetLedFet = 0x39,
  SetDcUpdate = 0x3B,
  SetLedGsUpdate = 0x3C,
  SetDc = 0x3F,
  SetEEPRom = 0x7B,
  GetEEPRom = 0x7C,
  SetEnableResponse = 0x7D,
  SetDisableResponse = 0x7E,
  // SetLedDirect = 0x82,
  GetBoardInfo = 0xF0,
  GetBoardStatus = 0xF1,
  GetFirmSum = 0xF2,
  GetProtocolVersion = 0xF3,
  // SetBootMode = 0xFD,

  AckStatus_Ok = 0x01,
  AckStatus_SumError = 0x02,
  AckStatus_ParityError = 0x03,
  AckStatus_FramingError = 0x04,
  AckStatus_OverRunError = 0x05,
  AckStatus_RecvBfOverFlow = 0x06,
  AckStatus_Invalid = 0xFF,

  AckReport_Ok = 0x01,
  AckReport_Busy = 0x02,
  AckReport_CommandUnknown = 0x03,
  AckReport_ParamError = 0x04,
  AckReport_Invalid = 0xFF
};

typedef union {
  uint8_t buffer[39];
  struct {
    struct {
      // uint8_t sync;
      uint8_t dstNodeID;
      uint8_t srcNodeID;
      uint8_t length;
      uint8_t command;
    };
    union {
      uint8_t timeout;  // SetTimeout
      struct {          // SetLedGs8Bit
        uint8_t index;
        uint8_t color[3];
      };
      struct {  // SetLedGs8BitMulti, SetLedGs8BitMultiFade, SetDc
        uint8_t start;
        uint8_t end;  //length
        uint8_t skip;
        uint8_t Multi_color[3];
        uint8_t speed;  // SetDc no exist
      };
      struct {  // SetLedFet
        uint8_t BodyLed;
        uint8_t ExtLed;
        uint8_t SideLed;
      };
      struct {  // SetEEPRom
        uint8_t Set_adress;
        uint8_t writeData;
      };
      uint8_t Get_adress;           // GetEEPRom
      uint8_t Direct_color[11][3];  // SetLedDirect
    };
    // uint8_t sum;
  };
} PacketReq;

typedef union {
  uint8_t buffer[12];
  struct {
    struct {
      // uint8_t sync;
      uint8_t dstNodeID;
      uint8_t srcNodeID;
      uint8_t length;
      uint8_t status;
      uint8_t command;
      uint8_t report;
    };
    union {
      uint8_t eepData;  // GetEEPRom
      struct {          // GetBoardInfo
        uint8_t boardNo[9];
        uint8_t firmRevision;
      };
      struct {  // GetBoardStatus
        uint8_t timeoutStat;
        uint8_t timeoutSec;
        uint8_t pwmIo;
        uint8_t fetTimeout;
      };
      struct {  // GetFirmSumCommand
        uint8_t sum_upper;
        uint8_t sum_lower;
      };
      struct {  // GetProtocolVersionCommand
        uint8_t appliMode;
        uint8_t major;
        uint8_t minor;
      };
    };
    // uint8_t sum;
  };
} PacketAck;

static PacketReq req;
static PacketAck ack;

static uint8_t packet_read() {
  static uint8_t r_len, r, checksum;
  static bool escape = false;
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == Sync) {
      r_len = 0;
      checksum = 0;
      continue;
    }
    if (r == Marker) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }

    if (r_len == req.length + 3) {
      if (checksum == r) {
        return req.command;
      }
      return AckStatus_SumError;
    }
    req.buffer[r_len++] = r;
    checksum += r;
  }
  return 0;
}

void packet_write() {
  if (ack.command == 0) {
    return;
  }
  uint8_t checksum = 0, w_len = 0;
  SerialDevice.write(Sync);
  while (w_len < ack.length + 3) {
    uint8_t w;
    w = ack.buffer[w_len++];
    checksum += w;
    if (w == Sync || w == Marker) {
      SerialDevice.write(Marker);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
  }
  SerialDevice.write(checksum);
  ack.command = 0;
}

static uint8_t dummyEEPRom[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void ack_init(uint8_t length = 0, uint8_t status = AckStatus_Ok, uint8_t report = AckReport_Ok) {
  ack.dstNodeID = req.srcNodeID;
  ack.srcNodeID = req.dstNodeID;
  ack.length = 3 + length;
  ack.status = status;
  ack.command = req.command;
  ack.report = report;
}

void setLedGs8Bit() {
  leds[req.index] = CRGB(req.color[0], req.color[1], req.color[2]);
  ack_init();
}

long StartFadeTime, EndFadeTime, progress;
uint8_t StartFadeLed, EndFadeLed;
CRGB StartFadeColor, EndFadeColor;
bool NeedFade = false;

void setLedGs8BitMulti() {
  if (req.end == 0x20) {  // SetLedDataAllOff
    req.end = NUM_LEDS;
  }
  StartFadeColor = CRGB(req.Multi_color[0], req.Multi_color[1], req.Multi_color[2]);
  leds(req.start, req.end - 1) = StartFadeColor;
  NeedFade = false;
  ack_init();
}

void setLedGs8BitMultiFade() {
  EndFadeColor = CRGB(req.Multi_color[0], req.Multi_color[1], req.Multi_color[2]);
  StartFadeTime = millis();
  EndFadeTime = StartFadeTime + (4095 / req.speed * 8);
  StartFadeLed = req.start;
  EndFadeLed = req.end - 1;
  NeedFade = true;
  ack_init();
}

void setLedFet() {  // 框体灯，只有白色，值代表亮度，会多次发送实现渐变，需要立刻刷新
#if NUM_LEDS > 8
  leds[8] = blend(0x000000, 0xFFFFFF, req.BodyLed);
  leds[9] = blend(0x000000, 0xFFFFFF, req.ExtLed);    //same as BodyLed
  leds[10] = blend(0x000000, 0xFFFFFF, req.SideLed);  //00 or FF
  FastLED.show();
#endif
  ack_init();
}

void getBoardInfo() {
  memcpy(ack.boardNo, "15070-04\xFF\x90\x00\x30", 12);
  ack.firmRevision = 144;
  ack_init(10);
}

void getBoardStatus() {  // unknown
  ack.timeoutStat = 0;
  ack.timeoutSec = 1;
  ack.pwmIo = 0;
  ack.fetTimeout = 0;
  ack_init(4);
}

void getFirmSum() {  // unknown
  ack.sum_upper = 0;
  ack.sum_lower = 0;
  ack_init(2);
}

void getProtocolVersion() {  // unknown
  ack.appliMode = 1;         // IsNeedFirmUpdate = false
  ack.major = 1;
  ack.minor = 1;
  ack_init(3);
}