#include "clock/ClockEvents.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ArduinoJson.h>

void clockEventsBegin(ClockManager& mgr) {
    mgr.setOnChime([](uint16_t rings) {
        logger.schedulerf("Clock chimed: %u rings", (unsigned)rings);
        char buf[32];
        JsonDocument doc;
        doc["rings"] = rings;
        serializeJson(doc, buf, sizeof(buf));
        eventsPublish("clock/chimed", buf);
    });
}
