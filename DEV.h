#include <FastLED.h>
#include "HomeSpan.h"

#define NUM_LEDS 30
#define DATA_PIN 4

struct LED : Service::LightBulb {
  SpanCharacteristic *power;
  SpanCharacteristic *brightness;
  CRGB leds[NUM_LEDS];
  CRGB color = CRGB(255, 180, 70);
  bool wasDisabled = false;

  LED () : Service::LightBulb() {
    power = new Characteristic::On(Characteristic::On::ON, true);
    brightness = new Characteristic::Brightness(20, true);

    this->wasDisabled = !this->isDisabled();

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(this->leds, NUM_LEDS);
  }

  bool isDisabled() {
    return (bool)(power->getVal() == Characteristic::On::OFF || brightness->getVal() == 0);
  }

  void disable() {
    for (int i = 0; i < NUM_LEDS; i++) {
      this->leds[i] = CRGB::Black;
    }
    FastLED.setBrightness(0);
    FastLED.show();
    this->wasDisabled = true;
  }

  void enable() {
    for (int i = 0; i < NUM_LEDS; i++) {
      this->leds[i] = color;
    }
    if (this->wasDisabled) {
      this->blink();
      this->wasDisabled = false;
    }
    FastLED.setBrightness(map(this->brightness->getNewVal(), 0, 100, 0, 255));
    FastLED.show();
  }

  void handleState() {
    if (this->isDisabled()) {
      this->disable();
    } else {
      this->enable();
    }
  }

  void blink() {
    FastLED.setBrightness(30);
    FastLED.show();
    delay(150);
    FastLED.setBrightness(10);
    FastLED.show();
    delay(50);
    FastLED.setBrightness(50);
    FastLED.show();
    delay(50);
    FastLED.setBrightness(0);
    FastLED.show();
    delay(50);
    FastLED.setBrightness(map(this->brightness->getNewVal(), 0, 100, 0, 255));
    FastLED.show();
  }

  boolean update() {
    this->handleState();
    return(true);
  }
};
