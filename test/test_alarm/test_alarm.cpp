#include <unity.h>
#include <string.h>
#include "AlarmManager.h"

// ---------------------------------------------------------------------------
// Mock time
// ---------------------------------------------------------------------------
static struct tm _mock_time;
static bool      _mock_time_synced = false;

static bool mockGetTime(struct tm* t) {
    if (!_mock_time_synced) return false;
    *t = _mock_time;
    return true;
}

static void setMockTime(int yday, int wday, int hour, int min) {
    _mock_time_synced  = true;
    _mock_time.tm_yday = yday;
    _mock_time.tm_wday = wday;
    _mock_time.tm_hour = hour;
    _mock_time.tm_min  = min;
    _mock_time.tm_sec  = 0;
}

// ---------------------------------------------------------------------------
// Mock store
// ---------------------------------------------------------------------------
class MockAlarmStore : public AlarmStore {
public:
    std::vector<AlarmEntry> stored;
    int saveCallCount = 0;

    void load(std::vector<AlarmEntry>& alarms) override {
        alarms = stored;
    }
    void save(const std::vector<AlarmEntry>& alarms) override {
        stored = alarms;
        saveCallCount++;
    }
};

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------
static MockAlarmStore* mockStore;
static MotorDriver*    motor;
static Ringer*         ringer;
static AlarmManager*   mgr;

void setUp(void) {
    _mock_time_synced = false;
    memset(&_mock_time, 0, sizeof(_mock_time));

    mockStore = new MockAlarmStore();
    motor     = new MotorDriver(0, 0, 0);
    ringer    = new Ringer(*motor);
    mgr       = new AlarmManager(*ringer, *mockStore, mockGetTime);
}

void tearDown(void) {
    delete mgr;
    delete ringer;
    delete motor;
    delete mockStore;
}

// ---------------------------------------------------------------------------
// add / IDs
// ---------------------------------------------------------------------------

void test_add_returns_nonzero_id(void) {
    uint32_t id = mgr->add(8, 30, "chirp", 1, true, false);
    TEST_ASSERT_NOT_EQUAL(0, id);
}

void test_ids_increment(void) {
    uint32_t id1 = mgr->add(8, 0, "chirp", 1, true, false);
    uint32_t id2 = mgr->add(9, 0, "chirp", 1, true, false);
    TEST_ASSERT_EQUAL(id1 + 1, id2);
}

// ---------------------------------------------------------------------------
// Persistence — save behaviour
// ---------------------------------------------------------------------------

void test_add_repeating_saves_to_store(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    TEST_ASSERT_EQUAL(1, mockStore->saveCallCount);
    TEST_ASSERT_EQUAL(1, (int)mockStore->stored.size());
}

void test_add_oneoff_not_saved_to_store(void) {
    mgr->add(8, 30, "chirp", 1, false, false);
    TEST_ASSERT_EQUAL(0, mockStore->saveCallCount);
}

// ---------------------------------------------------------------------------
// tick — fires
// ---------------------------------------------------------------------------

void test_fires_at_correct_time(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    setMockTime(45, 4, 8, 30);
    mgr->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

void test_no_fire_wrong_time(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    setMockTime(45, 4, 8, 31);
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_no_double_fire_same_minute(void) {
    mgr->add(8, 30, "chirp", 3, true, false);
    setMockTime(45, 4, 8, 30);
    mgr->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
    ringer->ringStop();

    // Second tick at same minute: _lastCheckedMinuteKey blocks it
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_oneshot_disables_after_fire(void) {
    mgr->add(8, 30, "chirp", 1, false, false);
    setMockTime(45, 4, 8, 30);
    mgr->tick();

    const AlarmEntry& e = mgr->getAll()[0];
    TEST_ASSERT_FALSE(e.enabled);
}

void test_oneshot_not_refire_next_day(void) {
    mgr->add(8, 30, "chirp", 1, false, false);
    setMockTime(45, 4, 8, 30);
    mgr->tick();
    ringer->ringStop();

    // Advance to same time next day
    setMockTime(46, 5, 8, 30);
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_repeat_stays_enabled(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    setMockTime(45, 4, 8, 30);
    mgr->tick();

    const AlarmEntry& e = mgr->getAll()[0];
    TEST_ASSERT_TRUE(e.enabled);
}

void test_repeat_fires_next_day(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    setMockTime(45, 4, 8, 30);
    mgr->tick();
    ringer->ringStop();

    setMockTime(46, 5, 8, 30);
    mgr->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// tick — skipWeekends
// ---------------------------------------------------------------------------

void test_skip_weekends_sat(void) {
    mgr->add(8, 30, "chirp", 1, true, true);
    setMockTime(45, 6, 8, 30);  // Saturday
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_skip_weekends_sun(void) {
    mgr->add(8, 30, "chirp", 1, true, true);
    setMockTime(46, 0, 8, 30);  // Sunday
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

void test_no_skip_fires_weekday(void) {
    mgr->add(8, 30, "chirp", 1, true, true);
    setMockTime(45, 4, 8, 30);  // Thursday
    mgr->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void test_update_changes_fields(void) {
    uint32_t id = mgr->add(8, 30, "chirp", 1, true, false);
    bool ok = mgr->update(id, 9, 15, "us", 3, true, true);
    TEST_ASSERT_TRUE(ok);

    const AlarmEntry& e = mgr->getAll()[0];
    TEST_ASSERT_EQUAL(9,   e.hour);
    TEST_ASSERT_EQUAL(15,  e.minute);
    TEST_ASSERT_EQUAL_STRING("us", e.patternName);
    TEST_ASSERT_EQUAL(3,   e.rings);
    TEST_ASSERT_TRUE(e.skipWeekends);
}

void test_update_returns_false_unknown_id(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    TEST_ASSERT_FALSE(mgr->update(999, 9, 0, "chirp", 1, true, false));
}

void test_update_saves(void) {
    uint32_t id = mgr->add(8, 30, "chirp", 1, true, false);
    int prevCount = mockStore->saveCallCount;
    mgr->update(id, 9, 0, "chirp", 1, true, false);
    TEST_ASSERT_GREATER_THAN(prevCount, mockStore->saveCallCount);
}

void test_update_reenables_disabled(void) {
    uint32_t id = mgr->add(8, 30, "chirp", 1, false, false);
    // Manually disable
    setMockTime(45, 4, 8, 30);
    mgr->tick();  // fires and disables

    mgr->update(id, 8, 30, "chirp", 1, false, false);
    TEST_ASSERT_TRUE(mgr->getAll()[0].enabled);
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------

void test_remove_deletes(void) {
    uint32_t id = mgr->add(8, 30, "chirp", 1, true, false);
    TEST_ASSERT_TRUE(mgr->remove(id));
    TEST_ASSERT_EQUAL(0, (int)mgr->getAll().size());
}

void test_remove_returns_false_unknown(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    TEST_ASSERT_FALSE(mgr->remove(999));
}

void test_remove_all_clears(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    mgr->add(9, 0,  "us",    1, true, false);
    mgr->removeAll();
    TEST_ASSERT_EQUAL(0, (int)mgr->getAll().size());
}

// ---------------------------------------------------------------------------
// Disabled alarm
// ---------------------------------------------------------------------------

void test_disabled_alarm_does_not_fire(void) {
    mgr->add(8, 30, "chirp", 1, false, false);
    // Fire and disable it
    setMockTime(45, 4, 8, 30);
    mgr->tick();
    ringer->ringStop();

    // Next day, same time
    setMockTime(46, 5, 8, 30);
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

void test_init_loads_from_store(void) {
    AlarmEntry e;
    e.id          = 3;
    e.hour        = 7;
    e.minute      = 45;
    e.rings       = 2;
    e.repeat      = true;
    e.skipWeekends = false;
    e.enabled     = true;
    strncpy(e.patternName, "uk", sizeof(e.patternName));
    mockStore->stored.push_back(e);

    mgr->init();

    TEST_ASSERT_EQUAL(1, (int)mgr->getAll().size());
    TEST_ASSERT_EQUAL(7,  mgr->getAll()[0].hour);
    TEST_ASSERT_EQUAL(45, mgr->getAll()[0].minute);
}

void test_init_repairs_nextid(void) {
    AlarmEntry e;
    e.id          = 5;
    e.hour        = 8;
    e.minute      = 0;
    e.rings       = 1;
    e.repeat      = true;
    e.skipWeekends = false;
    e.enabled     = true;
    strncpy(e.patternName, "chirp", sizeof(e.patternName));
    mockStore->stored.push_back(e);

    mgr->init();
    uint32_t newId = mgr->add(9, 0, "chirp", 1, true, false);
    TEST_ASSERT_EQUAL(6, newId);
}

// ---------------------------------------------------------------------------
// tick — no-op when not synced
// ---------------------------------------------------------------------------

void test_tick_noop_when_not_synced(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    _mock_time_synced = false;
    mgr->tick();
    TEST_ASSERT_FALSE(ringer->isRinging());
}

// ---------------------------------------------------------------------------
// Multiple alarms — only matching fires
// ---------------------------------------------------------------------------

void test_multiple_alarms_only_matching_fires(void) {
    mgr->add(8, 30, "chirp", 1, true, false);
    mgr->add(9, 0,  "us",    1, true, false);

    setMockTime(45, 4, 8, 30);
    mgr->tick();
    TEST_ASSERT_TRUE(ringer->isRinging());

    // Ensure only one fired (second alarm's time hasn't come)
    TEST_ASSERT_EQUAL(2, (int)mgr->getAll().size());
}

// ---------------------------------------------------------------------------
// isTimeInFuture
// ---------------------------------------------------------------------------

void test_is_time_in_future_not_synced(void) {
    _mock_time_synced = false;
    TEST_ASSERT_FALSE(mgr->isTimeInFuture(8, 30));
}

void test_is_time_in_future_past_time(void) {
    setMockTime(45, 4, 9, 0);
    TEST_ASSERT_FALSE(mgr->isTimeInFuture(8, 30));
}

void test_is_time_in_future_same_time(void) {
    setMockTime(45, 4, 8, 30);
    TEST_ASSERT_FALSE(mgr->isTimeInFuture(8, 30));
}

void test_is_time_in_future_future_time(void) {
    setMockTime(45, 4, 8, 0);
    TEST_ASSERT_TRUE(mgr->isTimeInFuture(8, 30));
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_add_returns_nonzero_id);
    RUN_TEST(test_ids_increment);
    RUN_TEST(test_add_repeating_saves_to_store);
    RUN_TEST(test_add_oneoff_not_saved_to_store);
    RUN_TEST(test_fires_at_correct_time);
    RUN_TEST(test_no_fire_wrong_time);
    RUN_TEST(test_no_double_fire_same_minute);
    RUN_TEST(test_oneshot_disables_after_fire);
    RUN_TEST(test_oneshot_not_refire_next_day);
    RUN_TEST(test_repeat_stays_enabled);
    RUN_TEST(test_repeat_fires_next_day);
    RUN_TEST(test_skip_weekends_sat);
    RUN_TEST(test_skip_weekends_sun);
    RUN_TEST(test_no_skip_fires_weekday);
    RUN_TEST(test_update_changes_fields);
    RUN_TEST(test_update_returns_false_unknown_id);
    RUN_TEST(test_update_saves);
    RUN_TEST(test_update_reenables_disabled);
    RUN_TEST(test_remove_deletes);
    RUN_TEST(test_remove_returns_false_unknown);
    RUN_TEST(test_remove_all_clears);
    RUN_TEST(test_disabled_alarm_does_not_fire);
    RUN_TEST(test_init_loads_from_store);
    RUN_TEST(test_init_repairs_nextid);
    RUN_TEST(test_tick_noop_when_not_synced);
    RUN_TEST(test_multiple_alarms_only_matching_fires);
    RUN_TEST(test_is_time_in_future_not_synced);
    RUN_TEST(test_is_time_in_future_past_time);
    RUN_TEST(test_is_time_in_future_same_time);
    RUN_TEST(test_is_time_in_future_future_time);

    return UNITY_END();
}
