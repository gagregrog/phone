#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Parse a duration string (e.g. "1h30m", "20m", "90s") into milliseconds.
// Units must appear in h > m > s order. At least one suffix required.
// Sub-units capped at 59 when a larger unit is present.
// Returns 0 on parse error or out-of-range (max 24h).
unsigned long parseDuration(const char* str);

#ifdef __cplusplus
}
#endif
