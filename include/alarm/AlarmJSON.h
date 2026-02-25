#pragma once
#include "alarm/AlarmEntry.h"
#include <ArduinoJson.h>

void alarmFillJson(JsonObject obj, const AlarmEntry& e);
