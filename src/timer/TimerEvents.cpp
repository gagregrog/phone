#include "timer/TimerEvents.h"
#include "timer/TimerJSON.h"
#include "ringer/RingerEvents.h"
#include "web/Events.h"
#include <ArduinoJson.h>

void timerEventsBegin(Timer& timer) {
    timer.setOnFire([](const TimerInfo& info) {
        JsonDocument doc;
        timerInfoFillJson(doc.to<JsonObject>(), info);
        String body;
        serializeJson(doc, body);
        eventsPublish("timer/expired", body.c_str());
        publishRingStarted(info.patternName);
    });
}
