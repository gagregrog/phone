#include "hardware/MicEvents.h"
#include "web/Events.h"
#include "web/WebSocketAPI.h"
#include <new>
#include <string.h>

void micEventsBegin(MicReader& mic) {
    // Gate sampling: enable only when WebSocket clients are connected
    eventsSubscribe([&mic](const char* topic, const char* payload) {
        if (strcmp(topic, "ws/clients") != 0) return;
        // payload is e.g. {"count":2}
        // Simple parse: find digits after "count":
        const char* p = strstr(payload, "\"count\":");
        if (!p) return;
        p += 8; // skip "count":
        while (*p == ' ') p++;
        int count = 0;
        while (*p >= '0' && *p <= '9') { count = count * 10 + (*p - '0'); p++; }
        mic.setEnabled(count > 0);
    });

    // Batch two 1024-sample DMA reads into one 2048-sample WS frame.
    // At 44100 Hz this yields ~21 frames/sec.  Sending 1024-sample frames
    // (~43/sec) overwhelmed the AsyncWebSocket queue and caused crashes
    // (abort in _queueMessage).  Doubling the frame size halves queue pressure
    // while adding only ~23 ms of extra latency per frame.
    //
    // Frame format: [0x01 (type byte) | int16 PCM samples...]
    // wsEnqueueBinary takes ownership of the buffer.
    static uint8_t* pending = nullptr;
    static size_t pendingBytes = 0;

    mic.setOnBuffer([](const int16_t* samples, size_t count) {
        size_t payloadBytes = count * sizeof(int16_t);

        if (!pending) {
            // First half: allocate frame for two buffers, copy first batch
            pending = new (std::nothrow) uint8_t[1 + payloadBytes * 2];
            if (!pending) return;  // heap exhausted — drop frame
            pending[0] = 0x01;
            memcpy(pending + 1, samples, payloadBytes);
            pendingBytes = payloadBytes;
        } else {
            // Second half: append and send
            memcpy(pending + 1 + pendingBytes, samples, payloadBytes);
            wsEnqueueBinary(pending, 1 + pendingBytes + payloadBytes);
            pending = nullptr;
            pendingBytes = 0;
        }
    });
}
