#include "phonebook/PhoneBookBuiltins.h"
#include "phonebook/PhoneBookManager.h"
#include "phone/PhoneController.h"
#include "ringer/Ringer.h"
#include "ringer/RingPattern.h"
#include "web/Events.h"
#include "system/Logger.h"

static PhoneController* _phoneCtrl = nullptr;

static bool               _scheduled = false;   // builtin dialed, waiting for hang-up
static bool               _armed     = false;   // hung up, delay counting down
static unsigned long       _fireAt   = 0;
static const RingPattern*  _pattern  = nullptr;
static uint16_t            _cycles   = 0;

static void clearPending() {
    _scheduled = false;
    _armed     = false;
    _fireAt    = 0;
    _pattern   = nullptr;
    _cycles    = 0;
}

void phoneBookBuiltinsBegin(PhoneBookManager& mgr, PhoneController& phoneCtrl, Ringer& ringer) {
    _phoneCtrl = &phoneCtrl;

    mgr.setOnBuiltinCall([&phoneCtrl](const PhoneBookEntry& entry) {
        if (entry.builtinFunction == "ring_callback") {
            const RingPattern* pat = findPattern(entry.pattern.c_str());
            if (!pat) pat = &PATTERN_US;

            _pattern   = pat;
            _cycles    = entry.cycles;
            _scheduled = true;
            _armed     = false;
            _fireAt    = (unsigned long)entry.callbackDelay * 1000UL;

            phoneCtrl.callCompleted();

            logger.infof("Builtin: ring_callback scheduled (pattern=%s, cycles=%u, delay=%us)",
                         pat->name, (unsigned)entry.cycles, (unsigned)entry.callbackDelay);

            eventsPublish("phonebook/builtin", "{\"function\":\"ring_callback\"}");
        } else {
            logger.warnf("Builtin: unknown function '%s'", entry.builtinFunction.c_str());
            phoneCtrl.wrongNumber();
        }
    });

    phoneCtrl.setOnHungUp([&]() {
        if (_scheduled && !_armed) {
            _armed = true;
            _fireAt = millis() + _fireAt;
            logger.info("Builtin: hang-up detected, callback timer armed");
        }
    });
}

void phoneBookBuiltinsTick(unsigned long now) {
    if (!_armed) return;
    if (now - _fireAt > 0x7FFFFFFFUL) return;   // not yet (overflow-safe)

    const RingPattern* pat = _pattern;
    uint16_t cycles = _cycles;
    clearPending();

    RingResult result = _phoneCtrl->ring(*pat, cycles);
    if (result == RingResult::STARTED) {
        logger.infof("Builtin: callback ring fired (pattern=%s, cycles=%u)", pat->name, (unsigned)cycles);
    } else {
        logger.info("Builtin: callback ring blocked (phone busy)");
    }
}
