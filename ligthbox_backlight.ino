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
  styles += "body { font-family: sans-serif; font-size: 18px; line-height: 30px; display: flex; align-items: center; justify-content: center; flex-direction: column; text-align: center; }";
  styles += ".app { position: absolute; left: 0; right: 0; top: 40%; transform: translateY(-50%); display: flex; flex-direction: column; align-items: center; padding: 0 15px; }";
  styles += "h2 { font-size: 45px; line-height: 1; margin-bottom: 30px; }";
  styles += "select, input, button { appearance: button; border: 1px solid black; padding: 10px 20px; font-size: 24px; background: transparent; border-radius: 8px; width: 280px; }";
  styles += "select, input { border-radius: 8px 8px 0 0; }";
  styles += "select + input, input + input { border-radius: 0 0 8px 8px; border-top: 0 }";
  styles += "button { background: #93f179; color: black; margin-top: 15px; }";
  styles += "form { display: flex; flex-direction: column; }";
  styles += "a { color: black; }";
  styles += ".sending button span { display: none; } .sending button::before { content: 'Подключение...' }";
  styles += ".err { max-width: 280px; display: none; }";
  styles += ".sending .err { display: none; }";
  styles += ".fail .err { display: block; }";

  static String html_meta = "<meta name='viewport' content='width=device-width, initial-scale=1.0'><meta charset='utf-8'>";

  // WebServer
  server.on("/", [](){
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

    String html = "<html><head><title>Настройка Wi-Fi</title>";
    html += html_meta;
    html += "<style>" + styles + "</style></head><body><div class='app'>";
    html += "<h2>Настройка<br>Wi-Fi</h2><form method='POST'>";
    
    if (n > 0) {
      String options;
      for (int i = 0; i < n; ++i) {
          String ssid = WiFi.SSID(i);
          options += "<option value='" + ssid + "'>" + ssid + "</option>";
      }
      html += "<select name='ssid' required>" + options + "</select>";
    } else {
      html += "<input name='ssid' required placeholder='Сеть' />";
    }

    html += "<input name='pass' placeholder='Пароль'>";
    html += "<p class='err'>Не удалось подключиться. Попробуйте еще и проверьте пароль</p>";
    html += "<button><span>Подключиться</span></button></form></div><script>";
    html += "let f = document.querySelector('form'); let sending = false; let err = document.querySelector('.err');";
    html += "f.addEventListener('submit', (e) => {";
    html += "e.preventDefault(); if (sending) {return;} sending = true; f.classList.toggle('sending');";
    html += "fetch('/connect', { method: 'POST', body: new FormData(f) }).then((r) => {";
    html += "if (r.status === 400) { alert(''); f.classList.toggle('fail'); f.classList.toggle('sending'); sending = false; return; }";
    html += "window.location = '/success'; }) })";
    html += "</script></body></html>";

    server.send(200, "text/html; charset=utf-8", html);
  });

  server.on("/connect", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("Trying to connect to Wi-Fi");

    int timeout = 0;
    const int maxTimeout = 10000;
    while((WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED) && timeout < maxTimeout){
      delay(100);
      Serial.print(".");
      timeout += 100;
    }
    Serial.println();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Wi-Fi connected successfully!");
      homeSpan.setWifiCredentials(ssid.c_str(), pass.c_str());

      server.send(200, "text/html; charset=utf-8", "ok");
    } else {
      Serial.println("Failed to connect!");
      server.send(400, "text/html; charset=utf-8", "");
    }
  });

  server.on("/success", [](){
    String html = "<html><head><title>Wi-Fi настроен</title><style>" + styles + "</style>" + html_meta + "</head><body><div class='app'>";
    html += "<h2>Wi-Fi настроен</h2><p>Лайтбокс перезагружается. Страницу можно закрыть и&nbsp;перейти к&nbsp;добавлению аксессуара в&nbsp;Home.app</p>";
    html += "</div></body></html>";
    server.send(200, "text/html; charset=utf-8", html);
    delay(1000);
    ESP.restart();
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
  lightbox->handleState();
}

int BRIGTHNESS_STEPS = 5;
int STEP_SIZE = 100 / BRIGTHNESS_STEPS;
void cycleBrightness() {
  int currentStep = lightbox->brightness->getVal() / STEP_SIZE;
  int nextStep = currentStep + 1;
  if (nextStep > BRIGTHNESS_STEPS) {
    nextStep = 1;
  }
  int brightness = nextStep * STEP_SIZE;
  lightbox->brightness->setVal(brightness);
  lightbox->power->setVal(brightness ? 1 : 0);
}

void turnoff() {
  lightbox->power->setVal(0);
}

void turnon() {
  int brightness = lightbox->brightness->getVal();
  lightbox->brightness->setVal(brightness == 0 ? 20 : brightness);
  lightbox->power->setVal(1);
}

int prevButtonVal = LOW;
int prevResetVal = HIGH;
int lastTimeHigh = 0;

int resetStart = 0;
void loop() {
  homeSpan.poll();

  int buttonVal = digitalRead(BUTTON_PIN);
  if (buttonVal != prevButtonVal) {
    if (buttonVal == HIGH) {
      lastTimeHigh = millis();
    }
    if (buttonVal == LOW && lastTimeHigh != 0 && (millis() - lastTimeHigh) < 300) {
      lastTimeHigh = 0;
      cycleBrightness();
      lightbox->handleState();
    }
    prevButtonVal = buttonVal;
  } else {
    if (buttonVal == HIGH && lastTimeHigh != 0 && (millis() - lastTimeHigh) > 1000) {
      lastTimeHigh = 0;
      if (lightbox->isDisabled()) {
        turnon();
      } else {
        turnoff();
      }
      lightbox->handleState();
    }
  }
  
  server.handleClient();
  dns.processNextRequest();
}
