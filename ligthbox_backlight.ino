#include <FastLED.h>
#include "HomeSpan.h"
#include "DEV.h"

#include <WiFi.h>
#include <WebServer.h>

#define BUTTON_PIN 2

WebServer server(80);

LED *lightbox = nullptr;

void myApHook(){
  Serial.println("HomeSpan in AP/setup mode");

  WiFi.softAP("PhotoBox Setup");
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  String styles = "";
  styles += "* { padding: 0; margin: 0; box-sizing: border-box; }";
  styles += "html, body { min-height: 100vh; }";
  styles += "body { font-family: sans-serif; font-size: 20px; line-height: 30px; display: flex; align-items: center; justify-content: center; flex-direction: column; text-align: center; }";
  styles += "h2 { font-size: 50px; line-height: 1; margin-bottom: 25px; }";
  styles += "select, input, button { appearance: button; border: 1px solid black; padding: 10px 20px; font-size: 24px; background: transparent; border-radius: 8px; width: 300px; }";
  styles += "button { background: #93f179 }";
  styles += "form { display: flex; flex-direction: column; gap: 20px; }";
  styles += "a { color: black; }";

  // WebServer
  server.on("/", [styles](){
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

    String options;
    for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        options += "<option value='" + ssid + "'>" + ssid + "</option>";
    }

    String html = "<html><head><title>Wi-Fi Setup</title>";
    html += "<style>" + styles + "</style></head><body>";
    html += "<h2>Wi-Fi Setup</h2><form method='POST' action='/save'>";
    html += "<select name='ssid'>" + options + "</select>";
    html += "<input name='pass' placeholder='Password'>";
    html += "<button>Save</button></form></body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, [styles](){
    Serial.print("/save handler");
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("Trying to connect to Wi-Fi");

    int timeout = 0;
    const int maxTimeout = 10000; // 10 секунд
    while(WiFi.status() != WL_CONNECTED && timeout < maxTimeout){
      delay(100);
      Serial.print(".");
      timeout += 100;
    }
    Serial.println();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Wi-Fi connected successfully!");
      homeSpan.setWifiCredentials(ssid.c_str(), pass.c_str());

      String html = "<html><head><title>Wi-Fi is set up</title><style>" + styles + "</style></head><body>";
      html += "<h2>Wi-Fi is set up</h2><p>The device is rebooting. You can close this page and proceed with Home.app pairing</p>";
      html += "</body></html>";
      server.send(200, "text/html", html);
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("Failed to connect!");
      String html = "<html><head><title>Wi-Fi is set up</title><style>" + styles + "</style></head><body>";
      html += "<h2>Failed to connect to Wi-Fi</h2><a href='/'>Try again</a>";
      html += "</body></html>";
      server.send(200, "text/html", html);
    }
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(48, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  homeSpan.enableAutoStartAP();

  homeSpan.setPairingCode("23092019");
  homeSpan.begin(Category::Lighting, "PhotoBox by Lunyov");
  homeSpan.setApFunction(myApHook);

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
      int brightness = nextStep * STEP_SIZE;
      lightbox->brightness->setVal(brightness);
      lightbox->power->setVal(brightness ? 1 : 0);
    }
    prevVal = val;
  } else {
    if (val == HIGH && lastTimeHigh != 0 && (millis() - lastTimeHigh) > 1000) {
      lastTimeHigh = 0;
      lightbox->brightness->setVal(0);
        lightbox->power->setVal(0);
    }
  }
  lightbox->handleState();

  server.handleClient();
}
