#pragma once
#include "Timer.h"
#include <ArduinoJson.h>

void timerInfoFillJson(JsonObject obj, const TimerInfo& info);
