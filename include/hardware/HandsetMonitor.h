#pragma once
#include <functional>
#include <vector>
#include "hardware/ButtonTrigger.h"

class HandsetMonitor {
public:
    using ChangeCallback = std::function<void(bool offHook)>;

    explicit HandsetMonitor(uint8_t pin);

    void begin();
    void update();
    bool isOffHook() const;
    void addOnChange(ChangeCallback cb) { _callbacks.push_back(cb); }

private:
    ButtonTrigger               _btn;
    std::vector<ChangeCallback> _callbacks;
    bool                        _lastState;
};
