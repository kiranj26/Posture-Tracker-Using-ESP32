#include "config.h"
#include <string.h>
#include "esp_log.h"
#include "mpu6050.h"

static const char *TAG = "MPU6050";

esp_err_t mpu6050_init(void) {
    ESP_LOGI(TAG, "mpu6050_init — stub (Phase 0)");
    return ESP_OK;
}

esp_err_t mpu6050_read_angles(float *pitch, float *roll) {
    *pitch = 0.0f;
    *roll  = 0.0f;
    return ESP_OK;
}

void mpu6050_reset_filter(void) {
    // stub
}
