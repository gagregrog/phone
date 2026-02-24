#pragma once
#include <stdint.h>

struct AlarmEntry {
    uint32_t id;
    uint8_t  hour;           // 0-23 local time
    uint8_t  minute;         // 0-59
    uint16_t rings;          // 0 = infinite loop
    bool     repeat;
    bool     skipWeekends;   // skip Sat (wday=6) and Sun (wday=0)
    bool     enabled;
    char     patternName[16];
};
