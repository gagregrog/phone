#include "NVSAlarmStore.h"
#include "Logger.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <string.h>

static const char* NS  = "alarms";
static const char* KEY = "alarms";

void NVSAlarmStore::load(std::vector<AlarmEntry>& alarms) {
    Preferences prefs;
    prefs.begin(NS, true);
    String json = prefs.getString(KEY, "");
    prefs.end();

    if (json.isEmpty()) return;

    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        logger.warn("NVSAlarmStore: failed to parse JSON");
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        AlarmEntry e;
        e.id          = obj["id"]           | (uint32_t)0;
        e.hour        = obj["hour"]          | (uint8_t)0;
        e.minute      = obj["minute"]        | (uint8_t)0;
        e.rings       = obj["rings"]         | (uint16_t)0;
        e.repeat      = obj["repeat"]        | false;
        e.skipWeekends = obj["skipWeekends"] | false;
        e.enabled     = obj["enabled"]       | true;
        const char* name = obj["pattern"]    | "";
        strncpy(e.patternName, name, sizeof(e.patternName) - 1);
        e.patternName[sizeof(e.patternName) - 1] = '\0';
        alarms.push_back(e);
    }
    logger.infof("NVSAlarmStore: loaded %u alarm(s)", (unsigned)alarms.size());
}

void NVSAlarmStore::save(const std::vector<AlarmEntry>& alarms) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& e : alarms) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]          = e.id;
        obj["hour"]        = e.hour;
        obj["minute"]      = e.minute;
        obj["rings"]       = e.rings;
        obj["repeat"]      = e.repeat;
        obj["skipWeekends"] = e.skipWeekends;
        obj["enabled"]     = e.enabled;
        obj["pattern"]     = e.patternName;
    }

    String json;
    serializeJson(doc, json);

    Preferences prefs;
    prefs.begin(NS, false);
    prefs.putString(KEY, json);
    prefs.end();

    logger.infof("NVSAlarmStore: saved %u alarm(s)", (unsigned)alarms.size());
}
