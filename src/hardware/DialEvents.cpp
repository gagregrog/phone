#include "hardware/DialEvents.h"
#include "hardware/DialJSON.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ArduinoJson.h>

void dialEventsBegin(DialReader& dial) {
    dial.setOnDialStart([]() {
        JsonDocument doc;
        dialStatusFillJson(doc.to<JsonObject>(), true);
        String body;
        serializeJson(doc, body);
        eventsPublish("dial/dialing", body.c_str());
    });

    dial.setOnDigit([](int digit) {
        logger.infof("Dialed: %d", digit);
        JsonDocument doc;
        dialDigitFillJson(doc.to<JsonObject>(), digit);
        String body;
        serializeJson(doc, body);
        eventsPublish("dial/digit", body.c_str());
    });
}
