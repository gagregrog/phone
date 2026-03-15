#include "phone/PhoneAPI.h"
#include "web/API.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static const char* phoneStateStr(PhoneState s) {
    switch (s) {
        case PhoneState::RINGING:  return "RINGING";
        case PhoneState::OFF_HOOK: return "OFF_HOOK";
        case PhoneState::DIALING:  return "DIALING";
        case PhoneState::CALL_OUT: return "CALL_OUT";
        case PhoneState::IN_CALL:  return "IN_CALL";
        default:                   return "IDLE";
    }
}

void phoneAPIBegin(PhoneController& phone) {
    apiAddStatusContributor("phone", [&phone](JsonDocument& doc, const char* key) {
        doc[key].to<JsonObject>()["state"] = phoneStateStr(phone.getState());
    });

    apiGetServer()->on("/phone/state", HTTP_GET, [&phone](AsyncWebServerRequest* request) {
        logger.apif("[%s] GET /phone/state", request->client()->remoteIP().toString().c_str());
        char body[32];
        snprintf(body, sizeof(body), "{\"state\":\"%s\"}", phoneStateStr(phone.getState()));
        request->send(200, "application/json", body);
    });
}
