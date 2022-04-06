#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_ZERO)
#pragma message "当前的开发板是 ATmega32U4 或 SAMD_ZERO"
#define SerialDevice SerialUSB
#define DATA_PIN A0

#elif defined(ARDUINO_ESP8266_NODEMCU_ESP12E)
#pragma message "当前的开发板是 NODEMCU_ESP12E"
#define SerialDevice Serial
#define DATA_PIN D5

#elif defined(ARDUINO_NodeMCU_32S)
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial
#define LED_PIN 13


#else
#error "未经测试的开发板，请检查串口和阵脚定义"
#endif

#include "FastLED.h"
#define NUM_LEDS 60
CRGBArray<NUM_LEDS> leds;

enum {
  SetLedDirectCommand = 0x82,
  SetLedFetCommand = 0x39,
  SetLedGs8BitCommand = 0x31,//49
  SetLedGs8BitMultiCommand = 0x32,//50
  SetLedGs8BitMultiFadeCommand = 0x33,//51
  SetLedGsUpdateCommand = 0x3C,
};

typedef union {
  uint8_t base[64];
  struct {
    struct {
      uint8_t dstNodeID;
      uint8_t srcNodeID;
      uint8_t length;
      uint8_t cmd;
    };
    union {
      struct { //39
        uint8_t color[11][3];//CRGB
      };
      struct { //9
        uint8_t BodyLed;
        uint8_t ExtLed;
        uint8_t SideLed;
      };
      struct { //10
        uint8_t index;//<11
        uint8_t r;
        uint8_t g;
        uint8_t b;
      };
      struct { //13
        uint8_t start;
        uint8_t end;
        uint8_t skip;
        uint8_t mr;
        uint8_t mg;
        uint8_t mb;
        uint8_t speed;
      };
    };
  };
} Packet;

static Packet req;

static uint8_t len, r, checksum;
static bool escape = false;

static uint8_t packet_read() {
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      len = 0;
      checksum = 0;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }

    if (len - 3 == req.length && checksum == r) {
      return req.cmd;
    }
    req.base[len++] = r;
    checksum += r;
  }
  return 0;
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);
  FastLED.clear();
  SerialDevice.begin(115200);
  leds(0, 8) = 0xFF00FF;
  FastLED.delay(1000);
}
int progress = 0;
uint8_t fade_ms, fade_start, fade_end;
uint32_t last_ms;
CRGB fade_prev, fade_taget;

void loop() {
  switch (packet_read()) {
    case SetLedDirectCommand:
      break;
    case SetLedFetCommand://框体灯，只有白色，值代表亮度，会多次发送实现渐变，需要立刻刷新
      //      leds(36, 38) = blend(0x000000, 0xFFFFFF, req.BodyLed);
      //      leds(39, 41) = blend(0x000000, 0xFFFFFF, req.ExtLed);
      //      leds(42, 44) = blend(0x000000, 0xFFFFFF, req.SideLed);
      //      FastLED.show();
      break;
    case SetLedGs8BitCommand://设置 req.index 号键的灯为 CRGB(req.r, req.g, req.b)
      leds(req.index * 6, (req.index + 1) * 6) = CRGB(req.r, req.g, req.b);
      break;
    case SetLedGs8BitMultiCommand://设置 req.start 到 req.end 的灯为 CRGB(req.mr, req.mg, req.mb)
      leds(req.start * 6, req.end * 6) = CRGB(req.mr, req.mg, req.mb);
      break;
    case SetLedGs8BitMultiFadeCommand://非正确实现的渐变，具体情况参考 Mai2LED-Data.txt
      fade_prev = leds(req.start, req.end);
      fade_taget = CRGB(req.mr, req.mg, req.mb);
      progress = fade_ms = 4096 / req.speed * 8;
      fade_start = req.start;
      fade_end = req.end;
      last_ms = millis();
      break;
    case SetLedGsUpdateCommand://提交灯光数据
      FastLED.show();
      break;
    case 0:
      break;
  }
  if (!progress)return;//非正确实现的渐变
  progress = fade_ms - (millis() - last_ms);
  if (progress <= 0)progress = 0;
  leds(fade_start * 4, fade_end * 4 + 4) = blend(fade_taget, fade_prev, map(progress, 0, fade_ms, 0, 255));
  FastLED.show();
}
