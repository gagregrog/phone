#include <unity.h>
#include "phonebook/WledIntegration.h"

void setUp(void) {}
void tearDown(void) {}

static const PhoneBookExtension* findExt(const PhoneBookEntry& e, const char* ext) {
    for (const auto& x : e.extensions) {
        if (x.ext == ext) return &x;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// url / method / body / headers
// ---------------------------------------------------------------------------

void test_apply_defaults_sets_url(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    wledApplyDefaults(e);
    TEST_ASSERT_EQUAL_STRING("http://books.local/json/state", e.url.c_str());
}

void test_apply_defaults_sets_method_post(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    wledApplyDefaults(e);
    TEST_ASSERT_EQUAL_STRING("POST", e.method.c_str());
}

void test_apply_defaults_clears_body_and_headers(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    e.body = "leftover";
    PhoneBookHeader h;
    h.name = "X-Old";
    h.value = "1";
    e.headers.push_back(h);
    wledApplyDefaults(e);
    TEST_ASSERT_TRUE(e.body.empty());
    TEST_ASSERT_EQUAL(0, (int)e.headers.size());
}

// ---------------------------------------------------------------------------
// extensions
// ---------------------------------------------------------------------------

void test_apply_defaults_generates_six_extensions(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    wledApplyDefaults(e);
    TEST_ASSERT_EQUAL(6, (int)e.extensions.size());
}

void test_apply_defaults_on_off_bodies(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    wledApplyDefaults(e);

    const PhoneBookExtension* on = findExt(e, "9");
    TEST_ASSERT_NOT_NULL(on);
    TEST_ASSERT_EQUAL_STRING("On", on->name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"on\":true}", on->body.c_str());
    TEST_ASSERT_TRUE(on->path.empty());
    TEST_ASSERT_TRUE(on->method.empty());

    const PhoneBookExtension* off = findExt(e, "0");
    TEST_ASSERT_NOT_NULL(off);
    TEST_ASSERT_EQUAL_STRING("Off", off->name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"on\":false}", off->body.c_str());
}

void test_apply_defaults_preset_bodies(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    wledApplyDefaults(e);

    const char* presetExts[] = {"1", "2", "3", "4"};
    const char* presetNames[] = {"M1", "M2", "M3", "M4"};
    const char* presetBodies[] = {"{\"ps\":1}", "{\"ps\":2}", "{\"ps\":3}", "{\"ps\":4}"};

    for (int i = 0; i < 4; i++) {
        const PhoneBookExtension* x = findExt(e, presetExts[i]);
        TEST_ASSERT_NOT_NULL(x);
        TEST_ASSERT_EQUAL_STRING(presetNames[i], x->name.c_str());
        TEST_ASSERT_EQUAL_STRING(presetBodies[i], x->body.c_str());
    }
}

void test_apply_defaults_replaces_existing_extensions(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    PhoneBookExtension stale;
    stale.ext = "7";
    stale.name = "Stale";
    e.extensions.push_back(stale);

    wledApplyDefaults(e);

    TEST_ASSERT_EQUAL(6, (int)e.extensions.size());
    TEST_ASSERT_NULL(findExt(e, "7"));
}

void test_apply_defaults_regenerates_url_on_host_change(void) {
    PhoneBookEntry e;
    e.wledHost = "books";
    wledApplyDefaults(e);
    TEST_ASSERT_EQUAL_STRING("http://books.local/json/state", e.url.c_str());

    e.wledHost = "kitchen";
    wledApplyDefaults(e);
    TEST_ASSERT_EQUAL_STRING("http://kitchen.local/json/state", e.url.c_str());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_apply_defaults_sets_url);
    RUN_TEST(test_apply_defaults_sets_method_post);
    RUN_TEST(test_apply_defaults_clears_body_and_headers);

    RUN_TEST(test_apply_defaults_generates_six_extensions);
    RUN_TEST(test_apply_defaults_on_off_bodies);
    RUN_TEST(test_apply_defaults_preset_bodies);
    RUN_TEST(test_apply_defaults_replaces_existing_extensions);
    RUN_TEST(test_apply_defaults_regenerates_url_on_host_change);

    return UNITY_END();
}
