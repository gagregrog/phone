#include "hardware/DialManagerAPI.h"
#include "hardware/DialManagerJSON.h"
#include "web/API.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void dialManagerAPIBegin(DialManager& mgr) {
    apiGetServer()->on("/phone/status", HTTP_GET, [&mgr](AsyncWebServerRequest* request) {
        JsonDocument doc;
        dialManagerStatusFillJson(doc.to<JsonObject>(), mgr.isOffHook(), mgr.number());
        String body;
        serializeJson(doc, body);
        request->send(200, "application/json", body);
    });
}
