#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "posture_fsm.h"

static const char *TAG = "POSTURE_FSM";

static float s_baseline_pitch = 0.0f;
static float s_baseline_roll  = 0.0f;

void posture_fsm_set_baseline(float pitch, float roll)
{
    s_baseline_pitch = pitch;
    s_baseline_roll  = roll;
    ESP_LOGI(TAG, "Baseline set: pitch=%.2f  roll=%.2f", pitch, roll);
}

/*
 * Full FSM task implemented in Phase 5.
 */
void task_posture_fsm(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "task_posture_fsm — implemented in Phase 5");
    vTaskDelete(NULL);
}
