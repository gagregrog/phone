#pragma once
#include "hardware/MicReader.h"

void micEventsBegin(MicReader& mic);

// Manual enable/disable — mic streams only when enabled AND WS clients connected.
// Default: disabled. Publishes "mic/status" event on change.
void micSetEnabled(bool enabled);
bool micIsEnabled();
