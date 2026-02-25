#pragma once
#include "AlarmEntry.h"
#include <ArduinoJson.h>

void alarmFillJson(JsonObject obj, const AlarmEntry& e);
