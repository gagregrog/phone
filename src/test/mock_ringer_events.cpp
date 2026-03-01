// No-op stubs for native builds — RingerEvents.cpp uses ArduinoJson/String
// which are mocked but not fully functional in the native test environment.
#include "ringer/RingerEvents.h"

void ringerEventsBegin(Ringer&) {}
void publishRingStarted(const char*) {}
void publishRingStopped() {}
