#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <ElegantOTA.h>

#include <Filesys.h>
#include <Wifi.h>
#include <Alexa.h>
#include <ESP8266mDNS.h>

const char* VERSION = "1.0.13";
const char* devNamePath = "/dev_name.txt";

// AsyncWebServer on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

// GPIO LED
const int ledPin = 2;
String ledState;

// GPIO switch
const int WOL = 5;

// ESP responsive delay / restart
enum PowerState { IDLE, START_PRESS, HOLDING, RELEASING };
PowerState currentPowerState = IDLE;

unsigned long powerTimer = 0;
unsigned long holdDuration = 0;
unsigned long restartTimer = 0;
boolean restartPending = false;

void initAsyncWebServer();
void initGPIO();
void pushPwrOn();
void pushPwrOff();
void handleAlexaCommand(bool state);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void webLog(String message);
void handlePowerStateMachine(); 
void initmDNS();

void setup() {

    // Initialize GPIO
    initGPIO();

    // Initialize Serial port
    Serial.begin(115200);

    // Initialize File System
    Filesys.initFS();
     
    // Initialize Web Server 
    initAsyncWebServer();
    
    // Initialize WiFi
    Wifi.initWiFi(&server, handleAlexaCommand);

    // Initialize OTA
    ElegantOTA.begin(&server);
    
    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Initialize mDNS
    initmDNS();

    server.begin();
    webLog("Web server started for PC Control");
}

void loop() {
    Wifi.handleWiFiReconnection();
    ElegantOTA.loop();
    MDNS.update();
    Alexa.loopAlexa();
    ws.cleanupClients();

    handlePowerStateMachine();
    
    if (restartPending && (millis() - restartTimer >= 5000)) {
        ESP.restart();
    } 
}

void initmDNS() {
    String devName = Filesys.readFirstLine(devNamePath);
    if (devName.length() == 0) {
        devName = "pc-switch";
        Filesys.writeFile(devNamePath, devName.c_str());
    }
    
    if (MDNS.begin(devName.c_str())) {
        MDNS.addService("http", "tcp", 80); 
        webLog("mDNS started: http://" + devName + ".local/");
    } else {
        webLog("Error starting mDNS");
    }
}

void triggerPowerAction(unsigned long duration) {
    if (currentPowerState == IDLE) {
        holdDuration = duration;
        currentPowerState = START_PRESS;
        powerTimer = millis();
    }
}

void handlePowerStateMachine() {
    switch (currentPowerState) {
        case START_PRESS:
            if (millis() - powerTimer >= 100) {
                digitalWrite(WOL, HIGH);
                webLog("Power button pressed...");
                powerTimer = millis();
                currentPowerState = HOLDING;
            }
            break;

        case HOLDING:
            if (millis() - powerTimer >= holdDuration) {
                digitalWrite(WOL, LOW);
                webLog("Power button released");
                currentPowerState = IDLE;
            }
            break;
            
        default:
            break;
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
        String newSSID = "", newPass = "", devName = "";
        
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
                if (p->name() == "dev_name") {
                    devName = p->value();
                    Filesys.writeFile("/dev_name.txt", devName.c_str());
                }                  
            }
        }
        
        request->send(LittleFS, "/wifi_setup_success.html", "text/html");

        restartTimer = millis();
        restartPending = true;
    });

    // Serve static files (CSS, JS, Favicon, etc)
    server.serveStatic("/", LittleFS, "/");
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
    webLog("Action: Power ON/OFF (Short Press)");
    triggerPowerAction(500); // 500ms
}

void pushPwrOff() {
    webLog("Action: Force Shutdown (5s hold)");
    triggerPowerAction(5000); // 5000ms
}
