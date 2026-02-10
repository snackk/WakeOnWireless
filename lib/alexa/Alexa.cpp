#include <Alexa.h>

AlexaClass Alexa;

void AlexaClass::initAlexa(AsyncWebServer* server, std::function<void(bool)> onMessageFunc) {
    // Use shared AsyncWebServer
    fauxmo.createServer(false);
    fauxmo.setPort(80);        // required for Alexa Gen3+ devices
    fauxmo.enable(true);

    // Alexa device(s)
    fauxmo.addDevice(ALEXA_DEVICE_NAME); 

    Serial.println("[FAUXMO] - Setup done via AsyncWebServer");

    fauxmo.onSetState([=](unsigned char deviceId, const char* deviceName, bool state, unsigned char value) {
        Serial.printf("[FAUXMO] - Device #%d (%s) state: %s value: %d\n",
                     deviceId, deviceName, state ? "ON" : "OFF", value);
        onMessageFunc(state);
    });

    server->onNotFound([this](AsyncWebServerRequest *request){
        String body = ""; 
        if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body)) {
            return; 
        }
        
        request->send(404, "text/plain", "Not found");
    });
}

void AlexaClass::loopAlexa() {
    // Only handle if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        fauxmo.handle(); 
    }
}
