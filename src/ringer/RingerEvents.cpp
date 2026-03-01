#include "ringer/RingerEvents.h"
#include "ringer/RingerJSON.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ArduinoJson.h>

void ringerEventsBegin(Ringer& ringer) {
    ringer.setOnStop([]{
        logger.hardware("Ring stopped");
        publishRingStopped();
    });
    ringer.setOnStart([](const char* pattern){
        logger.hardwaref("Ring started: %s", pattern);
        publishRingStarted(pattern);
    });
}

void publishRingStarted(const char* pattern) {
    JsonDocument doc;
    ringStartedFillJson(doc.to<JsonObject>(), pattern);
    String body;
    serializeJson(doc, body);
    eventsPublish("ring/started", body.c_str());
}

void publishRingStopped() {
    JsonDocument doc;
    ringStoppedFillJson(doc.to<JsonObject>());
    String body;
    serializeJson(doc, body);
    eventsPublish("ring/stopped", body.c_str());
}
