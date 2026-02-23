#pragma once
#include "Ringer.h"

// Sets up the async REST API on the given port and starts listening.
// Endpoints:
void ringerAPIBegin(Ringer& ringer, uint16_t port = 80);
