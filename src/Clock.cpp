#include "Clock.h"
#include <Arduino.h>

void clockBegin(const char* tzString, const char* ntpServer) {
    configTzTime(tzString, ntpServer);
}

bool clockGetLocalTime(struct tm* t) {
    return getLocalTime(t, 0);
}
