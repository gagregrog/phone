#include "RingPattern.h"
#include <string.h>

// Phase arrays (alternating ON/OFF durations in ms)
static const unsigned long PHASES_US[] = {2000, 4000};
static const unsigned long PHASES_UK[] = {400, 200, 400, 2000};
static const unsigned long PHASES_DE[] = {1000, 4000};
static const unsigned long PHASES_FR[] = {1500, 3500};
static const unsigned long PHASES_JP[] = {1000, 2000};
static const unsigned long PHASES_IT[] = {1000, 1000, 1000, 3000};
static const unsigned long PHASES_SE[] = {1000, 5000};
static const unsigned long PHASES_CHIRP[] = {150, 100, 150, 600};

const RingPattern PATTERN_US = {"us", PHASES_US, 2};
const RingPattern PATTERN_UK = {"uk", PHASES_UK, 4};
const RingPattern PATTERN_DE = {"de", PHASES_DE, 2};
const RingPattern PATTERN_FR = {"fr", PHASES_FR, 2};
const RingPattern PATTERN_JP = {"jp", PHASES_JP, 2};
const RingPattern PATTERN_IT = {"it", PHASES_IT, 4};
const RingPattern PATTERN_SE = {"se", PHASES_SE, 2};
const RingPattern PATTERN_CHIRP = {"chirp", PHASES_CHIRP, 4};

const RingPattern* const ALL_PATTERNS[] = {
  &PATTERN_US, &PATTERN_UK, &PATTERN_DE, &PATTERN_FR,
  &PATTERN_JP, &PATTERN_IT, &PATTERN_SE,
  &PATTERN_CHIRP
};

const uint8_t PATTERN_COUNT = sizeof(ALL_PATTERNS) / sizeof(ALL_PATTERNS[0]);

const RingPattern* findPattern(const char* name) {
  for (uint8_t i = 0; i < PATTERN_COUNT; i++) {
    if (strcasecmp(name, ALL_PATTERNS[i]->name) == 0) {
      return ALL_PATTERNS[i];
    }
  }
  return nullptr;
}
