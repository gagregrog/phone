#include "ringer/RingerJSON.h"

void ringStartedFillJson(JsonObject obj, const char* pattern) {
    obj["ringing"] = true;
    obj["pattern"] = pattern;
}

void ringStoppedFillJson(JsonObject obj) {
    obj["ringing"] = false;
}
