#include "web/DeviceAPI.h"
#include "web/API.h"
#include "system/Logger.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

void deviceAPIBegin() {
    AsyncWebServer* server = apiGetServer();

    apiAddStatusContributor("device", [](JsonDocument& doc, const char* key) {
        doc[key].to<JsonObject>()["ip"] = WiFi.localIP().toString();
    });

    server->on("/ip", HTTP_GET, [](AsyncWebServerRequest* request) {
        logger.apif("[%s] GET /ip", request->client()->remoteIP().toString().c_str());
        request->send(200, "text/plain", WiFi.localIP().toString());
    });
}
