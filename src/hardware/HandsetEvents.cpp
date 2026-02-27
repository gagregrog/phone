#include "hardware/HandsetEvents.h"
#include "hardware/HandsetJSON.h"
#include "web/Events.h"
#include <ArduinoJson.h>

void handsetEventsBegin(HandsetMonitor& handset) {
    handset.setOnChange([](bool offHook) {
        JsonDocument doc;
        handsetFillJson(doc.to<JsonObject>(), offHook);
        String body;
        serializeJson(doc, body);
        eventsPublish(offHook ? "handset/up" : "handset/down", body.c_str());
    });
}
