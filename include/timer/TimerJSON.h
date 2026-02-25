#pragma once
#include "timer/Timer.h"
#include <ArduinoJson.h>

void timerInfoFillJson(JsonObject obj, const TimerInfo& info);
