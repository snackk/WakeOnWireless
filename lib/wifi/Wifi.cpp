#include <Wifi.h>

WifiClass Wifi;

void WifiClass::initWiFi(AsyncWebServer *server) {
    this->server = server;
    
    // Load WiFi credentials
    ssid = Filesys.readFile(ssidPath);
    pass = Filesys.readFile(passPath);
  
    if(ssid == "" || pass == "") {
        Serial.println("No WiFi credentials found. Starting in Access Point Mode");
        switchToAPMode();
    } else {
        Serial.println("WiFi credentials found. Starting in Station Mode");
        switchToStaMode();
    }
}

void WifiClass::switchToStaMode() {
    WiFi.mode(WIFI_STA);

    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);  // Disable WiFi sleep
    WiFi.setOutputPower(20.5);           // Wifi max power

    // Event handlers
    wifiConnectHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
        this->onWifiConnect(event);
    });
    
    wifiDisconnectHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected& event) {
        this->onWifiDisconnect(event);
    });

    // Wifi Attempt
    connectToWiFi();
}

void WifiClass::onWifiConnect(const WiFiEventStationModeGotIP& event) {
    // Reset tracking vars
    connectionAttempts = 0;
    wifiConnectStartTime = 0;
    shouldReconnect = false;
    lastDisconnectTime = 0;

    Serial.println("\n=== WiFi Connected Successfully ===");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("DNS Server: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("Channel: %d\n", WiFi.channel());
    Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
    Serial.println("===================================\n");
}

void WifiClass::onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
    Serial.printf("\n=== WiFi Disconnected ===\n");
    Serial.printf("Reason Code: %d\n", event.reason);
    
    if (connectionAttempts < MAX_WIFI_ATTEMPTS) {
        shouldReconnect = true;
        lastDisconnectTime = millis();
    } else {
        Serial.println("Max connection attempts reached. Switching to AP mode...");
        switchToAPMode();
    }
    Serial.println("========================\n");
}

String WifiClass::getBestBSSID() {
    int n = WiFi.scanNetworks();
    String bestBSSID = "";
    int bestRSSI = -100;
    int targetChannel = 0;
    
    Serial.printf("Looking for network: %s\n", ssid.c_str());
    
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == ssid) {
            int currentRSSI = WiFi.RSSI(i);
            Serial.printf("Found %s: BSSID=%s, RSSI=%d dBm, Channel=%d\n", 
                         ssid.c_str(), WiFi.BSSIDstr(i).c_str(), 
                         currentRSSI, WiFi.channel(i));
                         
            if (currentRSSI > bestRSSI) {
                bestRSSI = currentRSSI;
                bestBSSID = WiFi.BSSIDstr(i);
                targetChannel = WiFi.channel(i);
            }
        }
    }
    
    if (bestBSSID != "") {
        Serial.printf("Selected best AP: BSSID=%s, RSSI=%d dBm, Channel=%d\n", 
                     bestBSSID.c_str(), bestRSSI, targetChannel);
    } else {
        Serial.println("ERROR: Target network not found in scan results!");
    }
    
    return bestBSSID;
}

void WifiClass::convertBSSIDStringToBytes(const String& bssidStr, uint8_t* bssidBytes) {
    int byteIndex = 0;
    size_t start = 0;
    size_t length = bssidStr.length();

    for (size_t i = 0; i <= length && byteIndex < 6; i++) {
        if (i == length || bssidStr.charAt(i) == ':') {
            if (i > start) {
                String hexPart = bssidStr.substring(start, i);
                bssidBytes[byteIndex] = (uint8_t)strtol(hexPart.c_str(), NULL, 16);
                byteIndex++;
            }
            start = i + 1;
        }
    }
    
    // Verify we got all 6 bytes
    if (byteIndex == 6) {
        Serial.printf("BSSID successfully converted: %s\n", bssidStr.c_str());
    } else {
        Serial.printf("BSSID conversion failed: %s (got %d bytes)\n", bssidStr.c_str(), byteIndex);
        // Fill remaining bytes with zeros
        for (int i = byteIndex; i < 6; i++) {
            bssidBytes[i] = 0;
        }
    }
}

void WifiClass::connectToWiFi() {
    Serial.printf("\n=== Connection Attempt #%d ===\n", connectionAttempts + 1);
    
    // Scan for available networks
    WiFi.scanNetworks();
    
    // Find the best (strongest signal) access point for our network
    String bestBSSID = getBestBSSID();
    
    if (bestBSSID == "") {
        Serial.println("ERROR: Target network not found!");
        connectionAttempts++;
        if (connectionAttempts >= MAX_WIFI_ATTEMPTS) {
            switchToAPMode();
        }
        return;
    }
    
    // Get the channel BEFORE converting BSSID
    int targetChannel = 0;
    int n = WiFi.scanNetworks(); // Fresh scan
    for (int i = 0; i < n; i++) {
        if (WiFi.BSSIDstr(i) == bestBSSID) {
            targetChannel = WiFi.channel(i);
            break;
        }
    }
    
    uint8_t bssidBytes[6];
    convertBSSIDStringToBytes(bestBSSID, bssidBytes);
    
    Serial.printf("Connecting to: %s\n", ssid.c_str());
    Serial.printf("Target BSSID: %s\n", bestBSSID.c_str());
    Serial.printf("Target Channel: %d\n", targetChannel);
    
    WiFi.begin(ssid.c_str(), pass.c_str(), targetChannel, bssidBytes, true);
    
    // Set connection tracking variables
    wifiConnectStartTime = millis();
    connectionAttempts++;
    
    Serial.println("Connection initiated...");
}

void WifiClass::handleWiFiReconnection() {
    if (WiFi.status() != WL_CONNECTED && 
        wifiConnectStartTime > 0 && 
        millis() - wifiConnectStartTime > WIFI_TIMEOUT) {
        
        Serial.println("WiFi connection timeout detected");
        wifiConnectStartTime = 0;  // Clear the timer
        
        if (connectionAttempts < MAX_WIFI_ATTEMPTS) {
            Serial.println("Retrying connection...");
            connectToWiFi();
        } else {
            Serial.println("Max attempts reached, switching to AP mode");
            switchToAPMode();
        }
        return;
    }
    
    if (shouldReconnect && 
        millis() - lastDisconnectTime > RECONNECT_DELAY) {
        shouldReconnect = false;
        
        // Only reconnect if we're not already connected
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Initiating scheduled reconnection...");
            connectToWiFi();
        } else {
            Serial.println("Already connected, canceling scheduled reconnection");
        }
    }
}

void WifiClass::switchToAPMode() {
    // Don't switch to AP mode if we're successfully connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Already connected to WiFi, canceling AP mode switch");
        return;
    }
    
    Serial.println("\n=== Switching to Access Point Mode ===");
    
    // Disconnect from any existing connection
    WiFi.disconnect();
    delay(1000);
    
    // Switch to AP mode
    WiFi.mode(WIFI_AP);
    
    // Create AP with default settings
    bool apStarted = WiFi.softAP("WoW - AP", NULL);
    
    if (apStarted) {
        IPAddress IP = WiFi.softAPIP();
        Serial.printf("Access Point started successfully\n");
        Serial.printf("AP SSID: WoW - AP\n");
        Serial.printf("AP IP address: %s\n", IP.toString().c_str());
        Serial.printf("Connect to this network to configure WiFi credentials\n");
    } else {
        Serial.println("Failed to start Access Point!");
    }
    
    Serial.println("=====================================\n");
    
    // Reset connection tracking
    connectionAttempts = 0;
    wifiConnectStartTime = 0;
    shouldReconnect = false;
}