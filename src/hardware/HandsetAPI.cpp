#include "hardware/HandsetAPI.h"
#include "hardware/HandsetJSON.h"
#include "web/API.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

static HandsetMonitor* _handset = nullptr;

void handsetAPIBegin(HandsetMonitor& handset) {
    _handset = &handset;
    apiGetServer()->on("/handset/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        handsetFillJson(doc.to<JsonObject>(), _handset->isOffHook());
        String body;
        serializeJson(doc, body);
        request->send(200, "application/json", body);
    });
}
