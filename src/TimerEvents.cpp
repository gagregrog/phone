#include "TimerEvents.h"
#include "Events.h"
#include <ArduinoJson.h>

void timerEventsBegin(Timer& timer) {
    timer.setOnFire([](uint32_t id, const char* pattern) {
        char buf[64];
        JsonDocument doc;
        doc["id"] = id;
        doc["pattern"] = pattern;
        serializeJson(doc, buf, sizeof(buf));
        eventsPublish("timer/expired", buf);
        doc.clear();
        doc["pattern"] = pattern;
        serializeJson(doc, buf, sizeof(buf));
        eventsPublish("ring/started", buf);
    });
}
