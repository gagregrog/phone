#include "alarm/AlarmJSON.h"
#include <stdio.h>

void alarmFillJson(JsonObject obj, const AlarmEntry& e) {
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", (unsigned)e.hour, (unsigned)e.minute);
    obj["id"]           = e.id;
    obj["time"]         = timeBuf;
    obj["pattern"]      = e.patternName;
    obj["rings"]        = e.rings;
    obj["repeat"]       = e.repeat;
    obj["skipWeekends"] = e.skipWeekends;
    obj["enabled"]      = e.enabled;
}
