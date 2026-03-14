#pragma once
#include "phonebook/PhoneBookEntry.h"
#include <ArduinoJson.h>

// Serialize a PhoneBookEntry to a JSON object.
// When mask=true (default), header values containing "Authorization" or "Token"
// in their name are replaced with "********".
void phoneBookFillJson(JsonObject obj, const PhoneBookEntry& e, bool mask = true);

// Parse a PhoneBookEntry from a JSON object.
// When mergeFrom is non-null and a header value equals "********", the existing
// value from mergeFrom is preserved (so masked values aren't overwritten on PUT).
void phoneBookParseJson(JsonObject obj, PhoneBookEntry& e, const PhoneBookEntry* mergeFrom = nullptr);
