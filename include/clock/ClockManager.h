#pragma once
#include "ringer/Ringer.h"
#include <functional>
#include <time.h>

enum ChimeMode { CHIME_SINGLE, CHIME_N_CHIMES };

class ClockManager {
public:
    ClockManager(Ringer& ringer, std::function<bool(struct tm*)> getTime);
    void setOnChime(std::function<void(uint16_t rings)> cb);

    void tick();
    bool isEnabled() const;
    void setEnabled(bool enabled);
    ChimeMode getChimeMode() const;
    void setChimeMode(ChimeMode mode);

private:
    Ringer& _ringer;
    std::function<bool(struct tm*)> _getTime;
    std::function<void(uint16_t)> _onChime;
    bool _enabled;
    ChimeMode _chimeMode;
    uint32_t _lastCheckedMinuteKey;
};
