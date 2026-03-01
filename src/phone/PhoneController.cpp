#include "phone/PhoneController.h"
#include "ringer/RingerEvents.h"
#include "web/Events.h"
#include <string.h>

PhoneController::PhoneController(Ringer& ringer, HandsetMonitor& handset, DialManager& dial)
    : _ringer(ringer), _handset(handset), _dial(dial),
      _state(PhoneState::IDLE),
      _lastDialActivityMs(0),
      _dialActivitySinceLastTick(false) {}

void PhoneController::begin() {
    // Track when any source (alarm, timer, clock, API) starts or stops the ringer.
    // State is set BEFORE publishRingStopped() in our own methods, so the
    // "ring/stopped" subscriber below only acts when an external stop occurred
    // (e.g., natural cycle completion via _onStop).
    eventsSubscribe([this](const char* topic, const char*) {
        if (strcmp(topic, "ring/started") == 0) {
            if (_state == PhoneState::IDLE) _state = PhoneState::RINGING;
        } else if (strcmp(topic, "ring/stopped") == 0) {
            if (_state == PhoneState::RINGING) _state = PhoneState::IDLE;
        }
    });

    _handset.addOnChange([this](bool offHook) { onHandsetChanged(offHook); });
    _dial.addOnDialStart([this]() { onDialActivity(); });
    _dial.addOnDigit([this](int /*digit*/, const char* /*number*/) { onDialActivity(); });
}

void PhoneController::tick(unsigned long now) {
    if (_state != PhoneState::DIALING) return;

    if (_dialActivitySinceLastTick) {
        _lastDialActivityMs = now;
        _dialActivitySinceLastTick = false;
        return;
    }

    if (now - _lastDialActivityMs >= DIAL_TIMEOUT_MS) {
        const char* number = _dial.number();
        _state = PhoneState::CALL_OUT;
        if (_onDialComplete) _onDialComplete(number);
    }
}

RingResult PhoneController::ring(const RingPattern& pattern, uint16_t cycles) {
    if (_state != PhoneState::IDLE) return RingResult::BUSY;
    // Set state before ring() so that when _onStart fires ring/started,
    // the event subscriber above sees _state != IDLE and skips.
    _state = PhoneState::RINGING;
    _ringer.ring(pattern, cycles);
    return RingResult::STARTED;
}

void PhoneController::ringStop() {
    if (_state != PhoneState::RINGING) return;
    // Set state before publishing so the ring/stopped subscriber skips.
    _state = PhoneState::IDLE;
    _ringer.ringStop();
    publishRingStopped();
}

void PhoneController::callAnswered() {
    if (_state != PhoneState::CALL_OUT) return;
    _state = PhoneState::IN_CALL;
}

void PhoneController::onHandsetChanged(bool offHook) {
    if (offHook) {
        switch (_state) {
            case PhoneState::IDLE:
                _state = PhoneState::OFF_HOOK;
                if (_onOffHook) _onOffHook();
                break;
            case PhoneState::RINGING:
                // Set state before publishRingStopped so the ring/stopped
                // subscriber sees IN_CALL and skips back to IDLE.
                _state = PhoneState::IN_CALL;
                _ringer.ringStop();
                publishRingStopped();
                if (_onAnswered) _onAnswered();
                break;
            default:
                break;
        }
    } else {
        // Handset returned — always reset to IDLE
        bool wasActive = (_state == PhoneState::IN_CALL   ||
                          _state == PhoneState::CALL_OUT  ||
                          _state == PhoneState::OFF_HOOK  ||
                          _state == PhoneState::DIALING);
        _state = PhoneState::IDLE;
        if (wasActive && _onHungUp) _onHungUp();
    }
}

void PhoneController::onDialActivity() {
    if (_state == PhoneState::OFF_HOOK || _state == PhoneState::DIALING) {
        _state = PhoneState::DIALING;
        _dialActivitySinceLastTick = true;
    }
}
