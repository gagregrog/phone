#pragma once
#include "hardware/DialReader.h"
#include "hardware/HandsetMonitor.h"
#include <functional>
#include <string>

class DialManager {
public:
    using DialStartCallback = std::function<void()>;
    using DigitCallback     = std::function<void(int digit, const char* number)>;
    using ClearCallback     = std::function<void()>;

    DialManager(DialReader& dial, HandsetMonitor& handset);

    void begin();   // sets DialReader callbacks; snapshots initial handset state
    void tick();    // polls handset; detects on/off-hook transitions

    const char* number() const { return _number.c_str(); }
    bool isOffHook() const     { return _offHook; }

    void setOnDialStart(DialStartCallback cb) { _onDialStart = cb; }
    void setOnDigit(DigitCallback cb)         { _onDigit     = cb; }
    void setOnClear(ClearCallback cb)         { _onClear     = cb; }

private:
    DialReader&     _dial;
    HandsetMonitor& _handset;
    bool            _offHook = false;
    std::string     _number;

    DialStartCallback _onDialStart;
    DigitCallback     _onDigit;
    ClearCallback     _onClear;
};
