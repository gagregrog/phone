#pragma once
#include <ArduinoJson.h>

inline void handsetFillJson(JsonObject obj, bool offHook) {
    obj["offHook"] = offHook;
}
