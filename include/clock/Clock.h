#pragma once
#include <time.h>

void clockBegin(const char* tzString, const char* ntpServer = "pool.ntp.org");
bool clockGetLocalTime(struct tm* t);
