#include <Arduino.h>

#include <ESP8266WiFi.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>

#include <AsyncElegantOTA.h>

#include <Ticker.h>

#include <Alexa.h>
#include <Filesys.h>
#include <Mqtt.h>


const char* VERSION = "1.0.1";

// AsyncWebServer on port 80
AsyncWebServer server(80);

// WiFi
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// WiFi Setup HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";

// WiFi Setup Variables
String ssid;
String pass;

// WiFi Setup Paths
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

// GPIO LED
const int ledPin = 2;
String ledState;

// ESP restart
boolean restart = false;

void initAsyncWebServer(bool isWifiConnected);
String processor(const String& var);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void initWiFi();
void initGPIO();

void setup() {
  // Serial port
  Serial.begin(115200);

  // Initialize File System
  Filesys.initFS();

  // Initialize GPIO
  initGPIO();

  // Initialize WiFi (STATION MODE OR ACCESS POINT)
  initWiFi();

  // Initialize OTA
  AsyncElegantOTA.begin(&server);
  
  // Initialize Web Server 
  initAsyncWebServer(WiFi.isConnected());

  if(WiFi.isConnected()) {
    
    // Initialize Alexa
    Alexa.initAlexa([](bool state) {
      digitalWrite(ledPin, state ? HIGH : LOW);
    });

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
  Alexa.loopAlexa();
}

// Initialize GPIO
void initGPIO() {

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

// Initialize WiFi
void initWiFi() {

  // Load WiFi data
  ssid = Filesys.readFile(ssidPath);
  pass = Filesys.readFile(passPath);
  
  if(ssid == "" || pass == "") {
    Serial.println("Access Point Mode");
    WiFi.softAP("WoW - AP", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  } else {
    
    Serial.println("Station Mode");
    WiFi.mode(WIFI_STA);
  
    // WiFi Handler Setup
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.printf("Connecting to WiFi: %s", ssid.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
  }
}

// WiFi Handlers
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.printf("\nConnected to WiFi with IP: %s\n", WiFi.localIP().toString().c_str());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from WiFi");
  wifiReconnectTimer.once(2, initWiFi);
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

    server.serveStatic("/", LittleFS, "/");

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
  } else {
    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifi_setup.html", "text/html");
    });

    server.serveStatic("/", LittleFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i = 0; i < params; i++){
        AsyncWebParameter* p = request->getParam(i);
        
        if(p->isPost()){
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Filesys.writeFile(ssidPath, ssid.c_str());
          }

          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Filesys.writeFile(passPath, pass.c_str());
          }
        }
      }
      restart = true;
      request->send(LittleFS, "/wifi_setup_success.html", "text/html", false, processor);
    });
    server.begin();
  }
}
