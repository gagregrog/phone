#include "hardware/HandsetMonitor.h"

HandsetMonitor::HandsetMonitor(uint8_t pin)
    : _btn(pin, /*activeLow=*/true, /*debounceMs=*/50),
      _lastState(false) {}

void HandsetMonitor::begin() {
    _btn.begin();
    _lastState = _btn.isPressed();
}

void HandsetMonitor::update() {
    _btn.update();
    bool current = _btn.isPressed();
    if (current != _lastState) {
        _lastState = current;
        if (_onChange) _onChange(current);
    }
}

bool HandsetMonitor::isOffHook() const {
    return _btn.isPressed();
}
