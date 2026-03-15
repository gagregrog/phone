#pragma once
#include "hardware/DialReader.h"
#include "hardware/HandsetMonitor.h"
#include <functional>
#include <string>
#include <vector>

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
    void clearNumber()         { _number.clear(); }

    void addOnDialStart(DialStartCallback cb) { _onDialStart.push_back(cb); }
    void addOnDigit(DigitCallback cb)         { _onDigit.push_back(cb); }
    void addOnClear(ClearCallback cb)         { _onClear.push_back(cb); }

private:
    DialReader&     _dial;
    HandsetMonitor& _handset;
    bool            _offHook = false;
    std::string     _number;

    std::vector<DialStartCallback> _onDialStart;
    std::vector<DigitCallback>     _onDigit;
    std::vector<ClearCallback>     _onClear;
};
