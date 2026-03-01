#include "alarm/AlarmEvents.h"
#include "alarm/AlarmJSON.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ArduinoJson.h>

void alarmEventsBegin(AlarmManager& mgr) {
    mgr.setOnFire([](const AlarmEntry& e) {
        logger.infof("Alarm fired: id=%u %02u:%02u pattern=%s", (unsigned)e.id, e.hour, e.minute, e.patternName);
        JsonDocument doc;
        alarmFillJson(doc.to<JsonObject>(), e);
        String body;
        serializeJson(doc, body);
        eventsPublish("alarm/fired", body.c_str());
    });
}
