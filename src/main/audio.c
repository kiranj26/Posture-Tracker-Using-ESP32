#include "config.h"
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "audio.h"

static const char *TAG = "AUDIO";

esp_err_t audio_init(void) {
    ESP_LOGI(TAG, "audio_init — stub (Phase 0)");
    return ESP_OK;
}

void audio_play_tone(uint16_t freq_hz, uint16_t dur_ms, float volume) {
    (void)freq_hz; (void)dur_ms; (void)volume;
}

void audio_play_sequence(const tone_t *tones, uint8_t count) {
    (void)tones; (void)count;
}

void audio_play_wav(const uint8_t *data, uint32_t len) {
    (void)data; (void)len;
}

void task_audio(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "task_audio — stub (Phase 0)");
    vTaskDelete(NULL);
}
