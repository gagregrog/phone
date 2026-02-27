#include "hardware/DialAPI.h"
#include "hardware/DialJSON.h"
#include "web/API.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

static DialReader* _dial = nullptr;

void dialAPIBegin(DialReader& dial) {
    _dial = &dial;
    apiGetServer()->on("/dial/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        dialStatusFillJson(doc.to<JsonObject>(), _dial->isDialing());
        String body;
        serializeJson(doc, body);
        request->send(200, "application/json", body);
    });
}
