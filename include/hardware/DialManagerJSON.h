#pragma once
#include <ArduinoJson.h>

inline void dialManagerStatusFillJson(JsonObject obj, bool offHook, const char* number) {
    obj["offHook"] = offHook;
    obj["number"]  = number;
}

inline void dialManagerDialingFillJson(JsonObject obj) {
    obj["dialing"] = true;
}

inline void dialManagerDigitFillJson(JsonObject obj, int digit, const char* number) {
    obj["digit"]  = digit;
    obj["number"] = number;
}

inline void dialManagerClearFillJson(JsonObject obj) {
    obj["number"] = "";
}
