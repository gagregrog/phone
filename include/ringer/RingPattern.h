#pragma once
#include <stdint.h>

struct RingPattern {
  const char* name;
  const unsigned long* phases;  // alternating ON/OFF durations in ms
  uint8_t phaseCount;
};

// Built-in patterns
extern const RingPattern PATTERN_US;
extern const RingPattern PATTERN_UK;
extern const RingPattern PATTERN_DE;
extern const RingPattern PATTERN_FR;
extern const RingPattern PATTERN_JP;
extern const RingPattern PATTERN_IT;
extern const RingPattern PATTERN_SE;
extern const RingPattern PATTERN_CHIRP;
extern const RingPattern PATTERN_CHIME;
extern const RingPattern PATTERN_PIP;

// Number of built-in patterns
extern const uint8_t PATTERN_COUNT;

// Array of all built-in patterns (for enumeration)
extern const RingPattern* const ALL_PATTERNS[];

// Look up a pattern by name (case-insensitive). Returns nullptr if not found.
const RingPattern* findPattern(const char* name);
