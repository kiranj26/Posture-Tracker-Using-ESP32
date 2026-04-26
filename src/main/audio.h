#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    uint16_t freq_hz;
    uint16_t dur_ms;
    float    volume;
    uint16_t gap_ms;
} tone_t;

typedef enum {
    AUDIO_CMD_BOOT_READY,
    AUDIO_CMD_CAL_PROMPT,
    AUDIO_CMD_CAL_SUCCESS,
    AUDIO_CMD_CAL_FAIL,
    AUDIO_CMD_WARN_BEEP,
    AUDIO_CMD_BAD_ALERT,
    AUDIO_CMD_VOICE_CLIP,
    AUDIO_CMD_REWARD_CHIME,
    AUDIO_CMD_CORRECTION_CONFIRM,
    AUDIO_CMD_STOP,
} audio_cmd_e;

typedef struct {
    audio_cmd_e type;
} audio_cmd_t;

esp_err_t audio_init(void);
void      audio_play_tone(uint16_t freq_hz, uint16_t dur_ms, float volume);
void      audio_play_sequence(const tone_t *tones, uint8_t count);
void      audio_play_wav(const uint8_t *data, uint32_t len);
void      task_audio(void *arg);
