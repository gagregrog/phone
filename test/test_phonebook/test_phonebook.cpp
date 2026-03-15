#include <unity.h>
#include <string.h>
#include "phonebook/PhoneBookManager.h"
#include "web/Events.h"

// ---------------------------------------------------------------------------
// Mock store
// ---------------------------------------------------------------------------
class MockPhoneBookStore : public PhoneBookStore {
public:
    std::vector<PhoneBookEntry> stored;
    int saveCallCount = 0;

    void load(std::vector<PhoneBookEntry>& entries) override {
        entries = stored;
    }
    void save(const std::vector<PhoneBookEntry>& entries) override {
        stored = entries;
        saveCallCount++;
    }
};

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------
static MockPhoneBookStore* mockStore;
static PhoneBookManager*   mgr;

void setUp(void) {
    mockStore = new MockPhoneBookStore();
    mgr       = new PhoneBookManager(*mockStore);
    eventsReset();
}

void tearDown(void) {
    delete mgr;
    delete mockStore;
}

static PhoneBookEntry makeEntry(const char* number, const char* name,
                                 const char* url, const char* method = "GET") {
    PhoneBookEntry e;
    e.id     = 0;
    e.number = number;
    e.name   = name;
    e.url    = url;
    e.method = method;
    return e;
}

// ---------------------------------------------------------------------------
// add / IDs
// ---------------------------------------------------------------------------

void test_add_returns_nonzero_id(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    uint32_t id = mgr->add(e);
    TEST_ASSERT_NOT_EQUAL(0, id);
}

void test_ids_increment(void) {
    PhoneBookEntry e1 = makeEntry("1", "A", "http://a.com");
    PhoneBookEntry e2 = makeEntry("2", "B", "http://b.com");
    uint32_t id1 = mgr->add(e1);
    uint32_t id2 = mgr->add(e2);
    TEST_ASSERT_EQUAL(id1 + 1, id2);
}

void test_add_saves_to_store(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    mgr->add(e);
    TEST_ASSERT_EQUAL(1, mockStore->saveCallCount);
    TEST_ASSERT_EQUAL(1, (int)mockStore->stored.size());
}

void test_add_defaults_method_to_get(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    e.method = "";
    mgr->add(e);
    TEST_ASSERT_EQUAL_STRING("GET", mgr->getAll()[0].method.c_str());
}

// ---------------------------------------------------------------------------
// findByNumber
// ---------------------------------------------------------------------------

void test_find_by_number_match(void) {
    PhoneBookEntry e = makeEntry("411", "Info", "http://info.com");
    mgr->add(e);
    const PhoneBookEntry* found = mgr->findByNumber("411");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Info", found->name.c_str());
}

void test_find_by_number_no_match(void) {
    PhoneBookEntry e = makeEntry("411", "Info", "http://info.com");
    mgr->add(e);
    const PhoneBookEntry* found = mgr->findByNumber("999");
    TEST_ASSERT_NULL(found);
}

void test_find_by_number_returns_first(void) {
    PhoneBookEntry e1 = makeEntry("411", "First", "http://first.com");
    PhoneBookEntry e2 = makeEntry("411", "Second", "http://second.com");
    mgr->add(e1);
    mgr->add(e2);
    const PhoneBookEntry* found = mgr->findByNumber("411");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("First", found->name.c_str());
}

// ---------------------------------------------------------------------------
// findById
// ---------------------------------------------------------------------------

void test_find_by_id(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    uint32_t id = mgr->add(e);
    const PhoneBookEntry* found = mgr->findById(id);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("Test", found->name.c_str());
}

void test_find_by_id_not_found(void) {
    const PhoneBookEntry* found = mgr->findById(999);
    TEST_ASSERT_NULL(found);
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void test_update_changes_fields(void) {
    PhoneBookEntry e = makeEntry("1", "Old", "http://old.com", "GET");
    uint32_t id = mgr->add(e);

    PhoneBookEntry upd = makeEntry("2", "New", "http://new.com", "POST");
    upd.body = "{\"on\":true}";
    bool ok = mgr->update(id, upd);
    TEST_ASSERT_TRUE(ok);

    const PhoneBookEntry& result = mgr->getAll()[0];
    TEST_ASSERT_EQUAL_STRING("2", result.number.c_str());
    TEST_ASSERT_EQUAL_STRING("New", result.name.c_str());
    TEST_ASSERT_EQUAL_STRING("http://new.com", result.url.c_str());
    TEST_ASSERT_EQUAL_STRING("POST", result.method.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"on\":true}", result.body.c_str());
}

void test_update_returns_false_unknown_id(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    TEST_ASSERT_FALSE(mgr->update(999, e));
}

void test_update_saves(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    uint32_t id = mgr->add(e);
    int prevCount = mockStore->saveCallCount;

    PhoneBookEntry upd = makeEntry("1", "Updated", "http://updated.com");
    mgr->update(id, upd);
    TEST_ASSERT_GREATER_THAN(prevCount, mockStore->saveCallCount);
}

void test_update_preserves_id(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    uint32_t id = mgr->add(e);

    PhoneBookEntry upd = makeEntry("2", "Updated", "http://updated.com");
    mgr->update(id, upd);
    TEST_ASSERT_EQUAL(id, mgr->getAll()[0].id);
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------

void test_remove_deletes(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    uint32_t id = mgr->add(e);
    TEST_ASSERT_TRUE(mgr->remove(id));
    TEST_ASSERT_EQUAL(0, (int)mgr->getAll().size());
}

void test_remove_returns_false_unknown(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com");
    mgr->add(e);
    TEST_ASSERT_FALSE(mgr->remove(999));
}

void test_remove_all_clears(void) {
    mgr->add(makeEntry("1", "A", "http://a.com"));
    mgr->add(makeEntry("2", "B", "http://b.com"));
    mgr->removeAll();
    TEST_ASSERT_EQUAL(0, (int)mgr->getAll().size());
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

void test_init_loads_from_store(void) {
    PhoneBookEntry e;
    e.id     = 3;
    e.number = "411";
    e.name   = "Info";
    e.url    = "http://info.com";
    e.method = "GET";
    mockStore->stored.push_back(e);

    mgr->init();

    TEST_ASSERT_EQUAL(1, (int)mgr->getAll().size());
    TEST_ASSERT_EQUAL_STRING("411", mgr->getAll()[0].number.c_str());
}

void test_init_repairs_nextid(void) {
    PhoneBookEntry e;
    e.id     = 5;
    e.number = "411";
    e.name   = "Info";
    e.url    = "http://info.com";
    e.method = "GET";
    mockStore->stored.push_back(e);

    mgr->init();
    uint32_t newId = mgr->add(makeEntry("1", "New", "http://new.com"));
    TEST_ASSERT_EQUAL(6, newId);
}

// ---------------------------------------------------------------------------
// dial — callbacks
// ---------------------------------------------------------------------------

void test_dial_fires_on_call(void) {
    mgr->add(makeEntry("411", "Info", "http://info.com"));

    bool called = false;
    std::string calledNumber;
    mgr->setOnCall([&](const PhoneBookEntry& e) {
        called = true;
        calledNumber = e.number;
    });

    mgr->dial("411");
    TEST_ASSERT_TRUE(called);
    TEST_ASSERT_EQUAL_STRING("411", calledNumber.c_str());
}

void test_dial_fires_not_found(void) {
    bool notFound = false;
    std::string dialedNumber;
    mgr->setOnNotFound([&](const char* number) {
        notFound = true;
        dialedNumber = number;
    });

    mgr->dial("999");
    TEST_ASSERT_TRUE(notFound);
    TEST_ASSERT_EQUAL_STRING("999", dialedNumber.c_str());
}

void test_dial_no_callback_no_crash(void) {
    // Should not crash when no callbacks are set
    mgr->add(makeEntry("411", "Info", "http://info.com"));
    mgr->dial("411");
    mgr->dial("999");
}

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

void test_entry_with_headers(void) {
    PhoneBookEntry e = makeEntry("1", "Test", "http://example.com", "POST");
    PhoneBookHeader h;
    h.name = "Authorization";
    h.value = "Bearer secret123";
    e.headers.push_back(h);
    mgr->add(e);

    const PhoneBookEntry& stored = mgr->getAll()[0];
    TEST_ASSERT_EQUAL(1, (int)stored.headers.size());
    TEST_ASSERT_EQUAL_STRING("Authorization", stored.headers[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Bearer secret123", stored.headers[0].value.c_str());
}

// ---------------------------------------------------------------------------
// extensions
// ---------------------------------------------------------------------------

static PhoneBookEntry makeEntryWithExtensions(const char* number, const char* name,
                                                const char* url) {
    PhoneBookEntry e = makeEntry(number, name, url, "POST");
    e.body = "{\"on\":true}";
    PhoneBookExtension x1;
    x1.ext = "1"; x1.name = "On"; x1.path = "/on";
    PhoneBookExtension x2;
    x2.ext = "2"; x2.name = "Off"; x2.path = "/off"; x2.method = "PUT"; x2.body = "{\"off\":true}";
    e.extensions.push_back(x1);
    e.extensions.push_back(x2);
    return e;
}

void test_dial_with_extensions_fires_on_call_with_extensions(void) {
    PhoneBookEntry e = makeEntryWithExtensions("411", "WLED", "http://wled.local");
    mgr->add(e);

    bool fired = false;
    mgr->setOnCallWithExtensions([&](const PhoneBookEntry&) {
        fired = true;
    });
    mgr->setOnCall([](const PhoneBookEntry&) {
        TEST_FAIL_MESSAGE("onCall should not fire for entries with extensions");
    });

    mgr->dial("411");
    TEST_ASSERT_TRUE(fired);
}

void test_dial_without_extensions_fires_on_call(void) {
    PhoneBookEntry e = makeEntry("411", "Info", "http://info.com");
    mgr->add(e);

    bool fired = false;
    mgr->setOnCall([&](const PhoneBookEntry&) {
        fired = true;
    });

    mgr->dial("411");
    TEST_ASSERT_TRUE(fired);
}

void test_dial_extension_found_fires_on_call_with_merged_entry(void) {
    PhoneBookEntry e = makeEntryWithExtensions("411", "WLED", "http://wled.local");
    uint32_t id = mgr->add(e);

    std::string calledUrl;
    std::string calledMethod;
    std::string calledBody;
    mgr->setOnCall([&](const PhoneBookEntry& entry) {
        calledUrl    = entry.url;
        calledMethod = entry.method;
        calledBody   = entry.body;
    });

    mgr->dialExtension(id, "1");
    TEST_ASSERT_EQUAL_STRING("http://wled.local/on", calledUrl.c_str());
    TEST_ASSERT_EQUAL_STRING("POST", calledMethod.c_str());   // fallback to base
    TEST_ASSERT_EQUAL_STRING("{\"on\":true}", calledBody.c_str()); // fallback to base
}

void test_dial_extension_method_override(void) {
    PhoneBookEntry e = makeEntryWithExtensions("411", "WLED", "http://wled.local");
    uint32_t id = mgr->add(e);

    std::string calledMethod;
    std::string calledBody;
    mgr->setOnCall([&](const PhoneBookEntry& entry) {
        calledMethod = entry.method;
        calledBody   = entry.body;
    });

    mgr->dialExtension(id, "2");
    TEST_ASSERT_EQUAL_STRING("PUT", calledMethod.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"off\":true}", calledBody.c_str());
}

void test_dial_extension_not_found_fires_callback(void) {
    PhoneBookEntry e = makeEntryWithExtensions("411", "WLED", "http://wled.local");
    uint32_t id = mgr->add(e);

    bool notFound = false;
    mgr->setOnExtensionNotFound([&](uint32_t, const char*) {
        notFound = true;
    });

    mgr->dialExtension(id, "99");
    TEST_ASSERT_TRUE(notFound);
}

void test_dial_extension_invalid_entry_id(void) {
    bool notFound = false;
    mgr->setOnExtensionNotFound([&](uint32_t, const char*) {
        notFound = true;
    });

    mgr->dialExtension(999, "1");
    TEST_ASSERT_TRUE(notFound);
}

void test_has_extensions(void) {
    PhoneBookEntry e1 = makeEntryWithExtensions("411", "WLED", "http://wled.local");
    PhoneBookEntry e2 = makeEntry("911", "Test", "http://test.com");
    uint32_t id1 = mgr->add(e1);
    uint32_t id2 = mgr->add(e2);
    TEST_ASSERT_TRUE(mgr->hasExtensions(id1));
    TEST_ASSERT_FALSE(mgr->hasExtensions(id2));
}

void test_update_copies_extensions(void) {
    PhoneBookEntry e = makeEntry("411", "WLED", "http://wled.local");
    uint32_t id = mgr->add(e);
    TEST_ASSERT_FALSE(mgr->hasExtensions(id));

    PhoneBookEntry upd = makeEntryWithExtensions("411", "WLED", "http://wled.local");
    mgr->update(id, upd);
    TEST_ASSERT_TRUE(mgr->hasExtensions(id));
    TEST_ASSERT_EQUAL(2, (int)mgr->findById(id)->extensions.size());
}

void test_url_merging_concatenation(void) {
    PhoneBookEntry e = makeEntry("411", "WLED", "http://wled.local/json");
    PhoneBookExtension x;
    x.ext = "1"; x.name = "State"; x.path = "/state";
    e.extensions.push_back(x);
    uint32_t id = mgr->add(e);

    std::string calledUrl;
    mgr->setOnCall([&](const PhoneBookEntry& entry) {
        calledUrl = entry.url;
    });

    mgr->dialExtension(id, "1");
    TEST_ASSERT_EQUAL_STRING("http://wled.local/json/state", calledUrl.c_str());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_add_returns_nonzero_id);
    RUN_TEST(test_ids_increment);
    RUN_TEST(test_add_saves_to_store);
    RUN_TEST(test_add_defaults_method_to_get);
    RUN_TEST(test_find_by_number_match);
    RUN_TEST(test_find_by_number_no_match);
    RUN_TEST(test_find_by_number_returns_first);
    RUN_TEST(test_find_by_id);
    RUN_TEST(test_find_by_id_not_found);
    RUN_TEST(test_update_changes_fields);
    RUN_TEST(test_update_returns_false_unknown_id);
    RUN_TEST(test_update_saves);
    RUN_TEST(test_update_preserves_id);
    RUN_TEST(test_remove_deletes);
    RUN_TEST(test_remove_returns_false_unknown);
    RUN_TEST(test_remove_all_clears);
    RUN_TEST(test_init_loads_from_store);
    RUN_TEST(test_init_repairs_nextid);
    RUN_TEST(test_dial_fires_on_call);
    RUN_TEST(test_dial_fires_not_found);
    RUN_TEST(test_dial_no_callback_no_crash);
    RUN_TEST(test_entry_with_headers);

    RUN_TEST(test_dial_with_extensions_fires_on_call_with_extensions);
    RUN_TEST(test_dial_without_extensions_fires_on_call);
    RUN_TEST(test_dial_extension_found_fires_on_call_with_merged_entry);
    RUN_TEST(test_dial_extension_method_override);
    RUN_TEST(test_dial_extension_not_found_fires_callback);
    RUN_TEST(test_dial_extension_invalid_entry_id);
    RUN_TEST(test_has_extensions);
    RUN_TEST(test_update_copies_extensions);
    RUN_TEST(test_url_merging_concatenation);

    return UNITY_END();
}
