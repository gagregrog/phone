#include "TimerJSON.h"
#include "DurationParser.h"

void timerInfoFillJson(JsonObject obj, const TimerInfo& info) {
    char buf[10];
    obj["id"] = info.id;
    formatDuration((info.remainingMs + 500) / 1000, buf, sizeof(buf));
    obj["remaining"] = buf;
    formatDuration(info.totalMs / 1000, buf, sizeof(buf));
    obj["total"] = buf;
    obj["pattern"] = info.patternName;
}
