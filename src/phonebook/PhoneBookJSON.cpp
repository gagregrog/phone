#include "phonebook/PhoneBookJSON.h"
#include <string.h>

static const char* MASK_VALUE = "********";

static bool isSensitiveHeader(const char* name) {
    // Case-insensitive check for "authorization" or "token" anywhere in name
    String lower(name);
    lower.toLowerCase();
    return lower.indexOf("authorization") >= 0 || lower.indexOf("token") >= 0;
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
}
