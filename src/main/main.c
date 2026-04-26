#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Posture Tracker — Phase 1 starting");

    // Initialise I2C bus and MPU-6050. Abort on failure — nothing works without IMU.
    esp_err_t ret = mpu6050_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: MPU-6050 init failed — halting");
        abort();
    }

    ESP_LOGI(TAG, "MPU-6050 ready. Starting raw data loop...");

    int16_t ax, ay, az;
    while (1) {
        ret = mpu6050_read_raw(&ax, &ay, &az);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ax=%6d  ay=%6d  az=%6d", ax, ay, az);
        } else {
            ESP_LOGW(TAG, "Read failed: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_RATE_MS));
    }
}
