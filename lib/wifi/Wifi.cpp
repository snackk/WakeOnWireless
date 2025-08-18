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
    } else {
        
        Serial.println("Station Mode");
        WiFi.mode(WIFI_STA);

        // Set up reliability settings
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
    
        // WiFi Handler Setup
        wifiConnectHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
            this->onWifiConnect(event);
        });
        wifiDisconnectHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected& event) {
            this->onWifiDisconnect(event);
        });

        connectToWiFi();
    }
}

void WifiClass::onWifiConnect(const WiFiEventStationModeGotIP& event) {
    Serial.printf("\nConnected to WiFi with IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    
    connectionAttempts = 0;  // Reset attempt counter
    wifiConnectStartTime = 0; // Reset connection timer
}

void WifiClass::connectToWiFi() {
    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    // Set connection timeout
    wifiConnectStartTime = millis();
    connectionAttempts++;
    
    Serial.print("Connection attempt #");
    Serial.println(connectionAttempts);
}

void WifiClass::onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
    Serial.printf("Disconnected from WiFi. Reason: %d\n", event.reason);
    
    // Don't immediately reconnect if we're switching to AP mode
    if (connectionAttempts < MAX_WIFI_ATTEMPTS) {
        Serial.println("Attempting to reconnect in 5 seconds...");
        // Use a timer or delay in main loop to reconnect
        shouldReconnect = true;
        lastDisconnectTime = millis();
    } else {
        Serial.println("Max connection attempts reached. Switching to AP mode...");
        switchToAPMode();
    }
}

void WifiClass::handleWiFiReconnection() {
    // Check for connection timeout during initial connection
    if (WiFi.status() != WL_CONNECTED && 
        wifiConnectStartTime > 0 && 
        millis() - wifiConnectStartTime > WIFI_TIMEOUT) {
        
        Serial.println("WiFi connection timeout");
        wifiConnectStartTime = 0;
        
        if (connectionAttempts < MAX_WIFI_ATTEMPTS) {
            connectToWiFi();
        } else {
            Serial.println("Max attempts reached, switching to AP mode");
            switchToAPMode();
        }
    }
    
    // Handle reconnection after disconnect
    if (shouldReconnect && millis() - lastDisconnectTime > RECONNECT_DELAY) {
        shouldReconnect = false;
        connectToWiFi();
    }
}

void WifiClass::switchToAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WoW - AP", NULL);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Switched to AP mode. IP: ");
    Serial.println(IP);
}