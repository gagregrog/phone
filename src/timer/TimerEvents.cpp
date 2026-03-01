#include "timer/TimerEvents.h"
#include "timer/TimerJSON.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ArduinoJson.h>

void timerEventsBegin(Timer& timer) {
    timer.setOnFire([](const TimerInfo& info) {
        logger.schedulerf("Timer expired: id=%u pattern=%s", (unsigned)info.id, info.patternName);
        JsonDocument doc;
        timerInfoFillJson(doc.to<JsonObject>(), info);
        String body;
        serializeJson(doc, body);
        eventsPublish("timer/expired", body.c_str());
    });
}
