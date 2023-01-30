#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


// MQTT

// MQTT HTTP POST request
const char* MQTT_PARAM_INPUT_1 = "server";
const char* MQTT_PARAM_INPUT_2 = "username";
const char* MQTT_PARAM_INPUT_3 = "password";
const char* MQTT_PARAM_INPUT_4 = "hostname";
const char* MQTT_PARAM_INPUT_5 = "topic";
const int MQTT_PORT = 18345;

// MQTT Variables
String mqttServer;
String mqttUsername;
String mqttPassword;
String mqttHostname;
String mqttTopic;

// MQTT Paths
const char* mqttServerPath = "/mqtt_server.txt";
const char* mqttUsernamePath = "/mqtt_username.txt";
const char* mqttPasswordPath = "/mqtt_password.txt";
const char* mqttHostnamePath = "/mqtt_hostname.txt";
const char* mqttTopicPath = "/mqtt_topic.txt";


// WiFi Setup HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

// WiFi Setup Variables
String ssid;
String pass;
String ip;
String gateway;

// WiFi Setup Paths
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;

boolean restart = false;

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}


// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
  file.close();
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.println("Connecting to WiFi...");
  delay(20000);
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");
    return false;
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with LED state value
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

  if(var == "IP") {
    return ip;
  }

  if(var == "MQTT_STATE") {
    return mqttServer != "" ? "ENABLED" : "DISABLED";
  }
  
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  initFS();

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Load WiFi Setup data
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile (LittleFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  // Load MQTT data  
  mqttServer = readFile(LittleFS, mqttServerPath);
  mqttUsername = readFile(LittleFS, mqttUsernamePath);
  mqttPassword = readFile(LittleFS, mqttPasswordPath);
  mqttHostname = readFile (LittleFS, mqttHostnamePath);
  mqttTopic = readFile (LittleFS, mqttTopicPath);
  Serial.println(mqttServer);
  Serial.println(mqttUsername);
  Serial.println(mqttPassword);
  Serial.println(mqttHostname);
  Serial.println(mqttTopic);

  if(initWiFi()) {
    // Route for root / web page
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

    // Route to setup MQTT
    server.on("/mqtt_setup", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);
      request->send(LittleFS, "/mqtt_setup.html", "text/html");
    });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST server value
          if (p->name() == MQTT_PARAM_INPUT_1) {
            mqttServer = p->value().c_str();
            Serial.print("mqttServer set to: ");
            Serial.println(mqttServer);
            // Write file to save value
            writeFile(LittleFS, mqttServerPath, mqttServer.c_str());
          }
          // HTTP POST username value
          if (p->name() == MQTT_PARAM_INPUT_2) {
            mqttUsername = p->value().c_str();
            Serial.print("mqttUsername set to: ");
            Serial.println(mqttUsername);
            // Write file to save value
            writeFile(LittleFS, mqttUsernamePath, mqttUsername.c_str());
          }
          // HTTP POST password value
          if (p->name() == MQTT_PARAM_INPUT_3) {
            mqttPassword = p->value().c_str();
            Serial.print("mqttPassword set to: ");
            Serial.println(mqttPassword);
            // Write file to save value
            writeFile(LittleFS, mqttPasswordPath, mqttPassword.c_str());
          }
          // HTTP POST hostname value
          if (p->name() == MQTT_PARAM_INPUT_4) {
            mqttHostname = p->value().c_str();
            Serial.print("mqttHostname set to: ");
            Serial.println(mqttHostname);
            // Write file to save value
            writeFile(LittleFS, mqttHostnamePath, mqttHostname.c_str());
          }
          // HTTP POST topic value
          if (p->name() == MQTT_PARAM_INPUT_5) {
            mqttTopic = p->value().c_str();
            Serial.print("mqttTopic set to: ");
            Serial.println(mqttTopic);
            // Write file to save value
            writeFile(LittleFS, mqttTopicPath, mqttTopic.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("WoW - AP", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifi_setup.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      restart = true;
      request->send(LittleFS, "/wifi_setup_success.html", "text/html", false, processor);
    });
    server.begin();
  }
}

void loop() {
  if (restart){
    delay(5000);
    ESP.restart();
  }
}
