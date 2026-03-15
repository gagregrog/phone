#include <unity.h>
#include "phone/PhoneController.h"
#include "web/Events.h"
#include "hardware/pins.h"

extern unsigned long _mock_millis;
extern uint8_t _mock_pin_values[40];

// --- Fixtures ---

static MotorDriver   motor(0, 0, 0);
static Ringer*       ringer;
static HandsetMonitor* handset;
static DialReader*   dialReader;
static DialManager*  dialMgr;
static PhoneController* phone;

// Simulate handset going off-hook (pass debounce)
static void goOffHook() {
    _mock_pin_values[PIN_HANDSET] = LOW;   // activeLow: LOW = pressed = off-hook
    handset->update();                     // records change time
    _mock_millis += 55;
    handset->update();                     // debounce passes, fires callbacks
}

// Simulate handset returning to cradle
static void goOnHook() {
    _mock_pin_values[PIN_HANDSET] = HIGH;
    handset->update();
    _mock_millis += 55;
    handset->update();
}

// Simulate rotary dial starting to turn (pass DialReader debounce)
static void startDialing() {
    _mock_pin_values[PIN_DIAL_ACTIVE] = LOW;  // active-low: LOW = dialing
    dialReader->tick();                        // records debounce
    _mock_millis += 25;
    dialReader->tick();                        // fires onDialStart → DialManager → PhoneController
}

void setUp(void) {
    _mock_millis = 0;
    _mock_pin_values[PIN_HANDSET]     = HIGH;  // on-hook
    _mock_pin_values[PIN_DIAL_ACTIVE] = HIGH;  // not dialing
    _mock_pin_values[PIN_DIAL_PULSE]  = HIGH;

    eventsReset();

    ringer     = new Ringer(motor);
    handset    = new HandsetMonitor(PIN_HANDSET);
    dialReader = new DialReader();
    dialMgr    = new DialManager(*dialReader, *handset);
    phone      = new PhoneController(*ringer, *handset, *dialMgr);

    handset->begin();
    dialReader->begin();
    dialMgr->begin();
    phone->begin();
}

void tearDown(void) {
    delete phone;
    delete dialMgr;
    delete dialReader;
    delete handset;
    delete ringer;
}

// --- Initial state ---

void test_initial_state_is_idle(void) {
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
    TEST_ASSERT_FALSE(phone->isRinging());
}

// --- Inbound ring ---

void test_ring_in_idle_returns_started(void) {
    RingResult r = phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(RingResult::STARTED, r);
    TEST_ASSERT_EQUAL(PhoneState::RINGING, phone->getState());
    TEST_ASSERT_TRUE(phone->isRinging());
}

void test_ring_in_ringing_is_busy(void) {
    phone->ring(PATTERN_US);
    RingResult r = phone->ring(PATTERN_UK);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
    TEST_ASSERT_EQUAL(PhoneState::RINGING, phone->getState());
}

void test_ring_stop_returns_to_idle(void) {
    phone->ring(PATTERN_US);
    phone->ringStop();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
    TEST_ASSERT_FALSE(phone->isRinging());
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_ring_stop_when_not_ringing_is_noop(void) {
    phone->ringStop();  // should not crash or change state
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

// --- Handset up while idle → OFF_HOOK ---

void test_handset_up_idle_goes_off_hook(void) {
    goOffHook();
    TEST_ASSERT_EQUAL(PhoneState::OFF_HOOK, phone->getState());
}

void test_off_hook_callback_fires(void) {
    bool fired = false;
    phone->setOnOffHook([&fired]{ fired = true; });
    goOffHook();
    TEST_ASSERT_TRUE(fired);
}

void test_ring_in_off_hook_is_busy(void) {
    goOffHook();
    RingResult r = phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
    TEST_ASSERT_EQUAL(PhoneState::OFF_HOOK, phone->getState());
}

void test_handset_down_from_off_hook_to_idle(void) {
    goOffHook();
    goOnHook();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

// --- Handset up while ringing → IN_CALL ---

void test_answer_ringing_goes_to_in_call(void) {
    phone->ring(PATTERN_US);
    goOffHook();
    TEST_ASSERT_EQUAL(PhoneState::IN_CALL, phone->getState());
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_on_answered_fires_when_handset_picked_up(void) {
    bool answered = false;
    phone->setOnAnswered([&answered]{ answered = true; });
    phone->ring(PATTERN_US);
    goOffHook();
    TEST_ASSERT_TRUE(answered);
}

void test_on_answered_not_fired_when_picking_up_idle(void) {
    bool answered = false;
    phone->setOnAnswered([&answered]{ answered = true; });
    goOffHook();
    TEST_ASSERT_FALSE(answered);
}

void test_ring_in_in_call_is_busy(void) {
    phone->ring(PATTERN_US);
    goOffHook();
    RingResult r = phone->ring(PATTERN_UK);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
    TEST_ASSERT_EQUAL(PhoneState::IN_CALL, phone->getState());
}

// --- Hang up from IN_CALL → IDLE ---

void test_hang_up_from_in_call_to_idle(void) {
    phone->ring(PATTERN_US);
    goOffHook();
    goOnHook();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_on_hung_up_fires_on_hang_up(void) {
    bool hungUp = false;
    phone->setOnHungUp([&hungUp]{ hungUp = true; });
    phone->ring(PATTERN_US);
    goOffHook();
    goOnHook();
    TEST_ASSERT_TRUE(hungUp);
}

void test_on_hung_up_not_fired_when_on_hook_while_idle(void) {
    bool hungUp = false;
    phone->setOnHungUp([&hungUp]{ hungUp = true; });
    goOnHook();  // already on hook
    TEST_ASSERT_FALSE(hungUp);
}

// --- External ring/stopped event (natural ring completion) → IDLE ---

void test_external_ring_stopped_event_clears_ringing_state(void) {
    phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(PhoneState::RINGING, phone->getState());
    eventsPublish("ring/stopped", "{}");
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_external_ring_started_event_sets_ringing_from_idle(void) {
    // Simulates an alarm/timer starting the ring directly via Ringer,
    // bypassing PhoneController::ring().
    eventsPublish("ring/started", "{\"ringing\":true,\"pattern\":\"us\"}");
    TEST_ASSERT_EQUAL(PhoneState::RINGING, phone->getState());
}

// --- Dialing state ---

void test_dial_start_off_hook_goes_to_dialing(void) {
    goOffHook();
    dialMgr->tick();   // update dialMgr _offHook state
    startDialing();
    TEST_ASSERT_EQUAL(PhoneState::DIALING, phone->getState());
}

void test_ring_in_dialing_is_busy(void) {
    goOffHook();
    dialMgr->tick();
    startDialing();
    RingResult r = phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
}

void test_dial_timeout_transitions_to_call_out(void) {
    goOffHook();
    dialMgr->tick();
    startDialing();
    TEST_ASSERT_EQUAL(PhoneState::DIALING, phone->getState());

    // Advance past dial timeout (3000ms) without further activity
    _mock_millis += 1;
    phone->tick(_mock_millis);   // captures _lastDialActivityMs
    _mock_millis += PhoneController::DIAL_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    TEST_ASSERT_EQUAL(PhoneState::CALL_OUT, phone->getState());
}

void test_on_dial_complete_fires_with_number(void) {
    const char* dialedNumber = nullptr;
    static char numberBuf[16];
    phone->setOnDialComplete([&dialedNumber](const char* n) {
        strncpy(numberBuf, n, sizeof(numberBuf) - 1);
        dialedNumber = numberBuf;
    });

    goOffHook();
    dialMgr->tick();
    startDialing();
    _mock_millis += 1;
    phone->tick(_mock_millis);
    _mock_millis += PhoneController::DIAL_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    TEST_ASSERT_NOT_NULL(dialedNumber);
}

void test_ring_in_call_out_is_busy(void) {
    goOffHook();
    dialMgr->tick();
    startDialing();
    _mock_millis += 1;
    phone->tick(_mock_millis);
    _mock_millis += PhoneController::DIAL_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    RingResult r = phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
    TEST_ASSERT_EQUAL(PhoneState::CALL_OUT, phone->getState());
}

// --- callAnswered ---

void test_call_answered_transitions_call_out_to_in_call(void) {
    goOffHook();
    dialMgr->tick();
    startDialing();
    _mock_millis += 1;
    phone->tick(_mock_millis);
    _mock_millis += PhoneController::DIAL_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    phone->callAnswered();
    TEST_ASSERT_EQUAL(PhoneState::IN_CALL, phone->getState());
}

void test_call_answered_no_op_when_not_in_call_out(void) {
    phone->callAnswered();  // should not crash or change state
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

// --- phone/state events ---

void test_phone_state_published_on_ring(void) {
    char captured[128] = {};
    eventsSubscribe([&captured](const char* t, const char* p) {
        if (strcmp(t, "phone/state") == 0)
            strncpy(captured, p, sizeof(captured) - 1);
    });
    phone->ring(PATTERN_US);
    TEST_ASSERT_NOT_NULL(strstr(captured, "RINGING"));
}

void test_phone_state_published_on_ring_stop(void) {
    char captured[128] = {};
    eventsSubscribe([&captured](const char* t, const char* p) {
        if (strcmp(t, "phone/state") == 0)
            strncpy(captured, p, sizeof(captured) - 1);
    });
    phone->ring(PATTERN_US);
    phone->ringStop();
    TEST_ASSERT_NOT_NULL(strstr(captured, "IDLE"));
}

void test_phone_state_published_on_off_hook(void) {
    char captured[128] = {};
    eventsSubscribe([&captured](const char* t, const char* p) {
        if (strcmp(t, "phone/state") == 0)
            strncpy(captured, p, sizeof(captured) - 1);
    });
    goOffHook();
    TEST_ASSERT_NOT_NULL(strstr(captured, "OFF_HOOK"));
}

void test_phone_state_published_on_answer(void) {
    char captured[128] = {};
    eventsSubscribe([&captured](const char* t, const char* p) {
        if (strcmp(t, "phone/state") == 0)
            strncpy(captured, p, sizeof(captured) - 1);
    });
    phone->ring(PATTERN_US);
    goOffHook();
    TEST_ASSERT_NOT_NULL(strstr(captured, "IN_CALL"));
}

void test_phone_state_published_on_hang_up(void) {
    char captured[128] = {};
    eventsSubscribe([&captured](const char* t, const char* p) {
        if (strcmp(t, "phone/state") == 0)
            strncpy(captured, p, sizeof(captured) - 1);
    });
    phone->ring(PATTERN_US);
    goOffHook();
    goOnHook();
    TEST_ASSERT_NOT_NULL(strstr(captured, "IDLE"));
}

void test_phone_ring_rejected_event_when_busy(void) {
    char captured[128] = {};
    eventsSubscribe([&captured](const char* t, const char* p) {
        if (strcmp(t, "phone/ring-rejected") == 0)
            strncpy(captured, p, sizeof(captured) - 1);
    });
    phone->ring(PATTERN_US);
    phone->ring(PATTERN_UK);
    TEST_ASSERT_NOT_NULL(strstr(captured, "RINGING"));
    TEST_ASSERT_NOT_NULL(strstr(captured, "uk"));
}

// --- Hang up clears all active states ---

void test_hang_up_from_dialing_to_idle(void) {
    goOffHook();
    dialMgr->tick();
    startDialing();
    goOnHook();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_hang_up_from_call_out_to_idle(void) {
    goOffHook();
    dialMgr->tick();
    startDialing();
    _mock_millis += 1;
    phone->tick(_mock_millis);
    _mock_millis += PhoneController::DIAL_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    goOnHook();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

// --- Helper to reach CALL_OUT state ---
static void goToCallOut() {
    goOffHook();
    dialMgr->tick();
    startDialing();
    _mock_millis += 1;
    phone->tick(_mock_millis);
    _mock_millis += PhoneController::DIAL_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);
}

// --- AWAITING_EXTENSION ---

void test_await_extension_transitions_from_call_out(void) {
    goToCallOut();
    phone->awaitExtension();
    TEST_ASSERT_EQUAL(PhoneState::AWAITING_EXTENSION, phone->getState());
}

void test_await_extension_noop_from_idle(void) {
    phone->awaitExtension();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_dial_activity_in_awaiting_extension_resets_timeout(void) {
    goToCallOut();
    phone->awaitExtension();

    // Simulate time passing and dial activity
    _mock_millis += 1;
    phone->tick(_mock_millis);  // captures _lastDialActivityMs

    // Start dialing while in AWAITING_EXTENSION
    startDialing();
    _mock_millis += 4000;  // less than EXT_TIMEOUT from original, but > from activity
    phone->tick(_mock_millis);  // should record activity, not fire timeout

    // Should still be AWAITING_EXTENSION because activity reset the timer
    TEST_ASSERT_EQUAL(PhoneState::AWAITING_EXTENSION, phone->getState());
}

void test_extension_timeout_fires_callback(void) {
    goToCallOut();
    phone->awaitExtension();

    bool fired = false;
    static char capturedExt[16] = {};
    phone->setOnExtensionDialComplete([&fired](const char* ext) {
        fired = true;
        strncpy(capturedExt, ext, sizeof(capturedExt) - 1);
    });

    _mock_millis += 1;
    phone->tick(_mock_millis);  // captures _lastDialActivityMs
    _mock_millis += PhoneController::EXT_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    TEST_ASSERT_TRUE(fired);
}

void test_extension_timeout_empty_ext(void) {
    goToCallOut();

    static char capturedExt[16];
    memset(capturedExt, 'X', sizeof(capturedExt));
    phone->setOnExtensionDialComplete([](const char* ext) {
        strncpy(capturedExt, ext, sizeof(capturedExt) - 1);
        capturedExt[sizeof(capturedExt) - 1] = '\0';
    });

    phone->awaitExtension();

    _mock_millis += 1;
    phone->tick(_mock_millis);
    _mock_millis += PhoneController::EXT_TIMEOUT_MS + 1;
    phone->tick(_mock_millis);

    TEST_ASSERT_EQUAL_STRING("", capturedExt);
}

// --- WRONG_NUMBER ---

void test_wrong_number_from_call_out(void) {
    goToCallOut();
    phone->wrongNumber();
    TEST_ASSERT_EQUAL(PhoneState::WRONG_NUMBER, phone->getState());
}

void test_wrong_number_from_awaiting_extension(void) {
    goToCallOut();
    phone->awaitExtension();
    phone->wrongNumber();
    TEST_ASSERT_EQUAL(PhoneState::WRONG_NUMBER, phone->getState());
}

void test_wrong_number_noop_from_idle(void) {
    phone->wrongNumber();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_hang_up_from_awaiting_extension_to_idle(void) {
    goToCallOut();
    phone->awaitExtension();
    goOnHook();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_hang_up_from_wrong_number_to_idle(void) {
    goToCallOut();
    phone->wrongNumber();
    goOnHook();
    TEST_ASSERT_EQUAL(PhoneState::IDLE, phone->getState());
}

void test_ring_in_awaiting_extension_is_busy(void) {
    goToCallOut();
    phone->awaitExtension();
    RingResult r = phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
}

void test_ring_in_wrong_number_is_busy(void) {
    goToCallOut();
    phone->wrongNumber();
    RingResult r = phone->ring(PATTERN_US);
    TEST_ASSERT_EQUAL(RingResult::BUSY, r);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_is_idle);

    RUN_TEST(test_ring_in_idle_returns_started);
    RUN_TEST(test_ring_in_ringing_is_busy);
    RUN_TEST(test_ring_stop_returns_to_idle);
    RUN_TEST(test_ring_stop_when_not_ringing_is_noop);

    RUN_TEST(test_handset_up_idle_goes_off_hook);
    RUN_TEST(test_off_hook_callback_fires);
    RUN_TEST(test_ring_in_off_hook_is_busy);
    RUN_TEST(test_handset_down_from_off_hook_to_idle);

    RUN_TEST(test_answer_ringing_goes_to_in_call);
    RUN_TEST(test_on_answered_fires_when_handset_picked_up);
    RUN_TEST(test_on_answered_not_fired_when_picking_up_idle);
    RUN_TEST(test_ring_in_in_call_is_busy);

    RUN_TEST(test_hang_up_from_in_call_to_idle);
    RUN_TEST(test_on_hung_up_fires_on_hang_up);
    RUN_TEST(test_on_hung_up_not_fired_when_on_hook_while_idle);

    RUN_TEST(test_external_ring_stopped_event_clears_ringing_state);
    RUN_TEST(test_external_ring_started_event_sets_ringing_from_idle);

    RUN_TEST(test_dial_start_off_hook_goes_to_dialing);
    RUN_TEST(test_ring_in_dialing_is_busy);
    RUN_TEST(test_dial_timeout_transitions_to_call_out);
    RUN_TEST(test_on_dial_complete_fires_with_number);
    RUN_TEST(test_ring_in_call_out_is_busy);

    RUN_TEST(test_call_answered_transitions_call_out_to_in_call);
    RUN_TEST(test_call_answered_no_op_when_not_in_call_out);

    RUN_TEST(test_hang_up_from_dialing_to_idle);
    RUN_TEST(test_hang_up_from_call_out_to_idle);

    RUN_TEST(test_await_extension_transitions_from_call_out);
    RUN_TEST(test_await_extension_noop_from_idle);
    RUN_TEST(test_dial_activity_in_awaiting_extension_resets_timeout);
    RUN_TEST(test_extension_timeout_fires_callback);
    RUN_TEST(test_extension_timeout_empty_ext);
    RUN_TEST(test_wrong_number_from_call_out);
    RUN_TEST(test_wrong_number_from_awaiting_extension);
    RUN_TEST(test_wrong_number_noop_from_idle);
    RUN_TEST(test_hang_up_from_awaiting_extension_to_idle);
    RUN_TEST(test_hang_up_from_wrong_number_to_idle);
    RUN_TEST(test_ring_in_awaiting_extension_is_busy);
    RUN_TEST(test_ring_in_wrong_number_is_busy);

    RUN_TEST(test_phone_state_published_on_ring);
    RUN_TEST(test_phone_state_published_on_ring_stop);
    RUN_TEST(test_phone_state_published_on_off_hook);
    RUN_TEST(test_phone_state_published_on_answer);
    RUN_TEST(test_phone_state_published_on_hang_up);
    RUN_TEST(test_phone_ring_rejected_event_when_busy);

    return UNITY_END();
}
