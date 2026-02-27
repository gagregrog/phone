#pragma once
#include <ArduinoJson.h>

void ringStartedFillJson(JsonObject obj, const char* pattern);
void ringStoppedFillJson(JsonObject obj);
