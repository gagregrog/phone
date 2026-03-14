#include "hardware/MicAPI.h"
#include "hardware/MicEvents.h"
#include "web/API.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>

void micAPIBegin(MicReader& mic) {
    AsyncWebServer* server = apiGetServer();

    server->on("/mic/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        logger.apif("[%s] GET /mic/status", req->client()->remoteIP().toString().c_str());
        const char* body = micIsEnabled()
            ? "{\"enabled\":true}"
            : "{\"enabled\":false}";
        req->send(200, "application/json", body);
    });

    server->on("/mic/enable", HTTP_POST, [](AsyncWebServerRequest* req) {
        logger.apif("[%s] POST /mic/enable", req->client()->remoteIP().toString().c_str());
        micSetEnabled(true);
        req->send(200, "application/json", "{\"enabled\":true}");
    });

    server->on("/mic/disable", HTTP_POST, [](AsyncWebServerRequest* req) {
        logger.apif("[%s] POST /mic/disable", req->client()->remoteIP().toString().c_str());
        micSetEnabled(false);
        req->send(200, "application/json", "{\"enabled\":false}");
    });
}
