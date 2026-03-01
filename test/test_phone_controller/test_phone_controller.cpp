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

    return UNITY_END();
}
