#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define BTN_PIN D3  
#define LED1 D4     
#define LED2 D6
#define LED3 D7

const uint8_t leds[] = {LED1, LED2, LED3};

// int32_t      
// uint32_t

uint32_t prevLEDMillis = 0;
const uint32_t ledInterval = 400; 
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
  html += ".direction { font-size: 16px; color: #34495e; margin-bottom: 30px; background: #f8f9fa; padding: 20px; border-radius: 12px; border-left: 5px solid #3498db; line-height: 1.6; }";
  html += ".btn { background: #3498db; color: white; border: none; padding: 15px 30px; font-size: 16px; font-weight: bold; border-radius: 50px; cursor: pointer; transition: all 0.3s ease; box-shadow: 0 4px 15px rgba(52, 152, 219, 0.4); text-transform: uppercase; letter-spacing: 1px; }";
  html += ".btn:hover { background: #2980b9; transform: translateY(-3px); box-shadow: 0 6px 20px rgba(52, 152, 219, 0.6); }";
  html += ".btn:active { transform: translateY(1px); box-shadow: 0 2px 10px rgba(52, 152, 219, 0.4); }";
  html += "</style></head><body>";
  
  html += "<div class='card'>";
  html += "<h1>Laboratory Work #1</h1>";
  html += "<h3>Group IP-21 | Student: Anna</h3>";
  
  html += "<div class='direction'><b>Algorithm modes:</b><br><br>";
  html += "&#128308; Red &rarr; &#128993; Yellow &rarr; &#128309; Blue<br>";
  html += "<i>(or vice versa)</i><br>";
  html += "&#128309; Blue &rarr; &#128993; Yellow &rarr; &#128308; Red</div>";
  
  html += "<button class='btn' onclick=\"location.href='/toggle'\">Change Direction</button>";
  html += "</div>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggle() {
  forwardDirection = !forwardDirection;
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 3; i++) pinMode(leds[i], OUTPUT);
  
  pinMode(BTN_PIN, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), handleButton, FALLING);
  
  WiFi.softAP("Anna_IR21_Lab1");
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
  
  Serial.println("System ready. IP: 192.168.4.1");
}

void loop() {
  server.handleClient(); 
  uint32_t currentMillis = millis();

  if (rawClickDetected) {
    rawClickDetected = false;
    
    if (!waitingForSecond) {
      waitingForSecond = true;
      firstClickTime = currentMillis;
      Serial.println("Click 1 detected...");
    } 
    else {
      if (currentMillis - firstClickTime <= 700) {
        forwardDirection = !forwardDirection; 
        Serial.println(">>> DOUBLE CLICK! Direction changed.");
        waitingForSecond = false; 
      }
    }
  }

  if (waitingForSecond && (currentMillis - firstClickTime > 700)) {
    waitingForSecond = false;
    Serial.println("Timeout. Single click (ignored).");
  }

  if (currentMillis - prevLEDMillis >= ledInterval) {
    prevLEDMillis = currentMillis;
    
    for(uint8_t i=0; i<3; i++) digitalWrite(leds[i], LOW);
    
    if (forwardDirection) {
      currentStep = (currentStep + 1) % 3;
    } else {
      currentStep = (currentStep == 0) ? 2 : currentStep - 1;
    }
    
    digitalWrite(leds[currentStep], HIGH);
  }
}

