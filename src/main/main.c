#include "config.h"
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"
#include "audio.h"
#include "posture_fsm.h"

static const char *TAG = "MAIN";

/*
 * Collect CAL_TOTAL_SAMPLES angle readings, discard the first
 * CAL_DISCARD_SAMPLES (filter warm-up), then compute mean and stddev.
 * If stddev exceeds CAL_MAX_STDDEV_DEG the user moved — play fail tone
 * and retry. Loops until a clean calibration is accepted.
 */
static void calibrate_baseline(void)
{
    float pitch, roll;

    while (1) {
        mpu6050_reset_filter();

        // Prompt: ascending 3-note — "sit straight now"
        extern void audio_play_sequence(const tone_t *tones, uint8_t count);
        // Defined as static in audio.c — call via the public wrapper below
        // by playing each tone individually using audio_play_tone.
        audio_play_tone(400, 150, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(50));
        audio_play_tone(600, 150, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(50));
        audio_play_tone(800, 150, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "Calibration: collecting %d samples...", CAL_VALID_SAMPLES);

        float pitch_sum    = 0.0f;
        float roll_sum     = 0.0f;
        float pitch_sq_sum = 0.0f;
        float roll_sq_sum  = 0.0f;

        for (int i = 0; i < CAL_TOTAL_SAMPLES; i++) {
            mpu6050_read_angles(&pitch, &roll);
            if (i >= CAL_DISCARD_SAMPLES) {
                pitch_sum    += pitch;
                roll_sum     += roll;
                pitch_sq_sum += pitch * pitch;
                roll_sq_sum  += roll  * roll;
            }
            vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_RATE_MS));
        }

        float n           = (float)CAL_VALID_SAMPLES;
        float pitch_mean  = pitch_sum / n;
        float roll_mean   = roll_sum  / n;

        // Validate on pitch stddev only. roll = atan2(gy, gz) is numerically
        // unstable when the sensor is vertical (gz ≈ 0) — e.g. mounted on a
        // human back — producing large roll noise even when perfectly still.
        // Pitch = atan2(gx, sqrt(gy²+gz²)) has no such singularity and is
        // the primary axis for detecting forward slouch.
        float pitch_std = sqrtf(fabsf(pitch_sq_sum / n - pitch_mean * pitch_mean));

        ESP_LOGI(TAG, "Cal result: pitch=%.2f  roll=%.2f  pitch_stddev=%.2f",
                 pitch_mean, roll_mean, pitch_std);

        if (pitch_std > CAL_MAX_STDDEV_DEG) {
            ESP_LOGW(TAG, "Cal rejected — too much movement (pitch_stddev=%.2f > %.1f)",
                     pitch_std, CAL_MAX_STDDEV_DEG);
            // Fail tone: descending 2-note
            audio_play_tone(600, 150, VOL_CHIME);
            vTaskDelay(pdMS_TO_TICKS(50));
            audio_play_tone(400, 200, VOL_CHIME);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        posture_fsm_set_baseline(pitch_mean, roll_mean);

        // Success tone: C5 → E5 → G5 arpeggio
        audio_play_tone(523, 100, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(30));
        audio_play_tone(659, 100, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(30));
        audio_play_tone(784, 150, VOL_CHIME);

        ESP_LOGI(TAG, "Calibration accepted.");
        return;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Posture Tracker — Phase 4 starting");

    esp_err_t ret = mpu6050_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: MPU-6050 init failed — halting");
        abort();
    }

    ret = audio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: audio init failed — halting");
        abort();
    }

    // Boot tone
    audio_play_tone(500, 200, VOL_BOOT);

    // Calibration — blocks until accepted
    calibrate_baseline();

    ESP_LOGI(TAG, "Starting monitoring loop.");

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
