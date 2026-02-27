#pragma once
#include <ArduinoJson.h>

inline void dialStatusFillJson(JsonObject obj, bool dialing) {
    obj["dialing"] = dialing;
}

inline void dialDigitFillJson(JsonObject obj, int digit) {
    obj["digit"] = digit;
}
