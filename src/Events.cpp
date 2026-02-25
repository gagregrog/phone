#include "Events.h"
#include <vector>

static std::vector<EventHandler> _handlers;

void eventsSubscribe(EventHandler handler) {
    _handlers.push_back(std::move(handler));
}

void eventsPublish(const char* topic, const char* payload) {
    for (auto& h : _handlers) {
        h(topic, payload);
    }
}
