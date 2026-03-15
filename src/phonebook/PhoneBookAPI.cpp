#include "phonebook/PhoneBookAPI.h"
#include "phonebook/PhoneBookJSON.h"
#include "phonebook/PhoneBookCaller.h"
#include "web/API.h"
#include "web/Events.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>

static PhoneBookManager* _pbMgr = nullptr;

static void sendJson(AsyncWebServerRequest* req, int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

void phoneBookAPIBegin(PhoneBookManager& mgr) {
    _pbMgr = &mgr;
    AsyncWebServer* server = apiGetServer();

    apiAddStatusContributor("phonebook", [](JsonDocument& doc, const char* key) {
        JsonArray arr = doc[key].to<JsonArray>();
        for (const auto& e : _pbMgr->getAll()) {
            phoneBookFillJson(arr.add<JsonObject>(), e, true);
        }
    });

    // GET /phonebook — list all entries (masked headers)
    server->on("/phonebook", HTTP_GET, [](AsyncWebServerRequest* req) {
        logger.apif("[%s] GET /phonebook", req->client()->remoteIP().toString().c_str());
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& e : _pbMgr->getAll()) {
            phoneBookFillJson(arr.add<JsonObject>(), e, true);
        }
        sendJson(req, 200, doc);
    });

    // DELETE /phonebook — delete all entries
    server->on("/phonebook", HTTP_DELETE, [](AsyncWebServerRequest* req) {
        logger.apif("[%s] DELETE /phonebook", req->client()->remoteIP().toString().c_str());
        _pbMgr->removeAll();
        JsonDocument doc;
        doc["status"] = "cleared";
        sendJson(req, 200, doc);
        eventsPublish("phonebook/cleared", "{}");
    });

    // POST /phonebook — create entry
    // ESPAsyncWebServer prefix-matches, so /phonebook/test/* also hits this handler.
    // We check the exact path and handle test routes inline.
    server->on("/phonebook", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            // POST /phonebook/test/<id> or /phonebook/test/<id>/<ext>
            String url = req->url();
            if (url.startsWith("/phonebook/test/")) {
                String rest = url.substring(16);
                int slash = rest.indexOf('/');
                String idStr = (slash >= 0) ? rest.substring(0, slash) : rest;
                String extStr = (slash >= 0) ? rest.substring(slash + 1) : "";
                uint32_t id = (uint32_t)idStr.toInt();
                if (id == 0) {
                    req->send(400, "application/json", "{\"error\":\"invalid id\"}");
                    return;
                }
                const PhoneBookEntry* entry = _pbMgr->findById(id);
                if (!entry) {
                    req->send(404, "application/json", "{\"error\":\"not found\"}");
                    return;
                }

                PhoneBookEntry testEntry = *entry;
                if (extStr.length() > 0) {
                    // Test a specific extension
                    bool found = false;
                    for (const auto& x : entry->extensions) {
                        if (x.ext == extStr.c_str()) {
                            testEntry.url = entry->url + x.path;
                            testEntry.method = x.method.empty() ? entry->method : x.method;
                            testEntry.body = x.body.empty() ? entry->body : x.body;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        req->send(404, "application/json", "{\"error\":\"extension not found\"}");
                        return;
                    }
                    logger.apif("[%s] POST /phonebook/test/%u/%s",
                                 req->client()->remoteIP().toString().c_str(),
                                 (unsigned)id, extStr.c_str());
                } else {
                    logger.apif("[%s] POST /phonebook/test/%u",
                                 req->client()->remoteIP().toString().c_str(), (unsigned)id);
                }

                int httpCode = phoneBookCallerExec(testEntry);
                JsonDocument doc;
                doc["id"] = id;
                doc["httpCode"] = httpCode;
                doc["success"] = (httpCode >= 200 && httpCode < 300);
                sendJson(req, 200, doc);
                return;
            }

            if (url != "/phonebook") {
                req->send(404, "application/json", "{\"error\":\"not found\"}");
                return;
            }

            if (!req->_tempObject) {
                req->send(400, "application/json", "{\"error\":\"missing body\"}");
                return;
            }

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, (const char*)req->_tempObject);
            free(req->_tempObject);
            req->_tempObject = nullptr;

            if (err != DeserializationError::Ok) {
                req->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
                return;
            }

            PhoneBookEntry e;
            phoneBookParseJson(doc.as<JsonObject>(), e);

            if (e.number.empty() || e.url.empty()) {
                req->send(400, "application/json", "{\"error\":\"number and url are required\"}");
                return;
            }

            uint32_t id = _pbMgr->add(e);
            logger.apif("[%s] POST /phonebook: created id=%u number=%s",
                         req->client()->remoteIP().toString().c_str(),
                         (unsigned)id, e.number.c_str());

            const PhoneBookEntry* created = _pbMgr->findById(id);
            if (created) {
                JsonDocument resp;
                phoneBookFillJson(resp.to<JsonObject>(), *created, true);
                String body;
                serializeJson(resp, body);
                req->send(201, "application/json", body);
                eventsPublish("phonebook/created", body.c_str());
            }
        },
        nullptr,
        // Body handler for POST /phonebook
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                req->_tempObject = malloc(total + 1);
            }
            if (req->_tempObject) {
                memcpy((char*)req->_tempObject + index, data, len);
                if (index + len >= total) {
                    ((char*)req->_tempObject)[total] = '\0';
                }
            }
        });

    // Body handler for PUT /phonebook/{id}
    apiAddBodyHandler([](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                         size_t index, size_t total) {
        if (req->method() == HTTP_PUT && req->url().startsWith("/phonebook/")) {
            if (index == 0) {
                req->_tempObject = malloc(total + 1);
            }
            if (req->_tempObject) {
                memcpy((char*)req->_tempObject + index, data, len);
                if (index + len >= total) {
                    ((char*)req->_tempObject)[total] = '\0';
                }
            }
        }
    });

    // NotFoundHandler: PUT /phonebook/{id}, DELETE /phonebook/{id}, POST /phonebook/test/{id}
    apiAddNotFoundHandler([](AsyncWebServerRequest* request) -> bool {
        String url = request->url();
        if (!url.startsWith("/phonebook/")) return false;
        String ip = request->client()->remoteIP().toString();
        int method = request->method();

        // PUT /phonebook/<id>
        if (method == HTTP_PUT) {
            String idStr = url.substring(11);
            uint32_t id = (uint32_t)idStr.toInt();
            if (id == 0) {
                if (request->_tempObject) { free(request->_tempObject); request->_tempObject = nullptr; }
                request->send(400, "application/json", "{\"error\":\"invalid id\"}");
                return true;
            }

            const char* bodyStr = request->_tempObject ? (const char*)request->_tempObject : "{}";
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyStr);
            if (request->_tempObject) { free(request->_tempObject); request->_tempObject = nullptr; }

            if (err != DeserializationError::Ok) {
                request->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
                return true;
            }

            const PhoneBookEntry* existing = _pbMgr->findById(id);
            if (!existing) {
                request->send(404, "application/json", "{\"error\":\"not found\"}");
                return true;
            }

            PhoneBookEntry e;
            phoneBookParseJson(doc.as<JsonObject>(), e, existing);

            if (_pbMgr->update(id, e)) {
                logger.apif("[%s] PUT /phonebook/%u: updated", ip.c_str(), (unsigned)id);
                const PhoneBookEntry* updated = _pbMgr->findById(id);
                if (updated) {
                    JsonDocument resp;
                    phoneBookFillJson(resp.to<JsonObject>(), *updated, true);
                    String body;
                    serializeJson(resp, body);
                    request->send(200, "application/json", body);
                    eventsPublish("phonebook/updated", body.c_str());
                }
            } else {
                request->send(404, "application/json", "{\"error\":\"not found\"}");
            }
            return true;
        }

        // DELETE /phonebook/<id>
        if (method == HTTP_DELETE) {
            String idStr = url.substring(11);
            uint32_t id = (uint32_t)idStr.toInt();
            if (id == 0) {
                request->send(400, "application/json", "{\"error\":\"invalid id\"}");
                return true;
            }
            JsonDocument doc;
            if (_pbMgr->remove(id)) {
                logger.apif("[%s] DELETE /phonebook/%u: deleted", ip.c_str(), (unsigned)id);
                doc["status"] = "deleted";
                doc["id"] = id;
                String body;
                serializeJson(doc, body);
                request->send(200, "application/json", body);
                eventsPublish("phonebook/deleted", body.c_str());
            } else {
                doc["error"] = "not found";
                sendJson(request, 404, doc);
            }
            return true;
        }

        return false;
    });
}
