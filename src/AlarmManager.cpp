#include "AlarmManager.h"
#include "RingPattern.h"
#include <string.h>

AlarmManager::AlarmManager(Ringer& ringer, AlarmStore& store, TimeFunc getTime)
    : _ringer(ringer), _store(store), _getTime(getTime),
      _nextId(1), _lastCheckedMinuteKey((uint32_t)-1) {}

void AlarmManager::setOnFire(std::function<void(const AlarmEntry&)> cb) {
    _onFire = std::move(cb);
}

uint32_t AlarmManager::add(uint8_t hour, uint8_t minute, const char* pattern,
                            uint16_t rings, bool repeat, bool skipWeekends) {
    AlarmEntry e;
    e.id = _nextId++;
    e.hour = hour;
    e.minute = minute;
    e.rings = rings;
    e.repeat = repeat;
    e.skipWeekends = skipWeekends;
    e.enabled = true;
    strncpy(e.patternName, pattern, sizeof(e.patternName) - 1);
    e.patternName[sizeof(e.patternName) - 1] = '\0';
    _alarms.push_back(e);
    if (repeat) save();
    return e.id;
}

bool AlarmManager::update(uint32_t id, uint8_t hour, uint8_t minute,
                           const char* pattern, uint16_t rings, bool repeat, bool skipWeekends) {
    for (auto& e : _alarms) {
        if (e.id == id) {
            e.hour = hour;
            e.minute = minute;
            e.rings = rings;
            e.repeat = repeat;
            e.skipWeekends = skipWeekends;
            e.enabled = true;
            strncpy(e.patternName, pattern, sizeof(e.patternName) - 1);
            e.patternName[sizeof(e.patternName) - 1] = '\0';
            save();
            return true;
        }
    }
    return false;
}

bool AlarmManager::remove(uint32_t id) {
    for (auto it = _alarms.begin(); it != _alarms.end(); ++it) {
        if (it->id == id) {
            _alarms.erase(it);
            save();
            return true;
        }
    }
    return false;
}

void AlarmManager::removeAll() {
    _alarms.clear();
    save();
}

const std::vector<AlarmEntry>& AlarmManager::getAll() const {
    return _alarms;
}

bool AlarmManager::isTimeInFuture(uint8_t hour, uint8_t minute) const {
    struct tm now;
    if (!_getTime(&now)) return false;
    int nowMins    = now.tm_hour * 60 + now.tm_min;
    int targetMins = (int)hour * 60 + (int)minute;
    return targetMins > nowMins;
}

void AlarmManager::init() {
    _store.load(_alarms);
    for (const auto& e : _alarms) {
        if (e.id >= _nextId) {
            _nextId = e.id + 1;
        }
    }
}

void AlarmManager::tick() {
    struct tm now;
    if (!_getTime(&now)) return;

    uint32_t minuteKey = (uint32_t)now.tm_yday * 1440
                       + (uint32_t)now.tm_hour * 60
                       + (uint32_t)now.tm_min;
    if (minuteKey == _lastCheckedMinuteKey) return;
    _lastCheckedMinuteKey = minuteKey;

    std::vector<AlarmEntry> fired;
    bool changed = false;
    for (auto& e : _alarms) {
        if (!e.enabled) continue;
        if (e.hour != (uint8_t)now.tm_hour || e.minute != (uint8_t)now.tm_min) continue;
        if (e.skipWeekends && (now.tm_wday == 0 || now.tm_wday == 6)) continue;

        const RingPattern* p = findPattern(e.patternName);
        if (p) {
            _ringer.ring(*p, e.rings);
        }

        fired.push_back(e);

        if (!e.repeat) {
            e.enabled = false;
            changed = true;
        }
    }

    if (changed) save();

    for (const auto& e : fired) {
        if (_onFire) _onFire(e);
    }
}

void AlarmManager::save() {
    std::vector<AlarmEntry> toSave;
    for (const auto& e : _alarms) {
        if (e.repeat) toSave.push_back(e);
    }
    _store.save(toSave);
}
