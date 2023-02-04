#include <ESP8266WiFi.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>

#include <PubSubClient.h>
#include <CertStoreBearSSL.h>
#include <time.h>
#include <TZ.h>

#include "LittleFS.h"
#include <Ticker.h>

// MQTT
WiFiClientSecure espClient;
PubSubClient * client;
Ticker mqttReconnectTimer;
BearSSL::CertStore certStore;

// MQTT HTTP POST request
const char* MQTT_PARAM_INPUT_1 = "server";
const char* MQTT_PARAM_INPUT_2 = "username";
const char* MQTT_PARAM_INPUT_3 = "password";
const char* MQTT_PARAM_INPUT_4 = "hostname";
const char* MQTT_PARAM_INPUT_5 = "topic";
const int MQTT_PORT = 8883;

// MQTT Variables
String mqttServer;
String mqttUsername;
String mqttPassword;
String mqttHostname;
String mqttTopic;

// MQTT Paths
const char* mqttServerPath = "/mqttserver.txt";
const char* mqttUsernamePath = "/mqttusername.txt";
const char* mqttPasswordPath = "/mqttpassword.txt";
const char* mqttHostnamePath = "/mqtthostname.txt";
const char* mqttTopicPath = "/mqtttopic.txt";

// WiFi
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

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

// WiFi Ip 
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// GPIO LED
const int ledPin = 2;
String ledState;

// ESP restart
boolean restart = false;

void setup() {
  // Serial port
  Serial.begin(115200);

  // Initialize File System
  initFS();

  // Initialize GPIO
  initGPIO();

  // Initialize WiFi (STATION MODE OR ACCESS POINT)
  initWiFi();

  // Initialize Web Server 
  initAsyncWebServer(WiFi.isConnected());

  // Initialize MQTTT
  setDateTime();
  readMqttData();
  initMQTT();
}

void loop() {
  if (restart) {
    delay(5000);
    ESP.restart();
  } 
  
  mqqtLoop();
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}

// Initialize GPIO
void initGPIO() {

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}


// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return "";
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
    Serial.println("- write failed");
  }
  file.close();
}

// Initialize WiFi
void initWiFi() {

  // Load WiFi data
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile (LittleFS, gatewayPath);
  
  if(ssid == "" || ip == "") {
    Serial.println("Access Point Mode");
    // NULL sets an open Access Point
    WiFi.softAP("WoW - AP", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  } else {
    
    Serial.println("Station Mode");
    //WiFi.mode(WIFI_STA);
    //localIP.fromString(ip.c_str());
    //localGateway.fromString(gateway.c_str());
  
    //if (!WiFi.config(localIP, localGateway, subnet)) {
    //  Serial.println("STA Failed to configure");
    //  return;
    //}
  
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

  if(var == "IP") {
    return ip;
  }

  if(var == "MQTT_STATE") {
    return client != NULL ? (client->connected() ? "ENABLED" : "DISABLED") : "DISABLED";
  }
  
  return String();
}

void initAsyncWebServer(bool isWifiConnected) {
  if(isWifiConnected) {
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
      restart = true;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    server.begin();
  }
  else {
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


// MQTT
void initMQTT() {
  
  if(mqttServer == "") {
    return;  
  }
  
  // Load MQTT certs
  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  Serial.println("Setting MQTT server: " + mqttServer + ", port: " + String(MQTT_PORT));
  client = new PubSubClient(*bear);
  client->setServer(mqttServer.c_str(), MQTT_PORT);
  client->setCallback(callback);
}

void readMqttData() {
  mqttServer = readFile(LittleFS, mqttServerPath);  
  mqttUsername = readFile(LittleFS, mqttUsernamePath);
  mqttPassword = readFile(LittleFS, mqttPasswordPath);
  mqttHostname = readFile(LittleFS, mqttHostnamePath);
  mqttTopic = readFile (LittleFS, mqttTopicPath);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void mqqtLoop() {
  if(mqttServer == "") {
    return;  
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    if (client->connected()) {
      client->loop();
    } else {
      reconnect();
    }
  }
}

void reconnect() {
  Serial.print("Attempting MQTT connection");
  while (!client->connected()) {
    Serial.print(".");  
    if (client->connect(mqttHostname.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
      Serial.println();
      Serial.println("MQTT connected to: " + mqttServer);
      
      //client->publish("testTopic", "hello world");
      Serial.println("MQTT subscribing topic: " + mqttTopic);
      client->subscribe(mqttTopic.c_str());
    } else {
      Serial.print("MQTT connection failed! rc: ");
      Serial.print(client->state());
      Serial.println("retry again in 5 seconds");
      delay(5000);
    }
  }
}

void setDateTime() {
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP to sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("Current time is: %s %s", tzname[0], asctime(&timeinfo));
}
