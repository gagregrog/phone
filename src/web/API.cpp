#include "web/API.h"
#include "system/Logger.h"
#include <ESPAsyncWebServer.h>
#include <vector>

static AsyncWebServer* _server = nullptr;
static std::vector<NotFoundHandler> _notFoundHandlers;
static std::vector<BodyHandler> _bodyHandlers;

void apiInit(uint16_t port) {
    _server = new AsyncWebServer(port);
}

void apiStart() {
    _server->onNotFound([](AsyncWebServerRequest* request) {
        for (auto& h : _notFoundHandlers) {
            if (h(request)) return;
        }
        logger.apif("[%s] 404 %s", request->client()->remoteIP().toString().c_str(), request->url().c_str());
        request->send(404, "text/plain", "Not Found");
    });

    _server->onRequestBody([](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                              size_t index, size_t total) {
        for (auto& h : _bodyHandlers) {
            h(req, data, len, index, total);
        }
    });

    _server->begin();
    logger.api("API listening");
}

AsyncWebServer* apiGetServer() {
    return _server;
}

void apiAddNotFoundHandler(NotFoundHandler h) {
    _notFoundHandlers.push_back(h);
}

void apiAddBodyHandler(BodyHandler h) {
    _bodyHandlers.push_back(h);
}
