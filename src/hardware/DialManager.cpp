#include "hardware/DialManager.h"

DialManager::DialManager(DialReader& dial, HandsetMonitor& handset)
    : _dial(dial), _handset(handset) {}

void DialManager::begin() {
    _offHook = _handset.isOffHook();

    _dial.setOnDialStart([this]() {
        if (!_offHook) return;
        for (auto& cb : _onDialStart) cb();
    });

    _dial.setOnDigit([this](int digit) {
        if (!_offHook) return;
        _number += (char)('0' + digit);
        for (auto& cb : _onDigit) cb(digit, _number.c_str());
    });
}

void DialManager::tick() {
    bool current = _handset.isOffHook();
    if (current == _offHook) return;

    _offHook = current;
    _number.clear();

    if (!_offHook) {
        // Went on-hook (hang up): notify listeners
        for (auto& cb : _onClear) cb();
    }
    // Went off-hook (pick up): number cleared silently
}
