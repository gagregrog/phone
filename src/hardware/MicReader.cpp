#include "hardware/MicReader.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Post-filter amplification.  The carbon mic + NTE823 produces ~150 ADC counts
// peak-to-peak during speech; this gain brings that into a usable int16 range.
// Increase for more volume, decrease to reduce amplified noise.
// 50.0f is a good starting point.
static const float MIC_GAIN = 60.0f;

MicReader::MicReader(adc1_channel_t channel, uint32_t sampleRateHz, size_t bufferSize)
    : _channel(channel), _sampleRateHz(sampleRateHz), _bufferSize(bufferSize) {}

void MicReader::begin() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
    cfg.sample_rate = _sampleRateHz;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_MSB;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 8;
    cfg.dma_buf_len = (int)_bufferSize;  // max 1024 per ESP-IDF constraint
    // The default PLL (PLL_D2) can't divide cleanly to 44100 Hz — the actual
    // sample rate lands around ~37–41 kHz.  APLL gets closer but sounded worse
    // in testing; 44100 with the default PLL produces intelligible audio when
    // the browser plays back at 44100.
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = false;
    cfg.fixed_mclk = 0;

    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(_channel, ADC_ATTEN_DB_6);
    i2s_set_adc_mode(ADC_UNIT_1, _channel);
    i2s_adc_enable(I2S_NUM_0);
}

void MicReader::startTask() {
    xTaskCreate(taskFunc, "mic_reader", 4096, this, 1, NULL);
}

void MicReader::setEnabled(bool enabled) {
    _enabled = enabled;
}

bool MicReader::isEnabled() const {
    return _enabled;
}

void MicReader::setOnBuffer(BufferCallback cb) {
    _onBuffer = cb;
}

void MicReader::taskFunc(void* param) {
    static_cast<MicReader*>(param)->runTask();
}

void MicReader::runTask() {
    int16_t* buf = new int16_t[_bufferSize];
    float dcEstimate = 0;
    bool dcInitialized = false;
    float lpState = 0;
    // Single-pole IIR low-pass at ~4 kHz.  Speech lives below 4 kHz; everything
    // above is ESP32 ADC noise amplified by our gain stage.  A 100 nF ceramic cap
    // from the ADC pin to ground provides additional hardware filtering.
    // alpha = 1 - exp(-2*pi*4000/~37730) ≈ 0.49
    const float lpAlpha = 0.49f;

    while (true) {
        if (!_enabled || !_onBuffer) {
            dcInitialized = false;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, buf, _bufferSize * sizeof(int16_t),
                 &bytesRead, portMAX_DELAY);

        size_t n = bytesRead / sizeof(int16_t);

        for (size_t i = 0; i < n; i++) {
            // Each 16-bit DMA word: upper 4 bits = ADC channel, lower 12 = reading.
            // No interleaving occurs in ONLY_LEFT mode — all samples are valid.
            float raw = (float)((uint16_t)buf[i] & 0x0FFF);

            // DC removal: the carbon mic + NTE823 amp biases the signal at ~1850
            // (not the theoretical midpoint of 2048).  A slow-tracking EMA subtracts
            // whatever the actual DC level is, centering the waveform around zero.
            if (!dcInitialized) { dcEstimate = raw; dcInitialized = true; }
            dcEstimate += 0.001f * (raw - dcEstimate);

            // Low-pass → amplify.  The carbon mic produces a very small ADC swing
            // (~150 counts peak-to-peak during speech), so a gain of 50 brings it
            // into a usable int16 range without clipping.
            float centered = raw - dcEstimate;
            lpState += lpAlpha * (centered - lpState);

            int32_t sample = (int32_t)(lpState * MIC_GAIN);
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            buf[i] = (int16_t)sample;
        }
        if (n > 0) _onBuffer(buf, n);
    }
}
