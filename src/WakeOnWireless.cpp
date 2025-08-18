#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <ElegantOTA.h>

//#include <Alexa.h>
#include <Filesys.h>
#include <Mqtt.h>
#include <Wifi.h>


const char* VERSION = "1.0.9";


// AsyncWebServer on port 80
AsyncWebServer server(80);


// GPIO LED
const int ledPin = 2;
String ledState;


// GPIO switch
const int WOL = 5;


// ESP restart
boolean restart = false;


void initAsyncWebServer();
String processor(const String& var);
void initGPIO();
void pushPwr();


void setup() {
  // Initialize GPIO
  initGPIO();


  // Initialize Serial port
  Serial.begin(115200);


  // Initialize File System
  Filesys.initFS();


  // Initialize WiFi (STATION MODE OR ACCESS POINT)
  Wifi.initWiFi(&server);


  // Initialize OTA
  ElegantOTA.begin(&server);
  
  // Initialize Web Server 
  initAsyncWebServer();


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
  Wifi.handleWiFiReconnection();

  if (restart) {
    delay(5000);
    ESP.restart();
  } 
  
  Mqtt.mqqtLoop();
  //Alexa.loopAlexa();
  
  // Add ElegantOTA loop handling
  ElegantOTA.loop();
}


// Initialize GPIO
void initGPIO() {
  // Set GPIO 4 (switch) as an OUTPUT
  pinMode(WOL, OUTPUT);
  digitalWrite(WOL, LOW);

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


void initAsyncWebServer() {
    // Dynamic root handler - serves different pages based on WiFi status
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.getMode() == WIFI_AP || WiFi.status() != WL_CONNECTED) {
            // AP Mode or not connected - serve WiFi setup page
            request->send(LittleFS, "/wifi_setup.html", "text/html");
        } else {
            // Connected to WiFi - serve main dashboard
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        }
    });

    // Control routes (only work when connected)
    server.on("/led_on", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.status() == WL_CONNECTED) {
            pushPwr();
            digitalWrite(ledPin, LOW);
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        } else {
            request->send(400, "text/plain", "WiFi not connected");
        }
    });

    server.on("/led_off", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.status() == WL_CONNECTED) {
            digitalWrite(ledPin, HIGH);
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        } else {
            request->send(400, "text/plain", "WiFi not connected");
        }
    });

    // WiFi setup POST handler (works in AP mode)
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        String newSSID = "";
        String newPass = "";
        
        int params = request->params();
        for(int i = 0; i < params; i++){
            const AsyncWebParameter* p = request->getParam(i);
            
            if(p->isPost()){
                if (p->name() == "ssid") {
                    newSSID = p->value();
                    Filesys.writeFile("/ssid.txt", newSSID.c_str());
                }
                if (p->name() == "pass") {
                    newPass = p->value();
                    Filesys.writeFile("/pass.txt", newPass.c_str());
                }
            }
        }
        
        request->send(LittleFS, "/wifi_setup_success.html", "text/html");
        
        // Restart to connect with new credentials
        delay(2000);
        ESP.restart();
    });

    // Status API endpoint
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"wifi_mode\":\"" + String(WiFi.getMode() == WIFI_AP ? "AP" : "STA") + "\",";
        json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    // Serve static files
    server.serveStatic("/", LittleFS, "/");
    
    // Start server
    server.begin();
    Serial.println("Dynamic web server started");
}


void pushPwr1() {
    Serial.println("Power button triggered");
    
    // Ensure we start from OFF state
    digitalWrite(WOL, HIGH);
    delay(100);
    
    // Press and hold
    digitalWrite(WOL, LOW);
    Serial.println("Power button pressed");
    delay(500);  // Try 500ms duration
    
    // Release
    digitalWrite(WOL, HIGH);
    Serial.println("Power button released");
    
    // Visual confirmation
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
}

void pushPwr() {
    Serial.println("Power button triggered");
    
    // Ensure we start from OFF state
    digitalWrite(WOL, LOW);
    delay(100);
    
    // Press and hold
    digitalWrite(WOL, HIGH);
    Serial.println("Power button pressed");
    delay(500);  // Try 500ms duration
    
    // Release
    digitalWrite(WOL, LOW);
    Serial.println("Power button released");
}
