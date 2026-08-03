#pragma once
#include "phonebook/PhoneBookEntry.h"

// Populates url, method, body, headers, and the canonical On/Off/M1-M4
// extensions for a "wled"-type entry from its wledHost field. Called
// whenever such an entry is added or updated so it behaves identically to a
// hand-built "http" entry (with extensions) everywhere else in the code.
void wledApplyDefaults(PhoneBookEntry& entry);
