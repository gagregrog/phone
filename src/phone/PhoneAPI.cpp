#include "phone/PhoneAPI.h"
#include "web/API.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>

void phoneAPIBegin(PhoneController& phone) {
    apiGetServer()->on("/phone/state", HTTP_GET, [&phone](AsyncWebServerRequest* request) {
        logger.apif("[%s] GET /phone/state", request->client()->remoteIP().toString().c_str());
        const char* state;
        switch (phone.getState()) {
            case PhoneState::RINGING:  state = "RINGING";  break;
            case PhoneState::OFF_HOOK: state = "OFF_HOOK"; break;
            case PhoneState::DIALING:  state = "DIALING";  break;
            case PhoneState::CALL_OUT: state = "CALL_OUT"; break;
            case PhoneState::IN_CALL:  state = "IN_CALL";  break;
            default:                   state = "IDLE";     break;
        }
        char body[32];
        snprintf(body, sizeof(body), "{\"state\":\"%s\"}", state);
        request->send(200, "application/json", body);
    });
}
