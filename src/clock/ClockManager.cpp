#include "clock/ClockManager.h"
#include "ringer/RingPattern.h"

ClockManager::ClockManager(Ringer& ringer, std::function<bool(struct tm*)> getTime)
    : _ringer(ringer), _getTime(getTime),
      _enabled(false), _chimeMode(CHIME_N_CHIMES),
      _lastCheckedMinuteKey((uint32_t)-1) {}

void ClockManager::setOnChime(std::function<void(uint16_t)> cb) {
    _onChime = std::move(cb);
}

bool ClockManager::isEnabled() const {
    return _enabled;
}

void ClockManager::setEnabled(bool enabled) {
    _enabled = enabled;
}

ChimeMode ClockManager::getChimeMode() const {
    return _chimeMode;
}

void ClockManager::setChimeMode(ChimeMode mode) {
    _chimeMode = mode;
}

void ClockManager::tick() {
    struct tm now;
    if (!_getTime(&now)) return;

    uint32_t minuteKey = (uint32_t)now.tm_yday * 1440
                       + (uint32_t)now.tm_hour * 60
                       + (uint32_t)now.tm_min;
    if (minuteKey == _lastCheckedMinuteKey) return;
    _lastCheckedMinuteKey = minuteKey;

    if (!_enabled) return;
    if (now.tm_min != 0) return;
    if (_ringer.isRinging()) return;

    uint16_t rings;
    if (_chimeMode == CHIME_N_CHIMES) {
        rings = (now.tm_hour % 12 == 0) ? 12 : (uint16_t)(now.tm_hour % 12);
    } else {
        rings = 1;
    }
    _ringer.ring(PATTERN_CHIME, rings);
    if (_onChime) _onChime(rings);
}
