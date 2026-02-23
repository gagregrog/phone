#include "DurationParser.h"
#include <stdint.h>
#include <stdbool.h>

unsigned long parseDuration(const char* str) {
  if (!str || !*str) return 0;

  unsigned long totalSec = 0;
  unsigned long current = 0;
  bool hasDigits = false;
  uint8_t unitCount = 0;   // how many unit suffixes seen
  char lastUnit = 0;       // last suffix: 'h', 'm', or 's'

  for (const char* p = str; *p; p++) {
    char c = *p;
    if (c >= '0' && c <= '9') {
      current = current * 10 + (c - '0');
      hasDigits = true;
    } else if (c == 'h' || c == 'm' || c == 's') {
      if (!hasDigits) return 0;
      // Enforce order: h before m before s, no repeats
      if (lastUnit == 'h' && c != 'm' && c != 's') return 0;
      if (lastUnit == 'm' && c != 's') return 0;
      if (lastUnit == 's') return 0;
      if (lastUnit == c) return 0;
      // Cap sub-units at 59 when a larger unit is present
      if (unitCount > 0 && current > 59) return 0;
      if (c == 'h') totalSec += current * 3600;
      else if (c == 'm') totalSec += current * 60;
      else totalSec += current;
      lastUnit = c;
      unitCount++;
      current = 0;
      hasDigits = false;
    } else {
      return 0;  // invalid character
    }
  }

  // Trailing digits without a suffix — reject
  if (hasDigits) return 0;

  if (totalSec == 0) return 0;

  // Validate range: 1 second to 24 hours
  if (totalSec > 86400) return 0;

  return totalSec * 1000UL;
}
