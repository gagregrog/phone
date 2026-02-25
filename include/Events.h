#pragma once
#include <functional>

// Lightweight pub/sub event bus.
//
// Publishers call eventsPublish(topic, payload) and don't care who listens.
// Subscribers register once at startup via eventsSubscribe().
//
// Topics follow a "section/action" convention, e.g.:
//   "ring/started", "timer/expired", "alarm/fired", "clock/updated"
//
// Payload is a JSON string. Topic strings map directly to MQTT topics —
// a future MQTT subscriber just calls mqttClient.publish(topic, payload).

using EventHandler = std::function<void(const char* topic, const char* payload)>;

void eventsSubscribe(EventHandler handler);
void eventsPublish(const char* topic, const char* payload);
