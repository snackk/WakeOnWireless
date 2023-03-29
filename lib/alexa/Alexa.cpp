#include <Alexa.h>

AlexaClass Alexa;

void AlexaClass::initAlexa(std::function<void(bool)> onMessageFunc) {
  fauxmo.addDevice(ALEXA_DEVICE_NAME);
  
  fauxmo.setPort(80);
  fauxmo.enable(true);
  Serial.println("[FAUXMO] - Setup done");

  fauxmo.onSetState([&](unsigned char deviceId, const char* deviceName, bool state, unsigned char value) {

    Serial.printf("[FAUXMO] - Device #%d (%s) state: %s value: %d\n", deviceId, deviceName, state ? "ON" : "OFF", value);
    onMessageFunc(state);
  });
}

void AlexaClass::loopAlexa() {
    fauxmo.handle();
}