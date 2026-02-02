#include <FastLED.h>
#include "HomeSpan.h"
#include "DEV.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define BUTTON_PIN 2
#define RESET_PIN  5

WebServer server(80);

DNSServer dns;
const byte DNS_PORT = 53;

LED *lightbox = nullptr;

void myApHook(){
  Serial.println("HomeSpan in AP/setup mode");

  WiFi.softAP("PhotoBox Setup");
  dns.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  static String styles = "";
  styles += "* { padding: 0; margin: 0; box-sizing: border-box; }";
  styles += "html, body { min-height: 100vh; }";
  styles += "body { font-family: sans-serif; font-size: 20px; line-height: 30px; display: flex; align-items: center; justify-content: center; flex-direction: column; text-align: center; }";
  styles += "h2 { font-size: 50px; line-height: 1; margin-bottom: 25px; }";
  styles += "select, input, button { appearance: button; border: 1px solid black; padding: 10px 20px; font-size: 24px; background: transparent; border-radius: 8px; width: 300px; }";
  styles += "button { background: #93f179; color: black; }";
  styles += "form { display: flex; flex-direction: column; gap: 20px; }";
  styles += "a { color: black; }";

  static String html_meta = "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

  // WebServer
  server.on("/", [](){
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

    String html = "<html><head><title>Wi-Fi Setup</title>";
    html += html_meta;
    html += "<style>" + styles + "</style></head><body>";
    html += "<h2>Wi-Fi Setup</h2><form method='POST' action='/save'>";
    
    if (n > 0) {
      String options;
      for (int i = 0; i < n; ++i) {
          String ssid = WiFi.SSID(i);
          options += "<option value='" + ssid + "'>" + ssid + "</option>";
      }
      html += "<select name='ssid'>" + options + "</select>";
    } else {
      html += "<input name='ssid' placeholder='Wi-Fi network' />";
    }

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

      String html = "<html><head><title>Wi-Fi is set up</title><style>" + styles + "</style>" + html_meta + "</head><body>";
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

  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  homeSpan.enableAutoStartAP();

  homeSpan.setPairingCode("23092019");
  homeSpan.setControlPin(RESET_PIN);
  homeSpan.setStatusPin(48);
  homeSpan.begin(Category::Lighting, "PhotoBox by Lunyov");
  homeSpan.setApFunction(myApHook);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
    lightbox = new LED();
}

int BRIGTHNESS_STEPS = 5;
int STEP_SIZE = 100 / BRIGTHNESS_STEPS;
void cycleBrightness() {
  int currentStep = lightbox->brightness->getVal() / STEP_SIZE;
  int nextStep = currentStep + 1;
  if (nextStep > BRIGTHNESS_STEPS) {
    nextStep = 0;
  }
  int brightness = nextStep * STEP_SIZE;
  lightbox->brightness->setVal(brightness);
  lightbox->power->setVal(brightness ? 1 : 0);
}

void turnoff() {
  lightbox->brightness->setVal(0);
  lightbox->power->setVal(0);
}

int prevButtonVal = LOW;
int prevResetVal = HIGH;
int lastTimeHigh = 0;

int resetStart = 0;
void loop() {
  homeSpan.poll();

  int buttonVal = digitalRead(BUTTON_PIN);
  if (buttonVal != prevButtonVal) {
    lastTimeHigh = millis();
    if (buttonVal == HIGH) {
      cycleBrightness();
      lightbox->handleState();
    }
    prevButtonVal = buttonVal;
  } else {
    if (buttonVal == HIGH && lastTimeHigh != 0 && (millis() - lastTimeHigh) > 1000) {
      lastTimeHigh = 0;
      turnoff();
      lightbox->handleState();
    }
  }

  // int resetVal = digitalRead(RESET_PIN);
  // if (resetVal != prevResetVal) {
  //   if (resetVal == LOW) {      
  //     resetStart = millis();
  //   } else {
  //     if (resetStart != 0 && millis() - resetStart > 3000) {
  //       Serial.println("Reset!");
  //       // lightbox->blink();
  //       // homeSpan.reset();
  //       // ESP.restart();
  //     }
  //     resetStart = 0;
  //   }
  //   prevResetVal = resetVal;
  // }
  
  server.handleClient();
  dns.processNextRequest();
}
