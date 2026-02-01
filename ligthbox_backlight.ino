#include <FastLED.h>
#include "HomeSpan.h"
#include "DEV.h"

#define BUTTON_PIN 2

LED *lightbox = nullptr;

void setup() {
  Serial.begin(115200);
  pinMode(48, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  homeSpan.enableAutoStartAP();

  homeSpan.setPairingCode("11122333");
  homeSpan.begin(Category::Lighting, "Sunify Lightbox");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
    lightbox = new LED();
}

int prevVal = LOW;
int lastTimeHigh = 0;
int BRIGTHNESS_STEPS = 5;
int STEP_SIZE = 100 / BRIGTHNESS_STEPS;
void loop() {
  homeSpan.poll();
  int val = digitalRead(BUTTON_PIN);
  if (val != prevVal) {
    lastTimeHigh = millis();
    if (val == HIGH) {
      int currentStep = lightbox->brightness->getVal() / STEP_SIZE;
      int nextStep = currentStep + 1;
      if (nextStep > BRIGTHNESS_STEPS) {
        nextStep = 0;
      }
      lightbox->brightness->setVal(nextStep * STEP_SIZE);
    }
    prevVal = val;
  } else {
    if (val == HIGH && lastTimeHigh != 0 && (millis() - lastTimeHigh) > 1000) {
      lastTimeHigh = 0;
      lightbox->brightness->setVal(0);
    }
  }
  lightbox->handleState();
  
}
