#include "AlarmEvents.h"
#include "Events.h"
#include <ArduinoJson.h>

void alarmEventsBegin(AlarmManager& mgr) {
    mgr.setOnFire([](const AlarmEntry& e) {
        char timeBuf[6];
        snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", (unsigned)e.hour, (unsigned)e.minute);
        char buf[96];
        JsonDocument doc;
        doc["id"] = e.id;
        doc["time"] = timeBuf;
        doc["pattern"] = e.patternName;
        serializeJson(doc, buf, sizeof(buf));
        eventsPublish("alarm/fired", buf);
        doc.clear();
        doc["pattern"] = e.patternName;
        serializeJson(doc, buf, sizeof(buf));
        eventsPublish("ring/started", buf);
    });
}
