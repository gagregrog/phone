#pragma once
#include "ringer/Ringer.h"

void ringerEventsBegin(Ringer& ringer);
void publishRingStarted(const char* pattern);
void publishRingStopped();
