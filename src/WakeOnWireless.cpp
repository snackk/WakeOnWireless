#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <ElegantOTA.h>

#include <Filesys.h>
#include <Wifi.h>
#include <Alexa.h>

const char* VERSION = "1.0.11";

// AsyncWebServer on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

// GPIO LED
const int ledPin = 2;
String ledState;

// GPIO switch
const int WOL = 5;

// ESP restart
boolean restart = false;

void initAsyncWebServer();
void initGPIO();
void pushPwrOn();
void pushPwrOff();
void handleAlexaCommand(bool state);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void webLog(String message); 

void setup() {

    // Initialize GPIO
    initGPIO();

    // Initialize Serial port
    Serial.begin(115200);

    // Initialize File System
    Filesys.initFS();

    // Initialize WiFi
    Wifi.initWiFi(&server, handleAlexaCommand);

    // Initialize OTA
    ElegantOTA.begin(&server);
    
    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    
    // Initialize Web Server 
    initAsyncWebServer();
}

void loop() {
    Wifi.handleWiFiReconnection();
    ElegantOTA.loop();
    Alexa.loopAlexa();
    ws.cleanupClients();

    if (restart) {
        delay(5000);
        ESP.restart();
    } 
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        client->text("Connected to PC Controller");
    }
}

// Send log message to WebSocket
void webLog(String message) {
    Serial.println(message);
    if (ws.count() > 0) {
        ws.textAll(message);
    }
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

void initAsyncWebServer() {

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (WiFi.getMode() == WIFI_AP || WiFi.status() != WL_CONNECTED) {
            request->send(LittleFS, "/wifi_setup.html", "text/html");
        } else {
            request->send(LittleFS, "/index.html", "text/html");
        }
    });

    server.on("/api/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"version\":\"" + String(VERSION) + "\"}";
        request->send(200, "application/json", json);
    });

    // Console page
    server.on("/console", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/console.html", "text/html");
    });

    server.on("^\\/api\\/state\\/(ON|OFF)$", HTTP_PUT, [](AsyncWebServerRequest *request) {
        String state = request->pathArg(0);
        bool powerOn = (state == "ON");
        
        webLog("API command : Power " + state);
    
        powerOn ? pushPwrOn() : pushPwrOff();
        request->send(200, "application/json", "{\"result\":\"ok\"}");
    });

    // TODO refactor - Raspberry handler for Alexa
    server.on("/led_on", HTTP_GET, [](AsyncWebServerRequest *request) {
        pushPwrOn();
        request->send(302, "text/plain", "OK");
    });

    // WiFi setup POST handler
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
        delay(2000);
        ESP.restart();
    });

    // Serve static files (CSS, JS, Favicon, etc)
    server.serveStatic("/", LittleFS, "/");
    
    server.begin();
    webLog("Web server updated for PC Control");
}

void handleAlexaCommand(bool state) {
    if (state) {
        pushPwrOn();
        digitalWrite(ledPin, LOW);
    } else {
        digitalWrite(ledPin, HIGH);
    }
}

void pushPwrOn() {
    webLog("Power button triggered");
    
    // Ensure we start from OFF state
    digitalWrite(WOL, LOW);
    delay(100);
    
    // Press and hold
    digitalWrite(WOL, HIGH);
    webLog("Power button pressed");
    delay(500);  // 500ms duration
    
    // Release button
    digitalWrite(WOL, LOW);
    webLog("Power button released");
}

void pushPwrOff() {
    webLog("Force Shutdown triggered (5s hold)");
    
    digitalWrite(WOL, LOW);
    delay(100);
    
    digitalWrite(WOL, HIGH);
    webLog("Power button being held...");
    
    // Segura o botão por 5000ms (5 segundos)
    delay(5000); 
    
    digitalWrite(WOL, LOW);
    webLog("Power button released after 5s");
}
