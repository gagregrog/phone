#include "hardware/DialReader.h"
#include "hardware/pins.h"
#include <Arduino.h>

static const uint32_t DIAL_DEBOUNCE_MS  = 20;   // off-normal contact debounce
static const uint32_t PULSE_DEBOUNCE_MS = 5;    // pulse contact debounce
static const uint32_t SETTLE_MS         = 150;  // wait after dial returns before firing

void DialReader::begin() {
    pinMode(PIN_DIAL_ACTIVE, INPUT_PULLUP);
    pinMode(PIN_DIAL_PULSE,  INPUT_PULLUP);
}

void DialReader::tick() {
    uint32_t now = millis();

    // Debounce dial-active (active LOW: LOW = dialing)
    bool rawActive = (digitalRead(PIN_DIAL_ACTIVE) == LOW);
    if (rawActive != _rawDialActive) {
        _rawDialActive = rawActive;
        _dialDebounceAt = now + DIAL_DEBOUNCE_MS;
    }
    if (now >= _dialDebounceAt) {
        _dialActive = _rawDialActive;
    }

    // Debounce pulse pin (active LOW: LOW = pulse contact closed)
    // Capture previous debounced value before updating so we can detect edges below.
    bool prevPulse = _pulse;
    bool rawPulse = (digitalRead(PIN_DIAL_PULSE) == LOW);
    if (rawPulse != _rawPulse) {
        _rawPulse = rawPulse;
        _pulseDebounceAt = now + PULSE_DEBOUNCE_MS;
    }
    if (now >= _pulseDebounceAt) {
        _pulse = _rawPulse;
    }

    switch (_state) {
        case IDLE:
            if (_dialActive) {
                _state = DIALING;
                _pulseCount = 0;
            }
            break;

        case DIALING:
            // Count each LOW-going edge (HIGH→LOW = pulse contact closes)
            if (_pulse && !prevPulse) {
                _pulseCount++;
            }
            if (!_dialActive) {
                _state = DONE;
                _settleStart = now;
            }
            break;

        case DONE:
            if (now - _settleStart >= SETTLE_MS) {
                if (_onDigit && _pulseCount > 0) {
                    _onDigit(_pulseCount == 10 ? 0 : _pulseCount);
                }
                _state = IDLE;
            }
            break;
    }
}
