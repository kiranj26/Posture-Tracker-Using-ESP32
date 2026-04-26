#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "posture_fsm.h"

static const char *TAG = "POSTURE_FSM";

void posture_fsm_set_baseline(float pitch, float roll) {
    ESP_LOGI(TAG, "posture_fsm_set_baseline — stub (Phase 0): pitch=%.2f roll=%.2f", pitch, roll);
}

void task_posture_fsm(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "task_posture_fsm — stub (Phase 0)");
    vTaskDelete(NULL);
}
