#pragma once
#include <functional>
#include <stdint.h>
#include "ringer/Ringer.h"
#include "ringer/RingPattern.h"
#include "hardware/HandsetMonitor.h"
#include "hardware/DialManager.h"

enum class PhoneState {
    IDLE,       // On hook, ready for incoming calls
    RINGING,    // Incoming call, bell ringing, on hook
    OFF_HOOK,   // Handset lifted, no active call (dial tone would play)
    DIALING,    // Digits being accumulated; timeout fires onDialComplete
    CALL_OUT,   // Number dialed, awaiting answer (ring-back tone would play)
    IN_CALL,    // Connected — answered an incoming ring
};

enum class RingResult {
    STARTED,
    BUSY,
};

class PhoneController {
public:
    PhoneController(Ringer& ringer, HandsetMonitor& handset, DialManager& dial);

    // Wire callbacks from HandsetMonitor and DialManager in setup, then call begin()
    void begin();

    // Called from loop() — drives dial-complete timeout
    void tick(unsigned long now);

    // Request an incoming ring. Returns BUSY if handset is off-hook or already in a call.
    RingResult ring(const RingPattern& pattern, uint16_t cycles = 0);

    // Stop an in-progress incoming ring and return to IDLE.
    void ringStop();

    // Transition CALL_OUT → IN_CALL (called externally when outbound call is answered).
    void callAnswered();

    PhoneState getState() const { return _state; }
    bool isRinging() const      { return _state == PhoneState::RINGING; }

    void setOnAnswered(std::function<void()> cb)              { _onAnswered  = cb; }
    void setOnHungUp(std::function<void()> cb)                { _onHungUp    = cb; }
    void setOnOffHook(std::function<void()> cb)               { _onOffHook   = cb; }
    void setOnDialComplete(std::function<void(const char*)> cb) { _onDialComplete = cb; }

    static constexpr unsigned long DIAL_TIMEOUT_MS = 3000;

private:
    void onHandsetChanged(bool offHook);
    void onDialActivity();  // Called for both dialStart and each digit

    Ringer&         _ringer;
    HandsetMonitor& _handset;
    DialManager&    _dial;

    PhoneState    _state;
    unsigned long _lastDialActivityMs;
    bool          _dialActivitySinceLastTick;

    std::function<void()>             _onAnswered;
    std::function<void()>             _onHungUp;
    std::function<void()>             _onOffHook;
    std::function<void(const char*)>  _onDialComplete;
};
