#pragma once
#include <time.h>

void alarmClockBegin(const char* tzString, const char* ntpServer = "pool.ntp.org");
bool alarmClockGetLocalTime(struct tm* t);
