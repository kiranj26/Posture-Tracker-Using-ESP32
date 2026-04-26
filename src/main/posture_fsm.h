#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "audio.h"

// ── Shared IMU data struct ────────────────────────────────────────────────
typedef struct {
    float   pitch;
    float   roll;
    int64_t timestamp_us;
} imu_data_t;

// ── Globals defined in main.c, used by FSM and audio tasks ───────────────
extern imu_data_t         g_imu_data;
extern SemaphoreHandle_t  g_imu_mutex;
extern EventGroupHandle_t g_evt_group;
extern QueueHandle_t      g_audio_queue;

#define EVT_IMU_DATA_READY  (1 << 0)

// ── Public API ────────────────────────────────────────────────────────────
void posture_fsm_set_baseline(float pitch, float roll);
void task_posture_fsm(void *arg);
