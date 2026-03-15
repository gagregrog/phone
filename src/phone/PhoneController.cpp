#include "phone/PhoneController.h"
#include "ringer/RingerEvents.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <string.h>

static void publishPhoneState(const char* state, const char* number = nullptr) {
    char buf[64];
    if (number && number[0] != '\0') {
        snprintf(buf, sizeof(buf), "{\"state\":\"%s\",\"number\":\"%s\"}", state, number);
    } else {
        snprintf(buf, sizeof(buf), "{\"state\":\"%s\"}", state);
    }
    eventsPublish("phone/state", buf);
}

static const char* stateName(PhoneState s) {
    switch (s) {
        case PhoneState::IDLE:                return "IDLE";
        case PhoneState::RINGING:             return "RINGING";
        case PhoneState::OFF_HOOK:            return "OFF_HOOK";
        case PhoneState::DIALING:             return "DIALING";
        case PhoneState::CALL_OUT:            return "CALL_OUT";
        case PhoneState::IN_CALL:             return "IN_CALL";
        case PhoneState::AWAITING_EXTENSION:  return "AWAITING_EXTENSION";
        case PhoneState::WRONG_NUMBER:        return "WRONG_NUMBER";
        default:                              return "UNKNOWN";
    }
}

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
            if (_state == PhoneState::IDLE) {
                _state = PhoneState::RINGING;
                publishPhoneState("RINGING");
            }
        } else if (strcmp(topic, "ring/stopped") == 0) {
            if (_state == PhoneState::RINGING) {
                _state = PhoneState::IDLE;
                publishPhoneState("IDLE");
            }
        }
    });

    _handset.addOnChange([this](bool offHook) { onHandsetChanged(offHook); });
    _dial.addOnDialStart([this]() { onDialActivity(); });
    _dial.addOnDigit([this](int /*digit*/, const char* /*number*/) { onDialActivity(); });
}

void PhoneController::tick(unsigned long now) {
    if (_state == PhoneState::DIALING) {
        if (_dialActivitySinceLastTick) {
            _lastDialActivityMs = now;
            _dialActivitySinceLastTick = false;
            return;
        }
        if (now - _lastDialActivityMs >= DIAL_TIMEOUT_MS) {
            const char* number = _dial.number();
            logger.phonef("Dial complete: %s", number);
            _state = PhoneState::CALL_OUT;
            publishPhoneState("CALL_OUT", number);
            if (_onDialComplete) _onDialComplete(number);
        }
    } else if (_state == PhoneState::AWAITING_EXTENSION) {
        if (_dialActivitySinceLastTick) {
            _lastDialActivityMs = now;
            _dialActivitySinceLastTick = false;
            return;
        }
        if (now - _lastDialActivityMs >= EXT_TIMEOUT_MS) {
            const char* ext = _dial.number();
            logger.phonef("Extension timeout: %s", ext[0] ? ext : "(empty)");
            if (_onExtDialComplete) _onExtDialComplete(ext);
        }
    }
}

RingResult PhoneController::ring(const RingPattern& pattern, uint16_t cycles) {
    if (_state != PhoneState::IDLE) {
        logger.phonef("Ring rejected [%s]: %s", stateName(_state), pattern.name);
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"blockedBy\":\"%s\",\"pattern\":\"%s\"}", stateName(_state), pattern.name);
        eventsPublish("phone/ring-rejected", buf);
        return RingResult::BUSY;
    }
    // Set state before ring() so that when _onStart fires ring/started,
    // the event subscriber above sees _state != IDLE and skips.
    _state = PhoneState::RINGING;
    _ringer.ring(pattern, cycles);
    publishPhoneState("RINGING");
    return RingResult::STARTED;
}

void PhoneController::ringStop() {
    if (_state != PhoneState::RINGING) return;
    // Set state before publishing so the ring/stopped subscriber skips.
    _state = PhoneState::IDLE;
    logger.phone("Ring stopped");
    publishPhoneState("IDLE");
    _ringer.ringStop();
    publishRingStopped();
}

void PhoneController::callAnswered() {
    if (_state != PhoneState::CALL_OUT) return;
    logger.phone("Call answered");
    _state = PhoneState::IN_CALL;
    publishPhoneState("IN_CALL");
}

void PhoneController::awaitExtension() {
    if (_state != PhoneState::CALL_OUT) return;
    logger.phone("Awaiting extension");
    _dial.clearNumber();
    _state = PhoneState::AWAITING_EXTENSION;
    _dialActivitySinceLastTick = true;  // start the timeout from now
    publishPhoneState("AWAITING_EXTENSION");
}

void PhoneController::wrongNumber() {
    if (_state != PhoneState::CALL_OUT && _state != PhoneState::AWAITING_EXTENSION) return;
    logger.phone("Wrong number");
    _state = PhoneState::WRONG_NUMBER;
    publishPhoneState("WRONG_NUMBER");
}

void PhoneController::onHandsetChanged(bool offHook) {
    if (offHook) {
        switch (_state) {
            case PhoneState::IDLE:
                logger.phone("Handset off hook");
                _state = PhoneState::OFF_HOOK;
                publishPhoneState("OFF_HOOK");
                if (_onOffHook) _onOffHook();
                break;
            case PhoneState::RINGING:
                logger.phone("Call answered");
                // Set state before publishRingStopped so the ring/stopped
                // subscriber sees IN_CALL and skips back to IDLE.
                _state = PhoneState::IN_CALL;
                publishPhoneState("IN_CALL");
                _ringer.ringStop();
                publishRingStopped();
                if (_onAnswered) _onAnswered();
                break;
            default:
                break;
        }
    } else {
        // Handset returned — always reset to IDLE
        bool wasActive = (_state == PhoneState::IN_CALL              ||
                          _state == PhoneState::CALL_OUT             ||
                          _state == PhoneState::OFF_HOOK             ||
                          _state == PhoneState::DIALING              ||
                          _state == PhoneState::AWAITING_EXTENSION   ||
                          _state == PhoneState::WRONG_NUMBER);
        if (wasActive) logger.phonef("Handset on hook [was %s]", stateName(_state));
        _state = PhoneState::IDLE;
        publishPhoneState("IDLE");
        if (wasActive && _onHungUp) _onHungUp();
    }
}

void PhoneController::onDialActivity() {
    if (_state == PhoneState::OFF_HOOK) {
        logger.phone("Dialing started");
        _state = PhoneState::DIALING;
        publishPhoneState("DIALING");
        _dialActivitySinceLastTick = true;
    } else if (_state == PhoneState::DIALING || _state == PhoneState::AWAITING_EXTENSION) {
        _dialActivitySinceLastTick = true;
    }
}
