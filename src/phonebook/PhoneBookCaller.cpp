#include "phonebook/PhoneBookCaller.h"
#include "phonebook/PhoneBookJSON.h"
#include "phone/PhoneController.h"
#include "ringer/Ringer.h"
#include "ringer/RingPattern.h"
#include "web/Events.h"
#include "system/Logger.h"
#include "phonebook/UrlResolver.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

static const int HTTP_TIMEOUT_MS = 3000;
static uint32_t _pendingEntryId = 0;

static std::string mdnsLookup(const std::string& name) {
    IPAddress ip = MDNS.queryHost(name.c_str());
    if (ip == IPAddress()) {
        logger.warnf("PhoneBookCaller: mDNS lookup failed for '%s.local'", name.c_str());
        return "";
    }
    return std::string(ip.toString().c_str());
}

int phoneBookCallerExec(const PhoneBookEntry& entry) {
    std::string resolvedUrl = urlResolveLocal(entry.url, mdnsLookup);

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.begin(String(resolvedUrl.c_str()));

    // Set headers
    for (const auto& h : entry.headers) {
        http.addHeader(h.name.c_str(), h.value.c_str());
    }

    // If there's a body and no Content-Type header, default to application/json
    if (!entry.body.empty()) {
        bool hasContentType = false;
        for (const auto& h : entry.headers) {
            String lower(h.name.c_str());
            lower.toLowerCase();
            if (lower == "content-type") { hasContentType = true; break; }
        }
        if (!hasContentType) {
            http.addHeader("Content-Type", "application/json");
        }
    }

    int httpCode;
    if (entry.method == "POST") {
        httpCode = http.POST(entry.body.c_str());
    } else if (entry.method == "PUT") {
        httpCode = http.PUT(entry.body.c_str());
    } else if (entry.method == "DELETE") {
        httpCode = http.sendRequest("DELETE", entry.body.c_str());
    } else {
        httpCode = http.GET();
    }

    // Build event payload
    JsonDocument doc;
    doc["id"]       = entry.id;
    doc["number"]   = entry.number.c_str();
    doc["name"]     = entry.name.c_str();
    doc["httpCode"] = httpCode;

    if (httpCode > 0) {
        String response = http.getString();
        if (response.length() > 256) response = response.substring(0, 256);
        doc["response"] = response;
        logger.infof("PhoneBookCaller: %s %s -> %d", entry.method.c_str(), entry.url.c_str(), httpCode);

        String payload;
        serializeJson(doc, payload);
        eventsPublish("phonebook/called", payload.c_str());
    } else {
        doc["error"] = http.errorToString(httpCode);
        logger.warnf("PhoneBookCaller: %s %s -> error %d", entry.method.c_str(), entry.url.c_str(), httpCode);

        String payload;
        serializeJson(doc, payload);
        eventsPublish("phonebook/error", payload.c_str());
    }

    http.end();
    return httpCode;
}

void phoneBookCallerBegin(PhoneBookManager& mgr, PhoneController& phoneCtrl, Ringer& ringer) {
    phoneCtrl.setOnDialComplete([&mgr](const char* number) {
        mgr.dial(number);
    });

    mgr.setOnCall([&phoneCtrl](const PhoneBookEntry& entry) {
        phoneBookCallerExec(entry);
        phoneCtrl.callCompleted();
    });

    mgr.setOnNotFound([&phoneCtrl](const char* number) {
        logger.infof("PhoneBookCaller: no entry for number '%s'", number);
        phoneCtrl.wrongNumber();
        JsonDocument doc;
        doc["number"] = number;
        String payload;
        serializeJson(doc, payload);
        eventsPublish("phonebook/not-found", payload.c_str());
    });

    mgr.setOnCallWithExtensions([&phoneCtrl, &ringer](const PhoneBookEntry& entry) {
        _pendingEntryId = entry.id;
        ringer.ring(PATTERN_PIP, 1, true);
        phoneCtrl.awaitExtension();
    });

    phoneCtrl.setOnExtensionDialComplete([&mgr, &phoneCtrl](const char* ext) {
        if (!ext || ext[0] == '\0') {
            phoneCtrl.wrongNumber();
        } else if (mgr.dialExtension(_pendingEntryId, ext)) {
            phoneCtrl.callCompleted();
        }
    });

    mgr.setOnExtensionNotFound([&phoneCtrl](uint32_t entryId, const char* ext) {
        phoneCtrl.wrongNumber();
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"entryId\":%u,\"ext\":\"%s\"}", (unsigned)entryId, ext);
        eventsPublish("phonebook/extension-not-found", buf);
    });
}
