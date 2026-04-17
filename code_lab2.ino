#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

#define BTN_PIN D3
#define LED_BLUE D4
#define LED_YELLOW D6
#define LED_RED D7

const uint8_t leds[] = { LED_BLUE, LED_YELLOW, LED_RED };

SoftwareSerial partnerUART(D1, D2);

uint32_t prevLEDMillis = 0;
const uint32_t ledInterval = 800;
uint8_t currentStep = 0;
bool forwardDirection = true;

volatile bool rawClickDetected = false;
volatile uint32_t lastIntTime = 0;

uint32_t firstClickTime = 0;
bool waitingForSecond = false;

ESP8266WebServer server(80);

ICACHE_RAM_ATTR void handleButton() {
  uint32_t now = millis();
  if (now - lastIntTime > 150) {
    if (digitalRead(BTN_PIN) == LOW) {
      rawClickDetected = true;
    }
    lastIntTime = now;
  }
}

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #e0c3fc 0%, #8ec5fc 100%); margin: 0; height: 100vh; display: flex; align-items: center; justify-content: center; }";
  html += ".card { background: white; padding: 40px; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.15); text-align: center; max-width: 400px; width: 90%; }";
  html += "h1 { color: #2c3e50; margin-bottom: 5px; font-size: 26px; }";
  html += "h3 { color: #7f8c8d; margin-top: 0; font-weight: normal; font-size: 16px; margin-bottom: 30px; }";
  html += ".btn { background: #3498db; color: white; border: none; padding: 15px 30px; font-size: 16px; font-weight: bold; border-radius: 50px; cursor: pointer; transition: all 0.3s ease; margin: 10px; width: 80%; }";
  html += ".led-box { width: 50px; height: 50px; border-radius: 50%; opacity: 0.2; transition: all 0.3s; border: 2px solid #333; margin: 0 5px; }";
  html += "</style></head><body>";

  html += "<div class='card'>";
  html += "<h1>Laboratory Work #2</h1>";
  html += "<h3>Group IP-21 | Student: Anna</h3>";

  html += "<div style='display:flex; justify-content:center; margin-bottom:25px;'>";
  html += "<div id='l0' class='led-box' style='background:#3399ff;'></div>";
  html += "<div id='l1' class='led-box' style='background:#ffd60a;'></div>";
  html += "<div id='l2' class='led-box' style='background:#ff4d4d;'></div>";
  html += "</div>";

  html += "<button class='btn' onclick=\"fetch('/toggle')\">Change Direction</button>";
  html += "<button class='btn' style='background:#9b59b6' onclick=\"fetch('/send')\">Run Partner Leds</button>";
  html += "</div>";

  html += "<script>";
  html += "setInterval(() => { fetch('/state').then(r => r.text()).then(t => {";
  html += "let s = t.split(','); s.forEach((val, i) => {";
  html += "let el = document.getElementById('l' + i);";
  html += "if(val == '1') { el.style.opacity = '1'; el.style.boxShadow = '0 0 15px currentColor'; }";
  html += "else { el.style.opacity = '0.2'; el.style.boxShadow = 'none'; }";
  html += "});});}, 300);</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleState() {
  String s = "";
  for (int i = 0; i < 3; i++) {
    s += (i == currentStep) ? "1" : "0";
    if (i < 2) s += ",";
  }
  server.send(200, "text/plain", s);
}

void handleToggle() {
  forwardDirection = !forwardDirection;
  Serial.println("Web Control: Direction toggled locally");
  server.send(200);
}

void handleSend() {
  Serial.println("Web Control: Sending command 0x0A (A, HEX) to partner...");
  partnerUART.write(0x0A);
  server.send(200);
}

void setup() {
  Serial.begin(115200);
  partnerUART.begin(115200, SWSERIAL_6E2);
  
  for (uint8_t i = 0; i < 3; i++) pinMode(leds[i], OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), handleButton, FALLING);

  WiFi.softAP("Anna_IR21_Lab2");
  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/toggle", handleToggle);
  server.on("/send", handleSend);
  server.begin();
  
  Serial.println("\nSystem Started. Access Point: Anna_IR21_Lab2");
}

void loop() {
  server.handleClient();
  uint32_t currentMillis = millis();

  uint32_t uartStart = millis();
  while (partnerUART.available() > 0) {
    int incoming = partnerUART.read();
    
    if (incoming == 0x0F) {
      forwardDirection = !forwardDirection;
      Serial.println("UART: Received 0x0F from partner! Direction changed.");
    }
    
    if (millis() - uartStart > 500) {
      Serial.println("UART Error: Timeout > 500ms! Breaking loop.");
      break; 
    }
  }

  if (rawClickDetected) {
    rawClickDetected = false;
    if (!waitingForSecond) {
      waitingForSecond = true;
      firstClickTime = currentMillis;
    } else if (currentMillis - firstClickTime <= 700) {
      forwardDirection = !forwardDirection;
      Serial.println("Button: Double click detected, direction changed");
      waitingForSecond = false;
    }
  }
  if (waitingForSecond && (currentMillis - firstClickTime > 700)) {
    waitingForSecond = false;
  }

  if (currentMillis - prevLEDMillis >= ledInterval) {
    prevLEDMillis = currentMillis;
    for (uint8_t i = 0; i < 3; i++) digitalWrite(leds[i], LOW);
    if (forwardDirection) currentStep = (currentStep + 1) % 3;
    else currentStep = (currentStep == 0) ? 2 : currentStep - 1;
    digitalWrite(leds[currentStep], HIGH);
  }
}
