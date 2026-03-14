#include <unity.h>
#include "phonebook/UrlResolver.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// urlExtractHost
// ---------------------------------------------------------------------------

void test_extract_host_simple(void) {
    TEST_ASSERT_EQUAL_STRING("example.com",
        urlExtractHost("http://example.com/path").c_str());
}

void test_extract_host_with_port(void) {
    TEST_ASSERT_EQUAL_STRING("example.com",
        urlExtractHost("http://example.com:8080/path").c_str());
}

void test_extract_host_no_path(void) {
    TEST_ASSERT_EQUAL_STRING("example.com",
        urlExtractHost("http://example.com").c_str());
}

void test_extract_host_no_path_with_port(void) {
    TEST_ASSERT_EQUAL_STRING("example.com",
        urlExtractHost("http://example.com:80").c_str());
}

void test_extract_host_https(void) {
    TEST_ASSERT_EQUAL_STRING("example.com",
        urlExtractHost("https://example.com/path").c_str());
}

void test_extract_host_local(void) {
    TEST_ASSERT_EQUAL_STRING("wled.local",
        urlExtractHost("http://wled.local/json/state").c_str());
}

void test_extract_host_ip(void) {
    TEST_ASSERT_EQUAL_STRING("192.168.1.50",
        urlExtractHost("http://192.168.1.50/json/state").c_str());
}

void test_extract_host_malformed(void) {
    TEST_ASSERT_EQUAL_STRING("",
        urlExtractHost("not-a-url").c_str());
}

void test_extract_host_empty(void) {
    TEST_ASSERT_EQUAL_STRING("",
        urlExtractHost("").c_str());
}

void test_extract_host_scheme_only(void) {
    TEST_ASSERT_EQUAL_STRING("",
        urlExtractHost("http://").c_str());
}

// ---------------------------------------------------------------------------
// urlReplaceHost
// ---------------------------------------------------------------------------

void test_replace_host_simple(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50/path",
        urlReplaceHost("http://example.com/path", "192.168.1.50").c_str());
}

void test_replace_host_with_port(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50:8080/path",
        urlReplaceHost("http://example.com:8080/path", "192.168.1.50").c_str());
}

void test_replace_host_no_path(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50",
        urlReplaceHost("http://example.com", "192.168.1.50").c_str());
}

void test_replace_host_preserves_everything(void) {
    TEST_ASSERT_EQUAL_STRING("http://10.0.0.1:80/json/state?on=true",
        urlReplaceHost("http://wled.local:80/json/state?on=true", "10.0.0.1").c_str());
}

void test_replace_host_malformed_returns_original(void) {
    TEST_ASSERT_EQUAL_STRING("not-a-url",
        urlReplaceHost("not-a-url", "192.168.1.50").c_str());
}

// ---------------------------------------------------------------------------
// urlResolveLocal
// ---------------------------------------------------------------------------

static std::string mockLookup(const std::string& name) {
    if (name == "wled") return "192.168.1.50";
    if (name == "homeassistant") return "192.168.1.100";
    return "";
}

void test_resolve_local_simple(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50/json/state",
        urlResolveLocal("http://wled.local/json/state", mockLookup).c_str());
}

void test_resolve_local_with_port(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50:80/json/state",
        urlResolveLocal("http://wled.local:80/json/state", mockLookup).c_str());
}

void test_resolve_local_different_host(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.100:8123/api/services",
        urlResolveLocal("http://homeassistant.local:8123/api/services", mockLookup).c_str());
}

void test_resolve_non_local_unchanged(void) {
    std::string url = "http://example.com/path";
    TEST_ASSERT_EQUAL_STRING(url.c_str(),
        urlResolveLocal(url, mockLookup).c_str());
}

void test_resolve_ip_unchanged(void) {
    std::string url = "http://192.168.1.50/json/state";
    TEST_ASSERT_EQUAL_STRING(url.c_str(),
        urlResolveLocal(url, mockLookup).c_str());
}

void test_resolve_lookup_fails_returns_original(void) {
    std::string url = "http://unknown.local/path";
    TEST_ASSERT_EQUAL_STRING(url.c_str(),
        urlResolveLocal(url, mockLookup).c_str());
}

void test_resolve_malformed_returns_original(void) {
    std::string url = "not-a-url";
    TEST_ASSERT_EQUAL_STRING(url.c_str(),
        urlResolveLocal(url, mockLookup).c_str());
}

void test_resolve_no_path(void) {
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50",
        urlResolveLocal("http://wled.local", mockLookup).c_str());
}

void test_resolve_host_is_just_local(void) {
    // ".local" alone is not a valid mDNS name — should not resolve
    std::string url = "http://.local/path";
    TEST_ASSERT_EQUAL_STRING(url.c_str(),
        urlResolveLocal(url, mockLookup).c_str());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_host_simple);
    RUN_TEST(test_extract_host_with_port);
    RUN_TEST(test_extract_host_no_path);
    RUN_TEST(test_extract_host_no_path_with_port);
    RUN_TEST(test_extract_host_https);
    RUN_TEST(test_extract_host_local);
    RUN_TEST(test_extract_host_ip);
    RUN_TEST(test_extract_host_malformed);
    RUN_TEST(test_extract_host_empty);
    RUN_TEST(test_extract_host_scheme_only);

    RUN_TEST(test_replace_host_simple);
    RUN_TEST(test_replace_host_with_port);
    RUN_TEST(test_replace_host_no_path);
    RUN_TEST(test_replace_host_preserves_everything);
    RUN_TEST(test_replace_host_malformed_returns_original);

    RUN_TEST(test_resolve_local_simple);
    RUN_TEST(test_resolve_local_with_port);
    RUN_TEST(test_resolve_local_different_host);
    RUN_TEST(test_resolve_non_local_unchanged);
    RUN_TEST(test_resolve_ip_unchanged);
    RUN_TEST(test_resolve_lookup_fails_returns_original);
    RUN_TEST(test_resolve_malformed_returns_original);
    RUN_TEST(test_resolve_no_path);
    RUN_TEST(test_resolve_host_is_just_local);

    return UNITY_END();
}
