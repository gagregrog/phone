#include "phonebook/NVSPhoneBookStore.h"
#include "system/Logger.h"
#include <Preferences.h>
#include <ArduinoJson.h>

static const char* NS  = "phonebook";
static const char* KEY = "entries";

void NVSPhoneBookStore::load(std::vector<PhoneBookEntry>& entries) {
    Preferences prefs;
    prefs.begin(NS, false);  // read-write: auto-creates namespace on first boot
    if (!prefs.isKey(KEY)) {
        prefs.end();
        return;
    }
    String json = prefs.getString(KEY, "");
    prefs.end();

    if (json.isEmpty()) return;

    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        logger.warn("NVSPhoneBookStore: failed to parse JSON");
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        PhoneBookEntry e;
        e.id     = obj["id"]     | (uint32_t)0;
        e.number = (const char*)(obj["number"] | "");
        e.name   = (const char*)(obj["name"]   | "");
        e.url    = (const char*)(obj["url"]    | "");
        e.method = (const char*)(obj["method"] | "GET");
        e.body   = (const char*)(obj["body"]   | "");

        JsonArray hdrs = obj["headers"];
        if (hdrs) {
            for (JsonObject h : hdrs) {
                PhoneBookHeader hdr;
                hdr.name  = (const char*)(h["name"]  | "");
                hdr.value = (const char*)(h["value"] | "");
                e.headers.push_back(hdr);
            }
        }
        entries.push_back(e);
    }
    logger.infof("NVSPhoneBookStore: loaded %u entry(ies)", (unsigned)entries.size());
}

void NVSPhoneBookStore::save(const std::vector<PhoneBookEntry>& entries) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& e : entries) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]     = e.id;
        obj["number"] = e.number.c_str();
        obj["name"]   = e.name.c_str();
        obj["url"]    = e.url.c_str();
        obj["method"] = e.method.c_str();
        obj["body"]   = e.body.c_str();

        JsonArray hdrs = obj["headers"].to<JsonArray>();
        for (const auto& h : e.headers) {
            JsonObject ho = hdrs.add<JsonObject>();
            ho["name"]  = h.name.c_str();
            ho["value"] = h.value.c_str();
        }
    }

    String json;
    serializeJson(doc, json);

    if (json.length() > 3500) {
        logger.warnf("NVSPhoneBookStore: serialized size %u approaching NVS limit", (unsigned)json.length());
    }

    Preferences prefs;
    prefs.begin(NS, false);
    prefs.putString(KEY, json);
    prefs.end();

    logger.infof("NVSPhoneBookStore: saved %u entry(ies)", (unsigned)entries.size());
}
