#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"
#include "audio.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Posture Tracker — Phase 3 starting");

    // IMU — abort on failure, nothing works without it
    esp_err_t ret = mpu6050_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: MPU-6050 init failed — halting");
        abort();
    }

    // Audio — abort on failure, device is silent without it
    ret = audio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: audio init failed — halting");
        abort();
    }

    // Boot tone — confirms speaker is alive
    audio_play_tone(500, 200, VOL_BOOT);
    ESP_LOGI(TAG, "Boot tone played. Starting IMU loop.");

    mpu6050_reset_filter();

    float pitch, roll;
    while (1) {
        ret = mpu6050_read_angles(&pitch, &roll);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "pitch=%7.2f  roll=%7.2f", pitch, roll);
        } else {
            ESP_LOGW(TAG, "Read failed: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_RATE_MS));
    }
}
