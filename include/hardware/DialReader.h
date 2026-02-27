#pragma once
#include <functional>
#include <stdint.h>

class DialReader {
public:
    using DigitCallback     = std::function<void(int)>;
    using DialStartCallback = std::function<void()>;

    void begin();
    void tick();
    void setOnDigit(DigitCallback cb)         { _onDigit     = cb; }
    void setOnDialStart(DialStartCallback cb) { _onDialStart = cb; }
    bool isDialing() const { return _state == DIALING; }

private:
    enum State { IDLE, DIALING, DONE };

    DigitCallback     _onDigit;
    DialStartCallback _onDialStart;
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
