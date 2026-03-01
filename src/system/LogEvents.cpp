#include "system/LogEvents.h"
#include "system/Logger.h"
#include "clock/Clock.h"
#include "web/Events.h"
#include <ArduinoJson.h>

void logEventsBegin() {
    logger.setOnLog([](const char* level, const char* category, const char* msg) {
        JsonDocument doc;
        doc["level"] = level;
        if (category && category[0] != '\0') doc["category"] = category;
        doc["msg"]   = msg;
        struct tm t;
        if (clockGetLocalTime(&t)) {
            char timeBuf[9];
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                     t.tm_hour, t.tm_min, t.tm_sec);
            doc["time"] = timeBuf;
        }
        String body;
        serializeJson(doc, body);
        eventsPublish("log/message", body.c_str());
    });
}
