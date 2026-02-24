#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parse a duration string (e.g. "1h30m", "20m", "90s") into milliseconds.
// Units must appear in h > m > s order. At least one suffix required.
// Sub-units capped at 59 when a larger unit is present.
// Returns 0 on parse error or out-of-range (max 24h).
unsigned long parseDuration(const char* str);

// Format a duration in seconds as "H:MM:SS" (e.g. 3661 -> "1:01:01").
// buf must be at least 10 bytes.
void formatDuration(unsigned long totalSec, char* buf, size_t bufSize);

#ifdef __cplusplus
}
#endif
