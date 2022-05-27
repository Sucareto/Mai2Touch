#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_ZERO)
#pragma message "当前的开发板是 ATmega32U4 或 SAMD_ZERO"
#define SerialDevice SerialUSB
#define LED_PIN 13

#elif defined(ARDUINO_ESP8266_NODEMCU_ESP12E)
#pragma message "当前的开发板是 NODEMCU_ESP12E"
#define SerialDevice Serial
#define LED_PIN D5

#elif defined(ARDUINO_NodeMCU_32S)
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial
#define LED_PIN 13


#else
#error "未经测试的开发板，请检查串口和阵脚定义"
#endif

#include "FastLED.h"
#define NUM_LEDS 11
CRGBArray<NUM_LEDS> leds;


enum {
  LedGs8Bit = 0x31,//49
  LedGs8BitMulti = 0x32,//50
  LedGs8BitMultiFade = 0x33,//51
  LedFet = 0x39,//57
  LedGsUpdate = 0x3C,//60
  LedDirect = 0x82,
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
      struct { //LedGs8Bit
        uint8_t index;
        uint8_t r;
        uint8_t g;
        uint8_t b;
      };
      struct { //LedGs8BitMulti,LedGs8BitMultiFade
        uint8_t start;
        uint8_t end;//length
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
  leds(0, 7) = 0xFFFFFF;
  FastLED.delay(1000);
}

unsigned long fade_start, fade_end, progress;
uint8_t led_start, led_end;
CRGB fade_prev, fade_taget;

void loop() {
  switch (packet_read()) {
    case LedGs8Bit:
      leds[req.index] = CRGB(req.r, req.g, req.b);
      break;

    case LedGs8BitMulti:
      if (req.end == 0x20) {
        req.end = NUM_LEDS;
      }
      leds(req.start, req.end - 1) = CRGB(req.mr, req.mg, req.mb);
      fade_prev = CRGB(req.mr, req.mg, req.mb);
      break;

    case LedGs8BitMultiFade:
      fade_taget = CRGB(req.mr, req.mg, req.mb);
      fade_start = millis();
      fade_end = fade_start + (4095 / req.speed * 8);
      led_start = req.start;
      led_end = req.end - 1;
      leds(led_start, led_end) = CRGB(req.mr, req.mg, req.mb);
      break;

    case LedFet://框体灯，只有白色，值代表亮度，会多次发送实现渐变，需要立刻刷新
      leds[8] = blend(0x000000, 0xFFFFFF, req.BodyLed);
      leds[9] = blend(0x000000, 0xFFFFFF, req.ExtLed);//same as BodyLed
      leds[10] = blend(0x000000, 0xFFFFFF, req.SideLed);//00 or FF
      FastLED.show();
      break;

    case LedGsUpdate://提交灯光数据
      FastLED.show();
      break;
  }

  if (millis() > fade_end)return;
  progress = map(millis(), fade_start, fade_end, 0, 255);
  leds(led_start, led_end) = blend(fade_taget, fade_prev, progress);
  FastLED.show();
}
