#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_ZERO)
#pragma message "当前的开发板是 ATmega32U4 或 SAMD_ZERO"
#define SerialDevice SerialUSB
#define LED_PIN 5

#elif defined(ESP8266)
#pragma message "当前的开发板是 ESP8266"
#define SerialDevice Serial
#define LED_PIN D1

#elif defined(ESP32)
#pragma message "当前的开发板是 ESP32"
#define SerialDevice Serial
#define LED_PIN 13

#else
#error "未经测试的开发板，请检查串口和针脚定义"
#endif

#include "BD15070_4.h"

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  // FastLED.setBrightness(30);
  FastLED.clear();
  SerialDevice.begin(115200);
  FastLED.showColor(0xFFFFFF);
  memset(req.buffer, 0, sizeof(req.buffer));
  memset(ack.buffer, 0, sizeof(ack.buffer));
}

void loop() {
  switch (packet_read()) {
    case AckStatus_SumError:
      ack_init(0, AckStatus_SumError, 0);
      break;
    case SetLedGs8Bit:
      setLedGs8Bit();
      break;
    case SetLedGs8BitMulti:
      setLedGs8BitMulti();
      break;
    case SetLedGs8BitMultiFade:
      setLedGs8BitMultiFade();
      break;
    case SetLedFet:
      setLedFet();
      break;
    case SetLedGsUpdate:
      FastLED.show();
      ack_init();
      break;
    case SetEEPRom:
      dummyEEPRom[req.Set_adress] = req.writeData;
      ack_init();
      break;
    case GetEEPRom:
      ack.eepData = dummyEEPRom[req.Get_adress];
      ack_init(1);
      break;
    case GetBoardInfo:
      getBoardInfo();
      break;
    case GetBoardStatus:
      getBoardStatus();
      break;
    case GetFirmSum:
      getFirmSum();
      break;
    case GetProtocolVersion:
      getProtocolVersion();
      break;
    case SetEnableResponse:
    case SetDisableResponse:
    case 0:
      break;
    default:
      ack_init();
  }
  packet_write();

  if (!NeedFade) return;
  if (millis() > EndFadeTime) {
    progress = 255;
    NeedFade = false;
  } else {
    progress = map(millis(), StartFadeTime, EndFadeTime, 0, 255);
  }
  leds(StartFadeLed, EndFadeLed) = blend(StartFadeColor, EndFadeColor, progress);
  FastLED.show();
}
