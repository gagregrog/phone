#include "hardware/DialManagerEvents.h"
#include "hardware/DialManagerJSON.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ArduinoJson.h>

void dialManagerEventsBegin(DialManager& mgr) {
    mgr.addOnDialStart([]() {
        JsonDocument doc;
        dialManagerDialingFillJson(doc.to<JsonObject>());
        String body;
        serializeJson(doc, body);
        eventsPublish("phone/dialing", body.c_str());
    });

    mgr.addOnDigit([](int digit, const char* number) {
        logger.infof("Dialed: %d  number: %s", digit, number);
        JsonDocument doc;
        dialManagerDigitFillJson(doc.to<JsonObject>(), digit, number);
        String body;
        serializeJson(doc, body);
        eventsPublish("phone/digit", body.c_str());
    });

    mgr.addOnClear([]() {
        JsonDocument doc;
        dialManagerClearFillJson(doc.to<JsonObject>());
        String body;
        serializeJson(doc, body);
        eventsPublish("phone/clear", body.c_str());
    });
}
