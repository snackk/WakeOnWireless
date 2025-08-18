#ifndef Wifi_H_
#define Wifi_H_

#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Filesys.h>

class WifiClass {
    
    public:
        void 
            connectToWiFi(),
            handleWiFiReconnection(),
            switchToAPMode(),
            onWifiDisconnect(const WiFiEventStationModeDisconnected& event),
            onWifiConnect(const WiFiEventStationModeGotIP& event),
            initWiFi(AsyncWebServer *server);
    
    private:
        static const int MAX_WIFI_ATTEMPTS = 3;
        static const unsigned long WIFI_TIMEOUT = 30000;  // 30 seconds
        static const unsigned long RECONNECT_DELAY = 5000;  // 5 seconds
        
        int connectionAttempts = 0;
        unsigned long wifiConnectStartTime = 0;
        unsigned long lastDisconnectTime = 0;
        bool shouldReconnect = false;

        AsyncWebServer *server;
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

};

extern WifiClass Wifi;

#endif