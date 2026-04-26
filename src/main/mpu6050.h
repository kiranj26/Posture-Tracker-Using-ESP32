#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read_angles(float *pitch, float *roll);
void      mpu6050_reset_filter(void);
