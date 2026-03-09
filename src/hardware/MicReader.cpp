#include "hardware/MicReader.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

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
    cfg.dma_buf_len = (int)_bufferSize;
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

void MicReader::setTestTone(bool enabled) {
    _testTone = enabled;
}

void MicReader::taskFunc(void* param) {
    static_cast<MicReader*>(param)->runTask();
}

void MicReader::runTask() {
    int16_t* buf = new int16_t[_bufferSize];
    float dcEstimate = 0;
    bool dcInitialized = false;
    float tonePhase = 0;
    const float toneFreq = 440.0f;
    const float toneAmp = 16000.0f;
    const float phaseInc = 2.0f * M_PI * toneFreq / (float)_sampleRateHz;

    while (true) {
        if (!_enabled || !_onBuffer) {
            dcInitialized = false;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (_testTone) {
            // Generate a 440 Hz sine wave — bypasses ADC entirely
            for (size_t i = 0; i < _bufferSize; i++) {
                buf[i] = (int16_t)(toneAmp * sinf(tonePhase));
                tonePhase += phaseInc;
                if (tonePhase >= 2.0f * M_PI) tonePhase -= 2.0f * M_PI;
            }
            _onBuffer(buf, _bufferSize);
            // Pace to real-time: _bufferSize samples at _sampleRateHz
            vTaskDelay(pdMS_TO_TICKS(_bufferSize * 1000 / _sampleRateHz));
            continue;
        }

        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, buf, _bufferSize * sizeof(int16_t),
                 &bytesRead, portMAX_DELAY);

        size_t n = bytesRead / sizeof(int16_t);

        for (size_t i = 0; i < n; i++) {
            float raw = (float)((uint16_t)buf[i] & 0x0FFF);

            // Track DC level with exponential moving average
            if (!dcInitialized) { dcEstimate = raw; dcInitialized = true; }
            dcEstimate += 0.001f * (raw - dcEstimate);

            // AC-coupled: remove DC, then amplify
            int32_t sample = (int32_t)((raw - dcEstimate) * 50.0f);
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            buf[i] = (int16_t)sample;
        }
        if (n > 0) _onBuffer(buf, n);
    }
}
