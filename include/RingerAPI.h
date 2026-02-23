#pragma once
#include "Ringer.h"
#include "Timer.h"

// Sets up the async REST API on the given port and starts listening.
void ringerAPIBegin(Ringer& ringer, Timer& timer, uint16_t port = 80);
