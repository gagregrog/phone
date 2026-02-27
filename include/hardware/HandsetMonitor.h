#pragma once
#include <functional>
#include "hardware/ButtonTrigger.h"

class HandsetMonitor {
public:
    using ChangeCallback = std::function<void(bool offHook)>;

    explicit HandsetMonitor(uint8_t pin);

    void begin();
    void update();
    bool isOffHook() const;
    void setOnChange(ChangeCallback cb) { _onChange = cb; }

private:
    ButtonTrigger  _btn;
    ChangeCallback _onChange;
    bool           _lastState;
};
