#include <Mqtt.h>

MqttClass Mqtt;

void MqttClass::initMQTT(AsyncWebServer *server) {
    if(mqttServer == "") {
        return;  
    }

    this->server = server;

    // This should be done elsewhere
    this->initWebServer();  
    
    initDateTime();
    readMqttData();
    
    int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.printf("[MQTT] - Number of CA certs read #%d\n", numCerts);
    if (numCerts == 0) {
        Serial.printf("[MQTT] - No CA certs found.\n");
        return;
    }

    BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
    bear->setCertStore(&certStore);

    Serial.printf("[MQTT] - Setting MQTT server #%s on port #%i\n", mqttServer.c_str(), MQTT_PORT);
    client = new PubSubClient(*bear);
    client->setServer(mqttServer.c_str(), MQTT_PORT);
    client->setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
    Serial.println("[MQTT] - Setup done.");
}

String MqttClass::isMqttEnabled() {
    return client != NULL ? 
        (client->connected() ? 
            "ENABLED" 
            : "DISABLED") 
        : "DISABLED";
}

void MqttClass::mqqtLoop() {
    // Don't do anything if MQTT not configured
    if(mqttServer == "" || !client) {
        return;
    }
    
    // Don't do anything if WiFi not connected
    if(WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    // Don't do anything if NTP not synced (for secure connections)
    if (!isNTPSynced()) {
        return;
    }
    
    if (client->connected()) {
        client->loop();
    } else {
        reconnect();
    }
}


void MqttClass::callback(char* topic, byte* payload, unsigned int length) {
    String accPayload;
    for (int i = 0; (unsigned)i < length; i++) {
        accPayload += (char)payload[i];
    }
    Serial.printf("[MQTT] - Message arrived on topic #%s with payload #%s\n", topic, accPayload.c_str());
}

void MqttClass::reconnect() {
    static unsigned long lastReconnectAttempt = 0;
    
    if (millis() - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = millis();
        
        Serial.print("[MQTT] - Attempting connection...");
        if (client->connect(mqttHostname.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
            Serial.printf(" Connected to server %s\n", mqttHostname.c_str());
            client->subscribe(mqttTopic.c_str());
        } else {
            Serial.printf(" Failed (state: %d)\n", client->state());
        }
    }
}

void MqttClass::readMqttData() {
    //Probably replace this by a class
    mqttServer = Filesys.readFirstLine(mqttServerPath);  
    mqttUsername = Filesys.readFirstLine(mqttUsernamePath);
    mqttPassword = Filesys.readFirstLine(mqttPasswordPath);
    mqttHostname = Filesys.readFirstLine(mqttHostnamePath);
    mqttTopic = Filesys.readFirstLine(mqttTopicPath);
}

void MqttClass::initDateTime() {
    configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");
    Serial.println("[MQTT] - NTP sync initiated");
    ntpSyncStarted = true;
    ntpSyncTime = millis();
}

bool MqttClass::isNTPSynced() {
    if (!ntpSyncStarted) return false;
    
    time_t now = time(nullptr);
    if (now > 8 * 3600 * 2) {
        if (!ntpSynced) {
            Serial.printf("[MQTT] - NTP synced at %s\n", asctime(localtime(&now)));
            ntpSynced = true;
        }
        return true;
    }
    return false;
}

void MqttClass::initWebServer() {
    server->on("/mqtt_setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/mqtt_setup.html", "text/html");
    });

    server->on("/", HTTP_POST, [this](AsyncWebServerRequest *request) {
        int params = request->params();
        for(int i = 0; i < params; i++) {
            const AsyncWebParameter* p = request->getParam(i);

            if(p->isPost()){
            if (p->name() == MQTT_PARAM_INPUT_1) {
                mqttServer = p->value().c_str();            
                Filesys.writeFile(mqttServerPath, mqttServer.c_str());
            }

            if (p->name() == MQTT_PARAM_INPUT_2) {
                mqttUsername = p->value().c_str();
                Filesys.writeFile(mqttUsernamePath, mqttUsername.c_str());
            }

            if (p->name() == MQTT_PARAM_INPUT_3) {
                mqttPassword = p->value().c_str();
                Filesys.writeFile(mqttPasswordPath, mqttPassword.c_str());
            }

            if (p->name() == MQTT_PARAM_INPUT_4) {
                mqttHostname = p->value().c_str();            
                Filesys.writeFile(mqttHostnamePath, mqttHostname.c_str());
            }

            if (p->name() == MQTT_PARAM_INPUT_5) {
                mqttTopic = p->value().c_str();
                Filesys.writeFile(mqttTopicPath, mqttTopic.c_str());
            }
            }
        }
        //restart = true;
        request->send(LittleFS, "/index.html", "text/html", false);//, processor);
    });
}