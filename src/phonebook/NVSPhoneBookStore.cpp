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
        e.id              = obj["id"]     | (uint32_t)0;
        e.number          = (const char*)(obj["number"] | "");
        e.name            = (const char*)(obj["name"]   | "");
        e.type            = (const char*)(obj["type"]   | "");
        e.url             = (const char*)(obj["url"]    | "");
        e.method          = (const char*)(obj["method"] | "GET");
        e.body            = (const char*)(obj["body"]   | "");
        e.builtinFunction = (const char*)(obj["builtinFunction"] | "");
        e.pattern         = (const char*)(obj["pattern"] | "");
        e.cycles          = obj["cycles"] | 0;
        e.callbackDelay   = obj["callbackDelay"] | 0;

        JsonArray hdrs = obj["headers"];
        if (hdrs) {
            for (JsonObject h : hdrs) {
                PhoneBookHeader hdr;
                hdr.name  = (const char*)(h["name"]  | "");
                hdr.value = (const char*)(h["value"] | "");
                e.headers.push_back(hdr);
            }
        }

        JsonArray exts = obj["extensions"];
        if (exts) {
            for (JsonObject x : exts) {
                PhoneBookExtension ext;
                ext.ext             = (const char*)(x["ext"]    | "");
                ext.name            = (const char*)(x["name"]   | "");
                ext.type            = (const char*)(x["type"]   | "");
                ext.path            = (const char*)(x["path"]   | "");
                ext.method          = (const char*)(x["method"] | "");
                ext.body            = (const char*)(x["body"]   | "");
                ext.builtinFunction = (const char*)(x["builtinFunction"] | "");
                ext.pattern         = (const char*)(x["pattern"] | "");
                ext.cycles          = x["cycles"] | 0;
                ext.callbackDelay   = x["callbackDelay"] | 0;
                e.extensions.push_back(ext);
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
        if (!e.type.empty()) obj["type"] = e.type.c_str();
        obj["url"]    = e.url.c_str();
        obj["method"] = e.method.c_str();
        obj["body"]   = e.body.c_str();

        if (!e.builtinFunction.empty()) obj["builtinFunction"] = e.builtinFunction.c_str();
        if (!e.pattern.empty())         obj["pattern"]         = e.pattern.c_str();
        if (e.cycles)                   obj["cycles"]          = e.cycles;
        if (e.callbackDelay)            obj["callbackDelay"]   = e.callbackDelay;

        JsonArray hdrs = obj["headers"].to<JsonArray>();
        for (const auto& h : e.headers) {
            JsonObject ho = hdrs.add<JsonObject>();
            ho["name"]  = h.name.c_str();
            ho["value"] = h.value.c_str();
        }

        if (!e.extensions.empty()) {
            JsonArray exts = obj["extensions"].to<JsonArray>();
            for (const auto& x : e.extensions) {
                JsonObject xo = exts.add<JsonObject>();
                xo["ext"]  = x.ext.c_str();
                xo["name"] = x.name.c_str();
                if (!x.type.empty()) xo["type"] = x.type.c_str();
                xo["path"] = x.path.c_str();
                if (!x.method.empty()) xo["method"] = x.method.c_str();
                if (!x.body.empty())   xo["body"]   = x.body.c_str();
                if (!x.builtinFunction.empty()) xo["builtinFunction"] = x.builtinFunction.c_str();
                if (!x.pattern.empty())         xo["pattern"]         = x.pattern.c_str();
                if (x.cycles)                   xo["cycles"]          = x.cycles;
                if (x.callbackDelay)            xo["callbackDelay"]   = x.callbackDelay;
            }
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
