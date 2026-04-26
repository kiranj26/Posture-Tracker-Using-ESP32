#include "config.h"
#include <math.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "mpu6050.h"
#include "audio.h"
#include "posture_fsm.h"

static const char *TAG = "MAIN";

// ── Shared globals (extern'd in posture_fsm.h) ────────────────────────────
imu_data_t         g_imu_data    = {0};
SemaphoreHandle_t  g_imu_mutex   = NULL;
EventGroupHandle_t g_evt_group   = NULL;
QueueHandle_t      g_audio_queue = NULL;

// ── Task A — IMU Reader ───────────────────────────────────────────────────
static void task_read_imu(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "IMU task started");

    uint8_t consecutive_fail = 0;
    float pitch, roll;

    for (;;) {
        esp_err_t ret = mpu6050_read_angles(&pitch, &roll);

        if (ret != ESP_OK) {
            consecutive_fail++;
            ESP_LOGW(TAG, "IMU read fail #%d", consecutive_fail);
            if (consecutive_fail >= 3) {
                ESP_LOGE(TAG, "3 consecutive IMU failures — restarting");
                esp_restart();
            }
        } else {
            consecutive_fail = 0;

            if (xSemaphoreTake(g_imu_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
                g_imu_data.pitch        = pitch;
                g_imu_data.roll         = roll;
                g_imu_data.timestamp_us = esp_timer_get_time();
                xSemaphoreGive(g_imu_mutex);
                xEventGroupSetBits(g_evt_group, EVT_IMU_DATA_READY);
            } else {
                ESP_LOGW(TAG, "IMU mutex timeout — sample skipped");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_RATE_MS));
    }
}

// ── Calibration (synchronous, runs before tasks are created) ─────────────
static void calibrate_baseline(void)
{
    float pitch, roll;

    while (1) {
        mpu6050_reset_filter();

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

        float n          = (float)CAL_VALID_SAMPLES;
        float pitch_mean = pitch_sum / n;
        float roll_mean  = roll_sum  / n;
        float pitch_std  = sqrtf(fabsf(pitch_sq_sum / n - pitch_mean * pitch_mean));

        ESP_LOGI(TAG, "Cal result: pitch=%.2f  roll=%.2f  pitch_stddev=%.2f",
                 pitch_mean, roll_mean, pitch_std);

        if (pitch_std > CAL_MAX_STDDEV_DEG) {
            ESP_LOGW(TAG, "Cal rejected — too much movement (pitch_stddev=%.2f > %.1f)",
                     pitch_std, CAL_MAX_STDDEV_DEG);
            audio_play_tone(600, 150, VOL_CHIME);
            vTaskDelay(pdMS_TO_TICKS(50));
            audio_play_tone(400, 200, VOL_CHIME);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        posture_fsm_set_baseline(pitch_mean, roll_mean);

        audio_play_tone(523, 100, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(30));
        audio_play_tone(659, 100, VOL_CHIME);
        vTaskDelay(pdMS_TO_TICKS(30));
        audio_play_tone(784, 150, VOL_CHIME);

        ESP_LOGI(TAG, "Calibration accepted.");
        return;
    }
}

// ── app_main ─────────────────────────────────────────────────────────────
void app_main(void)
{
    ESP_LOGI(TAG, "Posture Tracker — Phase 5 starting");

    // NVS — erase and retry if corrupted
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) ESP_LOGW(TAG, "NVS init failed — continuing without NVS");
    else ESP_LOGI(TAG, "NVS initialised");

    ret = mpu6050_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: MPU-6050 init failed");
        abort();
    }

    ret = audio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: audio init failed");
        abort();
    }

    // Boot tone
    audio_play_tone(500, 200, VOL_BOOT);

    // Calibration — blocks until accepted
    calibrate_baseline();

    // FreeRTOS primitives
    g_imu_mutex   = xSemaphoreCreateMutex();
    g_evt_group   = xEventGroupCreate();
    g_audio_queue = xQueueCreate(AUDIO_QUEUE_DEPTH, sizeof(audio_cmd_t));

    configASSERT(g_imu_mutex   != NULL);
    configASSERT(g_evt_group   != NULL);
    configASSERT(g_audio_queue != NULL);

    // Launch tasks
    BaseType_t r;
    r = xTaskCreatePinnedToCore(task_read_imu,    "imu",
                                TASK_STACK_IMU,   NULL,
                                TASK_PRIO_IMU,    NULL, TASK_CORE_IMU);
    configASSERT(r == pdPASS);

    r = xTaskCreatePinnedToCore(task_posture_fsm, "fsm",
                                TASK_STACK_FSM,   NULL,
                                TASK_PRIO_FSM,    NULL, TASK_CORE_FSM);
    configASSERT(r == pdPASS);

    r = xTaskCreatePinnedToCore(task_audio,       "audio",
                                TASK_STACK_AUDIO, NULL,
                                TASK_PRIO_AUDIO,  NULL, TASK_CORE_AUDIO);
    configASSERT(r == pdPASS);

    ESP_LOGI(TAG, "All tasks launched. Monitoring active.");
    // app_main returns — FreeRTOS scheduler takes over
}
