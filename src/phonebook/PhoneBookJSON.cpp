#include "phonebook/PhoneBookJSON.h"
#include <string.h>

static const char* MASK_VALUE = "********";

// Case-insensitive substring search without heap allocation
static bool containsCI(const char* haystack, const char* needle) {
    size_t nlen = strlen(needle);
    for (const char* p = haystack; *p; p++) {
        if (strncasecmp(p, needle, nlen) == 0) return true;
    }
    return false;
}

static bool isSensitiveHeader(const char* name) {
    return containsCI(name, "authorization") || containsCI(name, "token");
}

void phoneBookFillJson(JsonObject obj, const PhoneBookEntry& e, bool mask) {
    obj["id"]     = e.id;
    obj["number"] = e.number.c_str();
    obj["name"]   = e.name.c_str();
    obj["url"]    = e.url.c_str();
    obj["method"] = e.method.c_str();
    obj["body"]   = e.body.c_str();

    JsonArray hdrs = obj["headers"].to<JsonArray>();
    for (const auto& h : e.headers) {
        JsonObject ho = hdrs.add<JsonObject>();
        ho["name"] = h.name.c_str();
        if (mask && isSensitiveHeader(h.name.c_str())) {
            ho["value"] = MASK_VALUE;
        } else {
            ho["value"] = h.value.c_str();
        }
    }

    JsonArray exts = obj["extensions"].to<JsonArray>();
    for (const auto& x : e.extensions) {
        JsonObject xo = exts.add<JsonObject>();
        xo["ext"]    = x.ext.c_str();
        xo["name"]   = x.name.c_str();
        xo["path"]   = x.path.c_str();
        xo["method"] = x.method.c_str();
        xo["body"]   = x.body.c_str();
    }
}

void phoneBookParseJson(JsonObject obj, PhoneBookEntry& e, const PhoneBookEntry* mergeFrom) {
    e.number = (const char*)(obj["number"] | "");
    e.name   = (const char*)(obj["name"]   | "");
    e.url    = (const char*)(obj["url"]    | "");
    e.method = (const char*)(obj["method"] | "GET");
    e.body   = (const char*)(obj["body"]   | "");

    e.headers.clear();
    JsonArray hdrs = obj["headers"];
    if (hdrs) {
        for (JsonObject h : hdrs) {
            PhoneBookHeader hdr;
            hdr.name  = (const char*)(h["name"]  | "");
            hdr.value = (const char*)(h["value"] | "");

            // If the value is the mask placeholder and we have an existing entry,
            // preserve the original value for that header name.
            if (mergeFrom && hdr.value == MASK_VALUE) {
                for (const auto& old : mergeFrom->headers) {
                    if (old.name == hdr.name) {
                        hdr.value = old.value;
                        break;
                    }
                }
            }
            e.headers.push_back(hdr);
        }
    }

    e.extensions.clear();
    JsonArray exts = obj["extensions"];
    if (exts) {
        for (JsonObject x : exts) {
            PhoneBookExtension ext;
            ext.ext    = (const char*)(x["ext"]    | "");
            ext.name   = (const char*)(x["name"]   | "");
            ext.path   = (const char*)(x["path"]   | "");
            ext.method = (const char*)(x["method"] | "");
            ext.body   = (const char*)(x["body"]   | "");
            e.extensions.push_back(ext);
        }
    }
}
