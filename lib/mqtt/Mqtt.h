#ifndef Mqtt_H_
#define Mqtt_H_

#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <CertStoreBearSSL.h>
#include <time.h>
#include <TZ.h>
#include <Ticker.h>
#include <Filesys.h>

class MqttClass {
    
    public:
        void
            initMQTT(AsyncWebServer *server),
            mqqtLoop();

        String isMqttEnabled();
    
    private:
        AsyncWebServer *server;

        //MQTT PORT
        const int MQTT_PORT = 8883;

        // MQTT HTTP POST request
        const char* MQTT_PARAM_INPUT_1 = "server";
        const char* MQTT_PARAM_INPUT_2 = "username";
        const char* MQTT_PARAM_INPUT_3 = "password";
        const char* MQTT_PARAM_INPUT_4 = "hostname";
        const char* MQTT_PARAM_INPUT_5 = "topic";

        // MQTT Paths
        const char* mqttServerPath = "/mqttserver.txt";
        const char* mqttUsernamePath = "/mqttusername.txt";
        const char* mqttPasswordPath = "/mqttpassword.txt";
        const char* mqttHostnamePath = "/mqtthostname.txt";
        const char* mqttTopicPath = "/mqtttopic.txt";

        // MQTT Variables
        String mqttServer;
        String mqttUsername;
        String mqttPassword;
        String mqttHostname;
        String mqttTopic;
            
        WiFiClientSecure espClient;
        PubSubClient* client;
        Ticker mqttReconnectTimer;
        BearSSL::CertStore certStore;

        void
            initWebServer(),
            readMqttData(),
            initDateTime(),
            reconnect(),
            callback(char* topic, byte* payload, unsigned int length);
};

extern MqttClass Mqtt;

#endif