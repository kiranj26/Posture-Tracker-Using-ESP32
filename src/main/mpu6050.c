#include "config.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "mpu6050.h"

static const char *TAG = "MPU6050";

// EMA filter state — reset via mpu6050_reset_filter() before calibration
static float prev_pitch = 0.0f;
static float prev_roll  = 0.0f;

// ── Internal helpers ──────────────────────────────────────────────────────────

/*
 * Write a single byte to a MPU-6050 register.
 * Sequence: START → device addr+W → reg addr → value → STOP
 */
static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd,
                                         pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/*
 * Burst-read `len` bytes starting at `reg` into `buf`.
 * Sequence: START → addr+W → reg → REPEATED START → addr+R → read N bytes → STOP
 */
static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);   // repeated start — no STOP between write and read
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd,
                                         pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// ── Public API ────────────────────────────────────────────────────────────────

/*
 * Initialise the I2C master bus and configure the MPU-6050 sensor.
 *
 * Steps (order is mandatory per MPU-6050 datasheet):
 *   1. Install I2C master driver
 *   2. Wake sensor from sleep (PWR_MGMT_1 = 0x00)
 *   3. Wait 100ms for sensor to stabilise after wake
 *   4. Verify WHO_AM_I register matches expected 0x68
 *   5. Set sample rate divider → 100Hz output rate
 *   6. Set DLPF → ~21Hz bandwidth (filters vibration)
 *   7. Set accel range → ±2g (16384 LSB/g)
 *   8. Set gyro range  → ±250°/s (gyro unused in MVP but must be configured)
 */
esp_err_t mpu6050_init(void)
{
    // 1. Configure and install I2C master driver
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA_IO,
        .scl_io_num       = I2C_MASTER_SCL_IO,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,  // GY-521 has onboard 4.7kΩ pull-ups
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. Wake sensor — default state after power-on is SLEEP mode
    ret = mpu6050_write_reg(MPU_REG_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake MPU-6050: %s", esp_err_to_name(ret));
        return ret;
    }

    // 3. Give sensor 100ms to stabilise after waking
    vTaskDelay(pdMS_TO_TICKS(100));

    // 4. Verify device identity
    uint8_t who = 0;
    ret = mpu6050_read_regs(MPU_REG_WHO_AM_I, &who, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WHO_AM_I read failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if (who != MPU_WHO_AM_I_RESPONSE) {
        ESP_LOGE(TAG, "WHO_AM_I mismatch: expected 0x%02X, got 0x%02X",
                 MPU_WHO_AM_I_RESPONSE, who);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "WHO_AM_I: 0x%02X — device confirmed", who);

    // 5. Sample rate = 1kHz / (1 + 9) = 100Hz
    ret = mpu6050_write_reg(MPU_REG_SMPLRT_DIV, 0x09);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "SMPLRT_DIV write failed"); return ret; }

    // 6. DLPF bandwidth ~21Hz — attenuates high-frequency vibration
    ret = mpu6050_write_reg(MPU_REG_CONFIG, 0x04);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "CONFIG write failed"); return ret; }

    // 7. Accel ±2g range → 16384 LSB/g
    ret = mpu6050_write_reg(MPU_REG_ACCEL_CONFIG, 0x00);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "ACCEL_CONFIG write failed"); return ret; }

    // 8. Gyro ±250°/s range (not used in MVP but must be initialised)
    ret = mpu6050_write_reg(MPU_REG_GYRO_CONFIG, 0x00);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "GYRO_CONFIG write failed"); return ret; }

    ESP_LOGI(TAG, "Registers configured OK");
    return ESP_OK;
}

/*
 * Burst-read accelerometer, compute pitch and roll angles, apply EMA filter.
 *
 * Raw pipeline:
 *   int16 → divide by 16384 → g units → atan2f → degrees → EMA filter
 *
 * Angle formulas (accel-only, gravity vector):
 *   pitch = atan2(gx, sqrt(gy² + gz²)) * 180/π  — forward/backward tilt
 *   roll  = atan2(gy, gz) * 180/π               — sideways tilt
 *
 * EMA filter: output = alpha * new + (1 - alpha) * prev
 * LPF_ALPHA = 0.15 → strong smoothing, ~1s to converge from cold start.
 *
 * Note: sensor sits at ~45° on breadboard so resting values are non-zero.
 * Phase 4 calibration captures the resting angle as baseline — this is correct.
 */
esp_err_t mpu6050_read_angles(float *pitch, float *roll)
{
    uint8_t buf[6];
    esp_err_t ret = mpu6050_read_regs(MPU_REG_ACCEL_XOUT_H, buf, 6);
    if (ret != ESP_OK) return ret;

    int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);

    float gx = raw_ax / MPU_ACCEL_SENSITIVITY;
    float gy = raw_ay / MPU_ACCEL_SENSITIVITY;
    float gz = raw_az / MPU_ACCEL_SENSITIVITY;

    float raw_pitch = atan2f(gx, sqrtf(gy * gy + gz * gz)) * (180.0f / M_PI);
    float raw_roll  = atan2f(gy, gz) * (180.0f / M_PI);

    // Exponential moving average — smooths sensor noise, attenuates vibration
    *pitch = LPF_ALPHA * raw_pitch + (1.0f - LPF_ALPHA) * prev_pitch;
    *roll  = LPF_ALPHA * raw_roll  + (1.0f - LPF_ALPHA) * prev_roll;

    prev_pitch = *pitch;
    prev_roll  = *roll;

    return ESP_OK;
}

/*
 * Reset low-pass filter state to zero.
 * Call before starting calibration to flush stale filter history.
 */
void mpu6050_reset_filter(void)
{
    prev_pitch = 0.0f;
    prev_roll  = 0.0f;
}
