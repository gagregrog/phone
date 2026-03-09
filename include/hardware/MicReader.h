#pragma once
#include <stdint.h>
#include <functional>
#include <driver/i2s.h>
#include <driver/adc.h>

class MicReader {
public:
    using BufferCallback = std::function<void(const int16_t* samples, size_t count)>;

    // bufferSize: samples per DMA read → also samples per WS frame.
    // 256 samples @ 8 kHz = ~31 frames/sec, safely below AsyncWebSocket's 32-message queue cap.
    MicReader(adc1_channel_t channel, uint32_t sampleRateHz = 8000, size_t bufferSize = 256);
    void begin();       // Install I2S driver, configure ADC channel + attenuation
    void startTask();   // Launch FreeRTOS streaming task — call after setOnBuffer()
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setOnBuffer(BufferCallback cb);
    void setTestTone(bool enabled);  // Generate 440 Hz sine wave instead of ADC data

private:
    bool _testTone = false;
    adc1_channel_t _channel;
    uint32_t _sampleRateHz;
    size_t _bufferSize;
    bool _enabled = false;
    BufferCallback _onBuffer;

    static void taskFunc(void* param);
    void runTask();
};
