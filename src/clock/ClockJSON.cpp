#include "clock/ClockJSON.h"

const char* chimeModeToString(ChimeMode mode) {
    return (mode == CHIME_N_CHIMES) ? "n_chimes" : "single";
}

void clockStateFillJson(JsonObject obj, bool enabled, ChimeMode mode) {
    obj["enabled"] = enabled;
    obj["mode"]    = chimeModeToString(mode);
}
