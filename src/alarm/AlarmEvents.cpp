#include "alarm/AlarmEvents.h"
#include "alarm/AlarmJSON.h"
#include "ringer/RingerEvents.h"
#include "web/Events.h"
#include <ArduinoJson.h>

void alarmEventsBegin(AlarmManager& mgr) {
    mgr.setOnFire([](const AlarmEntry& e) {
        JsonDocument doc;
        alarmFillJson(doc.to<JsonObject>(), e);
        String body;
        serializeJson(doc, body);
        eventsPublish("alarm/fired", body.c_str());
        publishRingStarted(e.patternName);
    });
}
