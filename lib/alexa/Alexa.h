#ifndef Alexa_H_
#define Alexa_H_

#include <fauxmoESP.h>

class AlexaClass {
    
    public:
        void
            initAlexa(std::function<void(bool)> onMessageFunc),
            loopAlexa();
    
    private:
        const char* ALEXA_DEVICE_NAME = "SOUSA";
        fauxmoESP fauxmo;
};

extern AlexaClass Alexa;

#endif