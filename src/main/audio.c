#include "config.h"
#include <stdint.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "audio.h"

static const char *TAG = "AUDIO";

// ── Tone sequence definitions ─────────────────────────────────────────────────
// Static — internal to this file. Not exposed via header.

static const tone_t TONES_BOOT[]       = {{ 500, 200, VOL_BOOT,    0  }};
static const tone_t TONES_CAL_PROMPT[] = {{ 400, 150, VOL_CHIME,   50 },
                                           { 600, 150, VOL_CHIME,   50 },
                                           { 800, 150, VOL_CHIME,   0  }};
static const tone_t TONES_CAL_OK[]     = {{ 523, 100, VOL_CHIME,   30 },
                                           { 659, 100, VOL_CHIME,   30 },
                                           { 784, 150, VOL_CHIME,   0  }};
static const tone_t TONES_CAL_FAIL[]   = {{ 600, 150, VOL_CHIME,   50 },
                                           { 400, 200, VOL_CHIME,   0  }};
static const tone_t TONES_WARN[]       = {{ 700, 120, VOL_WARNING,  0  }};
static const tone_t TONES_BAD[]        = {{ 1000,200, VOL_BAD,     100 },
                                           { 1000,200, VOL_BAD,     0   }};
static const tone_t TONES_CONFIRM[]    = {{ 600, 100, VOL_CHIME,   40 },
                                           { 400, 120, VOL_CHIME,   0  }};
static const tone_t TONES_REWARD[]     = {{ 784, 100, VOL_CHIME,   30 },
                                           { 659, 100, VOL_CHIME,   30 },
                                           { 523, 150, VOL_CHIME,   0  }};

// ── Public API ────────────────────────────────────────────────────────────────

/*
 * Install the I2S master TX driver and configure output pins.
 * Uses legacy driver/i2s.h API (ESP-IDF 5.x still supports it).
 * tx_desc_auto_clear=true prevents DMA underrun from producing noise.
 */
esp_err_t audio_init(void)
{
    i2s_config_t i2s_cfg = {
        .mode                 = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate          = I2S_SAMPLE_RATE_HZ,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count        = I2S_DMA_BUF_COUNT,
        .dma_buf_len          = I2S_DMA_BUF_LEN_SMPLS,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .intr_alloc_flags     = 0,
    };

    esp_err_t ret = i2s_driver_install(I2S_PORT_NUM, &i2s_cfg, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2s_pin_config_t pin_cfg = {
        .bck_io_num   = I2S_BCLK_IO,
        .ws_io_num    = I2S_WS_IO,
        .data_out_num = I2S_DOUT_IO,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };

    ret = i2s_set_pin(I2S_PORT_NUM, &pin_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2S driver installed — BCLK=%d WS=%d DOUT=%d",
             I2S_BCLK_IO, I2S_WS_IO, I2S_DOUT_IO);
    return ESP_OK;
}

/*
 * Generate and play a pure sine wave tone via I2S.
 *
 * 10ms amplitude fade-in and fade-out envelope prevents speaker click/pop.
 * Samples are written in chunks of I2S_DMA_BUF_LEN_SMPLS to match DMA buffer.
 * Phase is continuous across chunks via the global sample index.
 */
void audio_play_tone(uint16_t freq_hz, uint16_t dur_ms, float volume)
{
    int total_samples = (I2S_SAMPLE_RATE_HZ * dur_ms) / 1000;
    int fade_samples  = (I2S_SAMPLE_RATE_HZ * TONE_FADE_MS) / 1000;
    int16_t buf[I2S_DMA_BUF_LEN_SMPLS];
    size_t written;

    for (int i = 0; i < total_samples; i += I2S_DMA_BUF_LEN_SMPLS) {
        int chunk = total_samples - i;
        if (chunk > I2S_DMA_BUF_LEN_SMPLS) chunk = I2S_DMA_BUF_LEN_SMPLS;

        for (int j = 0; j < chunk; j++) {
            int idx = i + j;
            float t = (float)idx / (float)I2S_SAMPLE_RATE_HZ;

            float env = 1.0f;
            if (idx < fade_samples) {
                env = (float)idx / (float)fade_samples;
            } else if (idx > total_samples - fade_samples) {
                env = (float)(total_samples - idx) / (float)fade_samples;
            }

            buf[j] = (int16_t)(sinf(2.0f * M_PI * (float)freq_hz * t)
                               * 32767.0f * volume * env);
        }

        i2s_write(I2S_PORT_NUM, buf, (size_t)(chunk * (int)sizeof(int16_t)),
                  &written, pdMS_TO_TICKS(200));
    }
}

/*
 * Play a sequence of tones with silence gaps between them.
 * Blocking — returns when the last tone finishes.
 */
void audio_play_sequence(const tone_t *tones, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        audio_play_tone(tones[i].freq_hz, tones[i].dur_ms, tones[i].volume);
        if (tones[i].gap_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(tones[i].gap_ms));
        }
    }
}

/*
 * Play a raw PCM WAV file embedded in flash.
 * Skips the 44-byte WAV header, writes remaining PCM bytes directly to I2S.
 * Fully implemented in Phase 5 — stub here to satisfy linker.
 */
void audio_play_wav(const uint8_t *data, uint32_t len)
{
    (void)data; (void)len;
    ESP_LOGI(TAG, "audio_play_wav — implemented in Phase 5");
}

/*
 * FreeRTOS audio player task — blocks on audio queue, dispatches commands.
 * Fully implemented in Phase 5 — stub here to satisfy linker.
 */
void task_audio(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "task_audio — implemented in Phase 5");
    vTaskDelete(NULL);
}
