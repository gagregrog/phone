#pragma once
#include <functional>
#include <stdint.h>

class DialReader {
public:
    using DigitCallback = std::function<void(int)>;

    void begin();
    void tick();
    void setOnDigit(DigitCallback cb) { _onDigit = cb; }

private:
    enum State { IDLE, DIALING, DONE };

    DigitCallback _onDigit;
    State    _state           = IDLE;
    int      _pulseCount      = 0;
    uint32_t _settleStart     = 0;

    bool     _rawDialActive   = false;
    bool     _dialActive      = false;
    uint32_t _dialDebounceAt  = 0;

    bool     _rawPulse        = false;
    bool     _pulse           = false;
    uint32_t _pulseDebounceAt = 0;
};
