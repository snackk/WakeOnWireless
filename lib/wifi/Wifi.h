#ifndef Wifi_H_
#define Wifi_H_

#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Filesys.h>

class WifiClass {
    
    public:
        void 
            onWifiDisconnect(const WiFiEventStationModeDisconnected& event),
            onWifiConnect(const WiFiEventStationModeGotIP& event),
            initWiFi(AsyncWebServer *server),
            initWebServer();
    
    private:
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