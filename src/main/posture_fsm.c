#include "config.h"
#include <math.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "posture_fsm.h"
#include "audio.h"

static const char *TAG = "POSTURE_FSM";

typedef enum {
    STATE_GOOD,
    STATE_WARNING,
    STATE_BAD,
    STATE_RESET,
} posture_state_t;

static const char *state_name(posture_state_t s)
{
    switch (s) {
        case STATE_GOOD:    return "GOOD";
        case STATE_WARNING: return "WARNING";
        case STATE_BAD:     return "BAD";
        case STATE_RESET:   return "RESET";
        default:            return "UNKNOWN";
    }
}

static posture_state_t s_state          = STATE_GOOD;
static float           s_baseline_pitch = 0.0f;
static float           s_baseline_roll  = 0.0f;
static int64_t         s_warn_start_ms  = 0;
static int64_t         s_bad_start_ms   = 0;
static int64_t         s_reset_start_ms = 0;
static int64_t         s_good_streak_ms = 0;
static int64_t         s_last_alert_ms  = 0;
static uint8_t         s_escalation     = 0;
static bool            s_warn_fired     = false;

void posture_fsm_set_baseline(float pitch, float roll)
{
    s_baseline_pitch = pitch;
    s_baseline_roll  = roll;
    ESP_LOGI(TAG, "Baseline set: pitch=%.2f  roll=%.2f", pitch, roll);
}

static void enqueue_audio(audio_cmd_e type)
{
    audio_cmd_t cmd = { .type = type };
    if (xQueueSend(g_audio_queue, &cmd, 0) != pdTRUE) {
        ESP_LOGD(TAG, "Audio queue full — command %d dropped", type);
    }
}

void task_posture_fsm(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "FSM task started");

    posture_state_t prev_state = STATE_GOOD;
    imu_data_t snap;

    for (;;) {
        // Block until Task A signals new IMU data — burns zero CPU while waiting
        xEventGroupWaitBits(g_evt_group, EVT_IMU_DATA_READY,
                            pdTRUE, pdFALSE, portMAX_DELAY);

        // Copy shared data under mutex then release immediately
        if (xSemaphoreTake(g_imu_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
            ESP_LOGW(TAG, "Mutex timeout — skipping sample");
            continue;
        }
        snap = g_imu_data;
        xSemaphoreGive(g_imu_mutex);

        // Deviation from calibrated baseline
        float dp    = fabsf(snap.pitch - s_baseline_pitch);
        float dr    = fabsf(snap.roll  - s_baseline_roll);
        float delta = (dp > dr) ? dp : dr;

        // Discard violent movements — not a posture event
        if (delta > JUMP_FILTER_DEG) {
            ESP_LOGD(TAG, "Jump filtered: delta=%.1f", delta);
            continue;
        }

        int64_t now_ms = esp_timer_get_time() / 1000;

        switch (s_state) {

        case STATE_GOOD:
            if (s_good_streak_ms == 0) s_good_streak_ms = now_ms;

            // Reward chime after sustained good posture
            if ((now_ms - s_good_streak_ms) >= GOOD_STREAK_REWARD_MS) {
                enqueue_audio(AUDIO_CMD_REWARD_CHIME);
                s_good_streak_ms = now_ms;
            }

            if (delta > WARN_THRESH_DEG) {
                s_warn_start_ms = now_ms;
                s_bad_start_ms  = 0;
                s_warn_fired    = false;
                s_state         = STATE_WARNING;
            }
            break;

        case STATE_WARNING:
            s_good_streak_ms = 0;

            if (delta < WARN_THRESH_DEG) {
                s_warn_fired = false;
                s_state      = STATE_GOOD;
                break;
            }

            // Escalate to BAD if sustained above bad threshold
            if (delta > BAD_THRESH_DEG) {
                if (s_bad_start_ms == 0) s_bad_start_ms = now_ms;
                if ((now_ms - s_bad_start_ms) >= BAD_GRACE_MS) {
                    enqueue_audio(AUDIO_CMD_BAD_ALERT);
                    s_last_alert_ms = now_ms;
                    s_escalation    = 1;
                    s_state         = STATE_BAD;
                    break;
                }
            } else {
                s_bad_start_ms = 0;
            }

            // Warn once per WARNING entry after grace period
            if (!s_warn_fired && (now_ms - s_warn_start_ms) >= WARN_GRACE_MS) {
                enqueue_audio(AUDIO_CMD_WARN_BEEP);
                s_warn_fired = true;
            }
            break;

        case STATE_BAD:
            s_good_streak_ms = 0;

            if (delta < WARN_THRESH_DEG) {
                enqueue_audio(AUDIO_CMD_CORRECTION_CONFIRM);
                s_reset_start_ms = now_ms;
                s_state          = STATE_RESET;
                break;
            }

            // Escalating repeat alerts — beeps first, then voice clip repeating forever
            if ((now_ms - s_last_alert_ms) >= REPEAT_ALERT_MS) {
                if (s_escalation < ESCALATION_CAP) {
                    enqueue_audio(AUDIO_CMD_BAD_ALERT);
                    s_escalation++;
                } else {
                    // Cap reached — keep repeating voice clip until posture corrected
                    enqueue_audio(AUDIO_CMD_VOICE_CLIP);
                }
                s_last_alert_ms = now_ms;
            }
            break;

        case STATE_RESET:
            if (delta > WARN_THRESH_DEG) {
                s_warn_start_ms = now_ms;
                s_bad_start_ms  = 0;
                s_warn_fired    = false;
                s_state         = STATE_WARNING;
                break;
            }

            if ((now_ms - s_reset_start_ms) >= RESET_COOLDOWN_MS) {
                s_good_streak_ms = now_ms;
                s_escalation     = 0;
                s_warn_fired     = false;
                s_state          = STATE_GOOD;
            }
            break;
        }

        if (s_state != prev_state) {
            ESP_LOGI(TAG, "STATE: %s → %s | delta=%.1f",
                     state_name(prev_state), state_name(s_state), delta);
            prev_state = s_state;
        }
    }
}
