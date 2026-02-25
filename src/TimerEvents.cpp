#include "TimerEvents.h"
#include "TimerJSON.h"
#include "Events.h"
#include <ArduinoJson.h>

void timerEventsBegin(Timer& timer) {
    timer.setOnFire([](const TimerInfo& info) {
        JsonDocument doc;
        timerInfoFillJson(doc.to<JsonObject>(), info);
        String body;
        serializeJson(doc, body);
        eventsPublish("timer/expired", body.c_str());
        doc.clear();
        doc["pattern"] = info.patternName;
        String ringBody;
        serializeJson(doc, ringBody);
        eventsPublish("ring/started", ringBody.c_str());
    });
}
