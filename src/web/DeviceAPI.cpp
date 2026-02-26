#include "web/DeviceAPI.h"
#include "web/API.h"
#include "system/Logger.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

void deviceAPIBegin() {
    AsyncWebServer* server = apiGetServer();

    server->on("/ip", HTTP_GET, [](AsyncWebServerRequest* request) {
        logger.infof("[%s] GET /ip", request->client()->remoteIP().toString().c_str());
        request->send(200, "text/plain", WiFi.localIP().toString());
    });
}
