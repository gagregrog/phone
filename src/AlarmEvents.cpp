#include "AlarmEvents.h"
#include "AlarmJSON.h"
#include "Events.h"
#include <ArduinoJson.h>

void alarmEventsBegin(AlarmManager& mgr) {
    mgr.setOnFire([](const AlarmEntry& e) {
        JsonDocument doc;
        alarmFillJson(doc.to<JsonObject>(), e);
        String body;
        serializeJson(doc, body);
        eventsPublish("alarm/fired", body.c_str());
        doc.clear();
        doc["pattern"] = e.patternName;
        String ringBody;
        serializeJson(doc, ringBody);
        eventsPublish("ring/started", ringBody.c_str());
    });
}
