#include <Wifi.h>

WifiClass Wifi;

void WifiClass::initWiFi(AsyncWebServer *server) {
    this->server = server;
    // Load WiFi data
    ssid = Filesys.readFile(ssidPath);
    pass = Filesys.readFile(passPath);
  
    if(ssid == "" || pass == "") {
        Serial.println("Access Point Mode");
        WiFi.softAP("WoW - AP", NULL);

        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);
        
        // This should be done elsewhere
        this->initWebServer();
    } else {
        
        Serial.println("Station Mode");
        WiFi.mode(WIFI_STA);
    
        // WiFi Handler Setup
        wifiConnectHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
            this->onWifiConnect(event);
        });
        wifiDisconnectHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected& event) {
            this->onWifiDisconnect(event);
        });
        
        WiFi.begin(ssid.c_str(), pass.c_str());
        Serial.printf("Connecting to WiFi: %s", ssid.c_str());
        while (WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          delay(500);
        }
        Serial.println();
    }
}

void WifiClass::onWifiConnect(const WiFiEventStationModeGotIP& event) {
    Serial.printf("\nConnected to WiFi with IP: %s\n", WiFi.localIP().toString().c_str());
}

void WifiClass::onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from WiFi");
  //wifiReconnectTimer.once(2, initWiFi);
}

void WifiClass::initWebServer() {

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifi_setup.html", "text/html");
    });

    server->on("/", HTTP_POST, [this](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i = 0; i < params; i++){
         const AsyncWebParameter* p = request->getParam(i);
        
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
      //restart = true;
      request->send(LittleFS, "/wifi_setup_success.html", "text/html", false);//, processor);
    });
}