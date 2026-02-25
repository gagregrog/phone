#pragma once
#include "AlarmEntry.h"
#include "AlarmStore.h"
#include "Ringer.h"
#include <functional>
#include <time.h>
#include <vector>

using TimeFunc = std::function<bool(struct tm*)>;

class AlarmManager {
public:
    AlarmManager(Ringer& ringer, AlarmStore& store, TimeFunc getTime);

    uint32_t add(uint8_t hour, uint8_t minute, const char* pattern,
                 uint16_t rings, bool repeat, bool skipWeekends);
    bool     update(uint32_t id, uint8_t hour, uint8_t minute,
                    const char* pattern, uint16_t rings, bool repeat, bool skipWeekends);
    bool     remove(uint32_t id);
    void     removeAll();
    const std::vector<AlarmEntry>& getAll() const;

    // Returns true if the given HH:MM is still in the future today.
    // Returns false if time is not synced or if the time has already passed today.
    bool     isTimeInFuture(uint8_t hour, uint8_t minute) const;

    void     setOnFire(std::function<void(const AlarmEntry&)> cb);

    void     init();   // load from store, call once at boot
    void     tick();   // check for firing alarms, call each loop()

private:
    Ringer&     _ringer;
    AlarmStore& _store;
    TimeFunc    _getTime;
    std::vector<AlarmEntry> _alarms;
    uint32_t    _nextId;
    uint32_t    _lastCheckedMinuteKey;
    std::function<void(const AlarmEntry&)> _onFire;

    void save();
};
