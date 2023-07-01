#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <AsyncElegantOTA.h>

//#include <Alexa.h>
#include <Filesys.h>
#include <Mqtt.h>
#include <Wifi.h>

const char* VERSION = "1.0.1";

// AsyncWebServer on port 80
AsyncWebServer server(80);

// GPIO LED
const int ledPin = 2;
String ledState;

// ESP restart
boolean restart = false;

void initAsyncWebServer(bool isWifiConnected);
String processor(const String& var);
void initGPIO();

void setup() {
  // Initialize Serial port
  Serial.begin(115200);

  // Initialize File System
  Filesys.initFS();

  // Initialize GPIO
  initGPIO();

  // Initialize WiFi (STATION MODE OR ACCESS POINT)
  Wifi.initWiFi(&server);

  // Initialize OTA
  AsyncElegantOTA.begin(&server);
  
  // Initialize Web Server 
  initAsyncWebServer(WiFi.isConnected());

  if(WiFi.isConnected()) {
    
    // Initialize Alexa
    /*
    Alexa.initAlexa([](bool state) {
      digitalWrite(ledPin, state ? HIGH : LOW);
    });
    */

    // Initialize MQTTT
    Mqtt.initMQTT(&server);
  }
}

void loop() {
  if (restart) {
    delay(5000);
    ESP.restart();
  } 
  
  Mqtt.mqqtLoop();
  //Alexa.loopAlexa();
}

// Initialize GPIO
void initGPIO() {

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

// Templating with HTML pages
// Replaces %VAR% from HTML
String processor(const String& var) {
  if(var == "LED_STATE") {
    if(!digitalRead(ledPin)) {
      ledState = "ON";
    }
    else {
      ledState = "OFF";
    }
    return ledState;
  }

  //TODO - This isn't doing anything atm.
  if(var == "IP") {
    return "aaaa";
  }

  if(var == "MQTT_STATE") {
    return Mqtt.isMqttEnabled();
  }

  if(var == "VERSION") {
    return VERSION;
  }
  
  return String();
}

void initAsyncWebServer(bool isWifiConnected) {
  if(isWifiConnected) {
    // Dashboard
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to HIGH
    server.on("/led_on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, LOW);
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/led_off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    server.begin();
  }
  
  server.serveStatic("/", LittleFS, "/");
  server.begin();
}
