#pragma once
#include "ClockManager.h"
#include <ArduinoJson.h>

const char* chimeModeToString(ChimeMode mode);
void clockStateFillJson(JsonObject obj, bool enabled, ChimeMode mode);
