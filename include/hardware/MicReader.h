#pragma once
#include <stdint.h>
#include <functional>
#include <driver/i2s.h>
#include <driver/adc.h>

class MicReader {
public:
    using BufferCallback = std::function<void(const int16_t* samples, size_t count)>;

    // bufferSize: samples per DMA read (max 1024).
    // Two DMA reads are batched into one WS frame by MicEvents (~21 frames/sec at 44100 Hz).
    MicReader(adc1_channel_t channel, uint32_t sampleRateHz = 44100, size_t bufferSize = 1024);
    void begin();       // Install I2S driver, configure ADC channel + attenuation
    void startTask();   // Launch FreeRTOS streaming task — call after setOnBuffer()
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setOnBuffer(BufferCallback cb);

private:
    adc1_channel_t _channel;
    uint32_t _sampleRateHz;
    size_t _bufferSize;
    bool _enabled = false;
    BufferCallback _onBuffer;

    static void taskFunc(void* param);
    void runTask();
};
