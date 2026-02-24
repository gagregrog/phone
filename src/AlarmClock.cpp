#include "AlarmClock.h"
#include <Arduino.h>

void alarmClockBegin(const char* tzString, const char* ntpServer) {
    configTzTime(tzString, ntpServer);
}

bool alarmClockGetLocalTime(struct tm* t) {
    return getLocalTime(t, 0);
}
