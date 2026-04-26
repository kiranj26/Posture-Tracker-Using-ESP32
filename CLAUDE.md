# CLAUDE.md — Posture Tracker

> **Purpose of this file:** This is the single source of truth for any AI assistant (Claude or otherwise) helping with this project. Read this entire file before writing, reviewing, or debugging any code. Every architectural decision, hardware constraint, naming convention, and phase boundary is documented here. Do not deviate from what is written unless the human explicitly requests a change and this file is updated to match.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Hardware Inventory & Wiring](#2-hardware-inventory--wiring)
3. [Toolchain & Environment](#3-toolchain--environment)
4. [Project File Structure](#4-project-file-structure)
5. [Architecture & Design Decisions](#5-architecture--design-decisions)
6. [Configuration Reference (config.h)](#6-configuration-reference-configh)
7. [Module Specifications](#7-module-specifications)
8. [FreeRTOS Task Design](#8-freertos-task-design)
9. [Posture State Machine](#9-posture-state-machine)
10. [Audio System](#10-audio-system)
11. [Calibration System](#11-calibration-system)
12. [Error Handling Rules](#12-error-handling-rules)
13. [Development Phases](#13-development-phases)
14. [Coding Conventions](#14-coding-conventions)
15. [Build & Flash Commands](#15-build--flash-commands)
16. [Debugging & Serial Monitor](#16-debugging--serial-monitor)
17. [Known Constraints & Rules](#17-known-constraints--rules)

---

## 1. Project Overview

### What it is
A wearable shoulder-mounted posture tracker built on the ESP32 platform. It detects slouching and poor spinal alignment in real time using an IMU sensor (GY-521 / MPU-6050) and alerts the user through audio feedback (Adafruit MAX98357A I2S amplifier + 3W 4Ω speaker).

### What it is NOT (MVP scope boundary)
- ❌ No mobile app, no BLE data streaming
- ❌ No WiFi, no cloud connectivity, no OTA updates
- ❌ No battery management (USB-powered only for MVP)
- ❌ No physical enclosure or PCB in MVP
- ❌ No display, no buttons, no IR remote in MVP
- ❌ No medical accuracy claims — behavioural nudge tool only
- ❌ No Arduino framework — pure ESP-IDF + C only

### Core user flow
```
Power on → Calibration (3s, sit upright) → Monitoring loop (indefinite)
                                                     ↓
                              GOOD (silent) ←→ WARNING (soft beep) ←→ BAD (alert → voice escalation)
```

### Language & Framework
- **Language:** C (C99/C11 only — no C++ anywhere)
- **Framework:** ESP-IDF (via PlatformIO)
- **Build tool:** PlatformIO
- **Target board:** ESP32 Mini Dev Kit (esp32dev board target)

---

## 2. Hardware Inventory & Wiring

### Components

| Component | Identifier | Notes |
|---|---|---|
| ESP32 Mini Dev Kit | Main MCU | Dual-core 240MHz, 4MB flash, 3.3V logic |
| GY-521 (MPU-6050) | IMU sensor | From Elegoo 37-in-1 kit. I2C, address 0x68 |
| Adafruit MAX98357A | I2S amplifier | Class D mono, 3.2W max at 4Ω |
| Adafruit #3351 Speaker | Audio output | 3W, 4Ω — big for MVP, replace in V2 |
| Breadboard + jumper wires | Prototyping | From Elegoo kit, no soldering for MVP |
| USB cable | Power + flash | Laptop or power bank — no battery in MVP |

### Pin Assignment Table

```
ESP32 Mini Dev Kit
─────────────────────────────────────────────────────────
I2C BUS (MPU-6050 / GY-521)
  GPIO 21   →  SDA   (MPU-6050 pin: SDA)
  GPIO 22   →  SCL   (MPU-6050 pin: SCL)
  3.3V      →  VCC   (MPU-6050 pin: VCC)
  GND       →  GND   (MPU-6050 pin: GND)
  [AD0 on GY-521 left floating or tied to GND → I2C address 0x68]

I2S BUS (MAX98357A amplifier)
  GPIO 26   →  BCLK  (MAX98357A pin: BCLK)
  GPIO 25   →  WS    (MAX98357A pin: LRC)
  GPIO 27   →  DOUT  (MAX98357A pin: DIN)
  5V        →  VIN   (MAX98357A pin: VIN — needs 5V, not 3.3V)
  GND       →  GND   (MAX98357A pin: GND)
  [GAIN pin left floating → 9dB gain, sufficient for MVP]
  [SD pin left floating or tied HIGH → amplifier always on]

SPEAKER (wired to MAX98357A output terminals)
  MAX98357A OUT+  →  Speaker +
  MAX98357A OUT-  →  Speaker -
─────────────────────────────────────────────────────────
```

> **IMPORTANT:** The MAX98357A requires 5V on VIN. Use the 5V pin on the ESP32, not 3.3V. The I2S signal lines are 3.3V logic and are directly compatible with the MAX98357A.

> **IMPORTANT:** Pull-up resistors on I2C — the GY-521 module includes onboard 4.7kΩ pull-ups on SDA and SCL. Do NOT add additional external pull-ups; this will overdrive the bus.

### MPU-6050 Register Map (critical registers only)

| Register | Address | Write Value | Purpose |
|---|---|---|---|
| PWR_MGMT_1 | 0x6B | 0x00 | Wake device from sleep — MUST be first |
| WHO_AM_I | 0x75 | (read) expect 0x68 | Device presence verification |
| SMPLRT_DIV | 0x19 | 0x09 | Sets output rate: 1kHz / (1+9) = 100Hz |
| CONFIG | 0x1A | 0x04 | DLPF at ~21Hz bandwidth |
| ACCEL_CONFIG | 0x1C | 0x00 | ±2g range, 16384 LSB/g sensitivity |
| GYRO_CONFIG | 0x1B | 0x00 | ±250°/s range (gyro unused in MVP) |
| ACCEL_XOUT_H | 0x3B | (read 6 bytes) | Burst-read: AX_H, AX_L, AY_H, AY_L, AZ_H, AZ_L |

---

## 3. Toolchain & Environment

### Required installations
```bash
# PlatformIO CLI (or use VS Code PlatformIO extension)
pip install platformio

# Verify ESP-IDF toolchain is installed by PlatformIO automatically on first build
pio run --target clean
pio run
```

### platformio.ini (authoritative)
```ini
[env:esp32]
platform        = espressif32
board           = esp32dev
framework       = espidf
board_build.flash_size = 4MB
monitor_speed   = 115200
monitor_filters = esp32_exception_decoder

board_build.embed_files =
    src/main/voice/sit_up.wav

build_flags =
    -DLOG_LOCAL_LEVEL=ESP_LOG_DEBUG
    -Wno-unused-variable
    -Wno-unused-function
```

> **`board_build.flash_size = 4MB`** must be set explicitly. Without it, PlatformIO
> auto-detects 2MB on first run and generates a `sdkconfig.esp32` locked to 2MB,
> which causes a partition table overlap at flash time. See Phase 0 notes in PHASES.md.

### sdkconfig key settings
These live in `sdkconfig.defaults` — do NOT edit `sdkconfig` or `sdkconfig.esp32`
directly. Those files are auto-generated by the build from `sdkconfig.defaults`.
Commit `sdkconfig.defaults` and `sdkconfig.esp32`. Never commit `sdkconfig.old`.

```
CONFIG_FREERTOS_UNICORE=n              # Keep both cores active
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192   # app_main needs room for init
CONFIG_COMPILER_OPTIMIZATION_PERF=y   # Better float math speed (atan2f)
CONFIG_I2C_ISR_IRAM_SAFE=y            # Stable I2C under FreeRTOS scheduler
CONFIG_ESP_WIFI_ENABLED=n             # WiFi OFF — saves ~100KB RAM in MVP
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y      # Lock flash size — prevents 2MB mis-detection
CONFIG_PARTITION_TABLE_OFFSET=0x9000  # ESP-IDF 5.x bootloader is ~29KB, overruns
                                      # the default 0x8000 offset — move to 0x9000
```

> **Why `CONFIG_PARTITION_TABLE_OFFSET=0x9000`?** ESP-IDF 5.5.0 produces a
> bootloader of ~29,536 bytes. Starting at `0x1000`, it ends at `0x8360` — past the
> default partition table address of `0x8000`. Moving the table to `0x9000` gives
> ~3KB of headroom. The app offset moves automatically to `0x20000`.

> **If you need to reset sdkconfig:** delete `sdkconfig.esp32` and run
> `pio run --target clean && pio run`. It will regenerate cleanly from
> `sdkconfig.defaults`.

---

## 4. Project File Structure

```
posture-tracker/
├── CLAUDE.md                        ← THIS FILE — read before touching anything
├── PHASES.md                        ← Phase-by-phase build guide
├── platformio.ini                   ← PlatformIO project config (authoritative)
├── CMakeLists.txt                   ← Top-level ESP-IDF CMake entry
├── sdkconfig.defaults               ← Source-of-truth sdkconfig values (commit this)
├── sdkconfig.esp32                  ← Auto-generated by build from sdkconfig.defaults (commit this)
├── .gitignore                       ← Excludes .pio/, sdkconfig.old
└── src/
    └── main/
        ├── CMakeLists.txt           ← Registers sources, embeds WAV file
        ├── main.c                   ← app_main: boot sequence, init, calibrate, task launch
        ├── mpu6050.c                ← I2C driver, angle math, low-pass filter
        ├── mpu6050.h                ← Public API for IMU subsystem
        ├── audio.c                  ← I2S init, sine gen, WAV playback, audio task
        ├── audio.h                  ← Public API for audio subsystem
        ├── posture_fsm.c            ← State machine, threshold logic, timing
        ├── posture_fsm.h            ← Public API for FSM subsystem
        ├── config.h                 ← ALL compile-time constants — no .c pair
        └── voice/
            └── sit_up.wav           ← Embedded at compile time via EMBED_FILES
```

> **Note on sdkconfig files:**
> - `sdkconfig.defaults` — hand-written, version-controlled, source of truth
> - `sdkconfig.esp32` — auto-generated by `pio run`, version-controlled so the build
>   is reproducible on any machine without running menuconfig
> - Never commit `sdkconfig.old` — it is in `.gitignore`
> - If `sdkconfig.esp32` gets corrupted or stale, delete it and run `pio run` to regenerate

### src/main/CMakeLists.txt
```cmake
idf_component_register(
    SRCS
        "main.c"
        "mpu6050.c"
        "audio.c"
        "posture_fsm.c"
    INCLUDE_DIRS "."
    EMBED_FILES
        "voice/sit_up.wav"
)

target_link_libraries(${COMPONENT_LIB} PUBLIC m)
```

> **Why `-lm` (math lib)?** We use `atan2f()` and `sqrtf()` from `<math.h>`. The `m` target links the math library. Without it the build will fail with undefined reference to `atan2f`.

---

## 5. Architecture & Design Decisions

### Layered Architecture

```
┌─────────────────────────────────────────────┐
│  Layer 4: Application Logic                 │
│  posture_fsm.c — decisions, state, timing   │
├─────────────────────────────────────────────┤
│  Layer 3: Task Coordination                 │
│  FreeRTOS tasks, mutex, event group, queue  │
├─────────────────────────────────────────────┤
│  Layer 2: Driver Abstraction                │
│  mpu6050.c (I2C + math)  audio.c (I2S)     │
├─────────────────────────────────────────────┤
│  Layer 1: Hardware / ESP-IDF               │
│  driver/i2c.h  driver/i2s.h  nvs_flash.h  │
└─────────────────────────────────────────────┘
```

**Rule:** Upper layers may call lower layers. Lower layers must NEVER call up.
`posture_fsm.c` must never call `i2c_master_cmd_begin()` directly.
`mpu6050.c` must never call `xQueueSend()` directly.

### Why accel-only for MVP angle math?
- Accelerometer gives **absolute tilt** — it always knows where gravity is
- Gyroscope gives **rate of change** only — integrates over time → drifts
- For slow postural movements, accel alone is accurate enough
- V2 will add a complementary filter: `angle = 0.98*(angle + gyro*dt) + 0.02*accel_angle`

### Why FreeRTOS event group instead of polling?
- Task B (FSM) blocks on `xEventGroupWaitBits()` — burns **zero CPU** while waiting
- Task A sets the bit after every IMU write — instantly unblocks Task B
- Polling with `vTaskDelay` wastes cycles and adds unnecessary jitter

### Why non-blocking queue send from FSM to audio?
- Audio playback is slow (hundreds of ms). If the FSM blocked waiting for the queue, it would miss IMU samples during playback.
- `xQueueSend(..., 0)` = "send if space available, otherwise drop and continue"
- Queue depth of 5 ensures commands don't pile up. If full, the drop is acceptable.

### CPU Core Assignment
```
Core 0: Task A (IMU reader, priority 5) + Task B (FSM, priority 4)
Core 1: Task C (audio player, priority 3)
```
I2S DMA is managed by hardware and works better isolated to core 1. Keeping I2C and the state machine on core 0 avoids cross-core cache invalidation on the shared IMU struct.

---

## 6. Configuration Reference (config.h)

> **Rule:** Every value in this table must live in `config.h` as a named `#define`. No magic numbers in `.c` files. Period.

```c
#pragma once

// ── I2C (MPU-6050) ────────────────────────────────────────────────────────
#define I2C_MASTER_PORT         I2C_NUM_0
#define I2C_MASTER_SDA_IO       21
#define I2C_MASTER_SCL_IO       22
#define I2C_MASTER_FREQ_HZ      400000      // 400kHz Fast Mode
#define I2C_TIMEOUT_MS          100

// ── MPU-6050 registers ────────────────────────────────────────────────────
#define MPU6050_ADDR            0x68
#define MPU_REG_PWR_MGMT_1     0x6B
#define MPU_REG_WHO_AM_I        0x75
#define MPU_REG_SMPLRT_DIV     0x19
#define MPU_REG_CONFIG          0x1A
#define MPU_REG_ACCEL_CONFIG    0x1C
#define MPU_REG_GYRO_CONFIG     0x1B
#define MPU_REG_ACCEL_XOUT_H   0x3B
#define MPU_WHO_AM_I_RESPONSE   0x68
#define MPU_ACCEL_SENSITIVITY   16384.0f    // LSB/g for ±2g range

// ── I2S (MAX98357A) ───────────────────────────────────────────────────────
#define I2S_PORT_NUM            I2S_NUM_0
#define I2S_BCLK_IO             26
#define I2S_WS_IO               25
#define I2S_DOUT_IO             27
#define I2S_SAMPLE_RATE_HZ      44100
#define I2S_BITS_PER_SAMPLE     16
#define I2S_DMA_BUF_COUNT       8
#define I2S_DMA_BUF_LEN_SMPLS  512

// ── IMU sampling ─────────────────────────────────────────────────────────
#define IMU_SAMPLE_RATE_MS      100         // Task A loop delay
#define LPF_ALPHA               0.15f       // Exponential moving average coefficient

// ── Calibration ───────────────────────────────────────────────────────────
#define CAL_TOTAL_SAMPLES       300         // 3 seconds at 100ms per sample
#define CAL_DISCARD_SAMPLES     50          // Discard first 500ms (sensor settle)
#define CAL_MAX_STDDEV_DEG      5.0f        // Reject if user moved during cal
#define CAL_VALID_SAMPLES       (CAL_TOTAL_SAMPLES - CAL_DISCARD_SAMPLES)

// ── Posture thresholds ────────────────────────────────────────────────────
#define WARN_THRESH_DEG         10.0f       // ° deviation → enter WARNING
#define BAD_THRESH_DEG          20.0f       // ° deviation → enter BAD
#define JUMP_FILTER_DEG         60.0f       // ° deviation → discard sample (violent movement)

// ── State machine timings ─────────────────────────────────────────────────
#define WARN_GRACE_MS           3000        // ms sustained above WARN before WARNING fires
#define BAD_GRACE_MS            5000        // ms sustained above BAD before BAD fires
#define RESET_COOLDOWN_MS       2000        // ms in RESET before returning to GOOD
#define REPEAT_ALERT_MS         30000       // ms between repeated BAD alerts
#define ESCALATION_CAP          3           // Max escalation cycles before silence
#define GOOD_STREAK_REWARD_MS   300000      // ms of GOOD before reward chime (5 min)

// ── Audio volumes (0.0 – 1.0) ─────────────────────────────────────────────
#define VOL_WARNING             0.35f
#define VOL_BAD                 0.80f
#define VOL_CHIME               0.55f
#define VOL_VOICE               0.90f
#define VOL_BOOT                0.50f
#define TONE_FADE_MS            10          // Fade in/out duration to kill clicks

// ── FreeRTOS ─────────────────────────────────────────────────────────────
#define AUDIO_QUEUE_DEPTH       5
#define TASK_STACK_IMU          4096
#define TASK_STACK_FSM          4096
#define TASK_STACK_AUDIO        8192
#define TASK_PRIO_IMU           5
#define TASK_PRIO_FSM           4
#define TASK_PRIO_AUDIO         3
#define TASK_CORE_IMU           0
#define TASK_CORE_FSM           0
#define TASK_CORE_AUDIO         1
#define MUTEX_TIMEOUT_MS        5           // Max wait for imu_mutex before skipping
```

---

## 7. Module Specifications

### 7.1 mpu6050.h — Public API

```c
/**
 * @brief Initialise I2C master bus and configure MPU-6050.
 *        Wakes device, sets sample rate, DLPF, accel range.
 *        Verifies WHO_AM_I register.
 * @return ESP_OK on success, ESP_FAIL if device not found or I2C init fails.
 */
esp_err_t mpu6050_init(void);

/**
 * @brief Read current filtered pitch and roll angles.
 *        Performs I2C burst read, scales raw values, computes angles,
 *        applies exponential moving average filter in-place (stateful).
 * @param[out] pitch  Filtered pitch angle in degrees (forward/backward tilt)
 * @param[out] roll   Filtered roll angle in degrees (sideways tilt)
 * @return ESP_OK on success, ESP_FAIL on I2C error.
 */
esp_err_t mpu6050_read_angles(float *pitch, float *roll);

/**
 * @brief Reset the internal low-pass filter state to zero.
 *        Call at the start of calibration to flush stale filter state.
 */
void mpu6050_reset_filter(void);
```

**Internal (static, not in header):**
- `mpu6050_write_reg(uint8_t reg, uint8_t val)` — single register write
- `mpu6050_read_regs(uint8_t reg, uint8_t *buf, size_t len)` — burst read
- `prev_pitch`, `prev_roll` — static float state for EMA filter

### 7.2 audio.h — Public API

```c
/**
 * @brief Initialise I2S driver with configured port, pins, sample rate.
 * @return ESP_OK on success, ESP_FAIL if driver install fails.
 */
esp_err_t audio_init(void);

/**
 * @brief Generate and play a pure sine wave tone.
 *        Blocking — returns when tone finishes playing.
 *        Applies 10ms amplitude fade-in and fade-out envelope.
 * @param freq_hz   Tone frequency in Hz
 * @param dur_ms    Tone duration in milliseconds
 * @param volume    Volume scale 0.0 (silent) to 1.0 (full scale)
 */
void audio_play_tone(uint16_t freq_hz, uint16_t dur_ms, float volume);

/**
 * @brief Play a sequence of tones with gaps between them.
 * @param tones   Array of tone_t structs { freq_hz, dur_ms, volume, gap_ms }
 * @param count   Number of elements in the tones array
 */
void audio_play_sequence(const tone_t *tones, uint8_t count);

/**
 * @brief Play a raw PCM WAV file stored in flash.
 *        Skips the 44-byte WAV header. Writes remaining bytes as PCM to I2S.
 *        Blocking — returns when clip finishes.
 * @param data   Pointer to WAV file data (from embedded flash symbol)
 * @param len    Total byte length of WAV file (including header)
 */
void audio_play_wav(const uint8_t *data, uint32_t len);

/**
 * @brief FreeRTOS task entry point for the audio player.
 *        Blocks on audio_queue. Dispatches commands to playback functions.
 *        Pin to core 1 with stack TASK_STACK_AUDIO.
 */
void task_audio(void *arg);
```

**tone_t struct:**
```c
typedef struct {
    uint16_t freq_hz;
    uint16_t dur_ms;
    float    volume;
    uint16_t gap_ms;   // silence after this tone before next tone
} tone_t;
```

**Audio command enum:**
```c
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
```

### 7.3 posture_fsm.h — Public API

```c
/**
 * @brief Set the reference baseline angles.
 *        Called once by calibration after baseline is computed.
 * @param pitch  Baseline pitch in degrees
 * @param roll   Baseline roll in degrees
 */
void posture_fsm_set_baseline(float pitch, float roll);

/**
 * @brief FreeRTOS task entry point for the posture state machine.
 *        Blocks on IMU_DATA_READY_BIT event group bit.
 *        Evaluates thresholds, manages state transitions, enqueues audio commands.
 *        Pin to core 0 with stack TASK_STACK_FSM.
 */
void task_posture_fsm(void *arg);
```

### 7.4 main.c — Responsibilities

`app_main()` is the only entry point. It must do exactly these things in this order:

1. `nvs_flash_init()` — with erase-on-error retry
2. `mpu6050_init()` — halt with error tone if ESP_FAIL
3. `audio_init()` — halt with error tone if ESP_FAIL
4. Play boot-ready tone
5. Run `calibrate_baseline()` — blocks until accepted
6. Create `imu_mutex` — assert non-NULL
7. Create `evt_group` — assert non-NULL
8. Create `audio_queue` — assert non-NULL
9. `xTaskCreatePinnedToCore()` × 3 — assert all return pdPASS
10. Return from `app_main()` — FreeRTOS scheduler takes over

---

## 8. FreeRTOS Task Design

### Shared State

```c
// Shared between Task A (writer) and Task B (reader)
typedef struct {
    float   pitch;          // filtered pitch in degrees
    float   roll;           // filtered roll in degrees
    int64_t timestamp_us;   // esp_timer_get_time() at time of read
} imu_data_t;

// Globals declared in main.c, extern'd where needed
extern imu_data_t       g_imu_data;
extern SemaphoreHandle_t g_imu_mutex;
extern EventGroupHandle_t g_evt_group;
extern QueueHandle_t     g_audio_queue;

// Event group bit definitions
#define EVT_IMU_DATA_READY   (1 << 0)
```

### Task A — IMU Reader

```
Loop every IMU_SAMPLE_RATE_MS (100ms):
  1. Call mpu6050_read_angles(&pitch, &roll)
     → If ESP_FAIL: log warning, increment fail_count
       If fail_count >= 3: log error, call esp_restart()
  2. Take g_imu_mutex (timeout MUTEX_TIMEOUT_MS)
     → If take fails: log warning, skip this sample, continue loop
  3. Write pitch, roll, timestamp to g_imu_data
  4. Give g_imu_mutex
  5. xEventGroupSetBits(g_evt_group, EVT_IMU_DATA_READY)
  6. vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_RATE_MS))
```

### Task B — Posture FSM

```
Loop (blocking):
  1. xEventGroupWaitBits(g_evt_group, EVT_IMU_DATA_READY, pdTRUE, pdFALSE, portMAX_DELAY)
  2. Take g_imu_mutex (timeout MUTEX_TIMEOUT_MS)
     → If take fails: log warning, continue loop (wait for next bit)
  3. Copy g_imu_data to local imu_data_t snapshot
  4. Give g_imu_mutex immediately
  5. Compute delta = max(|pitch - baseline_pitch|, |roll - baseline_roll|)
  6. If delta > JUMP_FILTER_DEG: discard sample, continue (violent movement)
  7. Run state machine evaluation (see Section 9)
  8. If state transition requires audio: xQueueSend(g_audio_queue, &cmd, 0)
```

### Task C — Audio Player

```
Loop (blocking):
  1. xQueueReceive(g_audio_queue, &cmd, portMAX_DELAY)
  2. Switch on cmd.type → call appropriate playback function
  3. Return to step 1
```

---

## 9. Posture State Machine

### States
```
GOOD    → No deviation. Silent. Streak timer running.
WARNING → Deviation beyond WARN_THRESH_DEG for WARN_GRACE_MS. Soft alert once.
BAD     → Deviation beyond BAD_THRESH_DEG for BAD_GRACE_MS. Escalating alerts.
RESET   → Transitional cooldown after recovering from BAD. 2s duration.
```

### Transition Table

| From | To | Condition |
|---|---|---|
| GOOD | WARNING | `delta > WARN_THRESH_DEG` sustained for `WARN_GRACE_MS` |
| WARNING | GOOD | `delta < WARN_THRESH_DEG` (immediate, no timer) |
| WARNING | BAD | `delta > BAD_THRESH_DEG` sustained for `BAD_GRACE_MS` |
| BAD | RESET | `delta < WARN_THRESH_DEG` (immediate) |
| RESET | GOOD | `RESET_COOLDOWN_MS` elapsed since entering RESET |
| RESET | WARNING | `delta > WARN_THRESH_DEG` before cooldown expires |

### State Machine Variables (internal to posture_fsm.c)

```c
static posture_state_t  state            = STATE_GOOD;
static float            baseline_pitch   = 0.0f;
static float            baseline_roll    = 0.0f;
static int64_t          warn_start_ms    = 0;
static int64_t          bad_start_ms     = 0;
static int64_t          reset_start_ms   = 0;
static int64_t          good_streak_ms   = 0;
static int64_t          last_alert_ms    = 0;
static uint8_t          escalation_count = 0;
static bool             warn_fired       = false;  // Alert fires only once per WARNING entry
```

### Audio per state transition

| Transition / Event | Audio Command |
|---|---|
| GOOD → WARNING (first time) | `AUDIO_CMD_WARN_BEEP` |
| WARNING → BAD (entry) | `AUDIO_CMD_BAD_ALERT` |
| BAD, 30s elapsed, no correction | `AUDIO_CMD_BAD_ALERT` (repeat) |
| BAD, 60s elapsed, escalation_count < CAP | `AUDIO_CMD_VOICE_CLIP` |
| BAD → RESET | `AUDIO_CMD_CORRECTION_CONFIRM` |
| GOOD streak hits GOOD_STREAK_REWARD_MS | `AUDIO_CMD_REWARD_CHIME` |

---

## 10. Audio System

### Tone definitions (defined as const arrays in audio.c)

```c
// Boot ready: single medium tone
static const tone_t TONE_BOOT[]     = {{ 500, 200, VOL_BOOT, 0 }};

// Calibration prompt: ascending 3-note — "sit up straight now"
static const tone_t TONE_CAL_PROMPT[] = {
    { 400, 150, VOL_CHIME, 50 },
    { 600, 150, VOL_CHIME, 50 },
    { 800, 150, VOL_CHIME, 0  },
};

// Calibration success: C5 → E5 → G5 arpeggio
static const tone_t TONE_CAL_OK[] = {
    { 523, 100, VOL_CHIME, 30 },
    { 659, 100, VOL_CHIME, 30 },
    { 784, 150, VOL_CHIME, 0  },
};

// Calibration fail: descending 2-note
static const tone_t TONE_CAL_FAIL[] = {
    { 600, 150, VOL_CHIME, 50 },
    { 400, 200, VOL_CHIME, 0  },
};

// Warning beep: single soft beep
static const tone_t TONE_WARN[]    = {{ 700, 120, VOL_WARNING, 0 }};

// Bad alert: double sharp beep
static const tone_t TONE_BAD[]     = {
    { 1000, 200, VOL_BAD, 100 },
    { 1000, 200, VOL_BAD, 0   },
};

// Correction confirm: descending 2-note — "well done"
static const tone_t TONE_CONFIRM[] = {
    { 600, 100, VOL_CHIME, 40 },
    { 400, 120, VOL_CHIME, 0  },
};

// Good streak reward: G5 → E5 → C5 descending chime
static const tone_t TONE_REWARD[]  = {
    { 784, 100, VOL_CHIME, 30 },
    { 659, 100, VOL_CHIME, 30 },
    { 523, 150, VOL_CHIME, 0  },
};
```

### Sine wave generation algorithm

```c
void audio_play_tone(uint16_t freq_hz, uint16_t dur_ms, float volume) {
    int total_samples = (I2S_SAMPLE_RATE_HZ * dur_ms) / 1000;
    int fade_samples  = (I2S_SAMPLE_RATE_HZ * TONE_FADE_MS) / 1000;
    int16_t buf[I2S_DMA_BUF_LEN_SMPLS];
    size_t written;

    for (int i = 0; i < total_samples; i += I2S_DMA_BUF_LEN_SMPLS) {
        int chunk = MIN(I2S_DMA_BUF_LEN_SMPLS, total_samples - i);
        for (int j = 0; j < chunk; j++) {
            int idx = i + j;
            float t = (float)idx / I2S_SAMPLE_RATE_HZ;

            // Envelope: fade in and fade out
            float env = 1.0f;
            if (idx < fade_samples)
                env = (float)idx / fade_samples;
            else if (idx > total_samples - fade_samples)
                env = (float)(total_samples - idx) / fade_samples;

            buf[j] = (int16_t)(sinf(2.0f * M_PI * freq_hz * t) * 32767.0f * volume * env);
        }
        i2s_write(I2S_PORT_NUM, buf, chunk * sizeof(int16_t), &written, pdMS_TO_TICKS(200));
    }
}
```

### WAV file embedding

```
Prepare the voice clip:
  sox input.wav -r 16000 -c 1 -b 16 src/main/voice/sit_up.wav

Access in code (ESP-IDF embeds it as a flash symbol):
  extern const uint8_t sit_up_wav_start[] asm("_binary_sit_up_wav_start");
  extern const uint8_t sit_up_wav_end[]   asm("_binary_sit_up_wav_end");

Play it:
  uint32_t len = sit_up_wav_end - sit_up_wav_start;
  audio_play_wav(sit_up_wav_start, len);

audio_play_wav skips the 44-byte WAV header:
  const uint8_t *pcm = data + 44;
  uint32_t pcm_len   = len - 44;
  i2s_write(I2S_PORT_NUM, pcm, pcm_len, &written, pdMS_TO_TICKS(5000));
```

---

## 11. Calibration System

Calibration lives as a standalone function `calibrate_baseline()` in `main.c`. It is not a task — it runs synchronously during boot before any tasks are created.

### Algorithm

```
1. mpu6050_reset_filter()                     // Flush EMA state
2. audio_play_sequence(TONE_CAL_PROMPT, 3)    // "Sit straight"
3. vTaskDelay(pdMS_TO_TICKS(500))             // Brief pause after prompt

4. Collect CAL_TOTAL_SAMPLES samples:
   for i = 0 to CAL_TOTAL_SAMPLES - 1:
     mpu6050_read_angles(&p, &r)
     if i >= CAL_DISCARD_SAMPLES:
       accumulate pitch_sum, roll_sum
       accumulate pitch_sq_sum, roll_sq_sum  // for stddev
     vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_RATE_MS))

5. Compute mean:
   pitch_mean = pitch_sum / CAL_VALID_SAMPLES
   roll_mean  = roll_sum  / CAL_VALID_SAMPLES

6. Compute stddev:
   pitch_stddev = sqrtf(pitch_sq_sum/CAL_VALID_SAMPLES - pitch_mean^2)
   roll_stddev  = sqrtf(roll_sq_sum/CAL_VALID_SAMPLES  - roll_mean^2)
   max_stddev   = max(pitch_stddev, roll_stddev)

7. Validate:
   if max_stddev > CAL_MAX_STDDEV_DEG:
     audio_play_sequence(TONE_CAL_FAIL, 2)
     goto step 1                              // Retry — no limit in MVP

8. Accept:
   posture_fsm_set_baseline(pitch_mean, roll_mean)
   audio_play_sequence(TONE_CAL_OK, 3)
   ESP_LOGI(TAG, "Calibration OK: pitch=%.2f roll=%.2f stddev=%.2f",
            pitch_mean, roll_mean, max_stddev)
```

---

## 12. Error Handling Rules

| Condition | Response |
|---|---|
| `mpu6050_init()` returns `ESP_FAIL` | Log `FATAL`. Play 3 descending error tones. Call `abort()`. |
| `audio_init()` returns `ESP_FAIL` | Log `FATAL`. Cannot play tone. Call `abort()`. |
| `nvs_flash_init()` fails | `nvs_flash_erase()` then retry. If still fails, continue without NVS (log warning). |
| WHO_AM_I mismatch at boot | Log `FATAL: MPU-6050 not found (got 0xXX)`. Call `abort()`. |
| I2C read returns non-`ESP_OK` | Skip sample. Increment `consecutive_fail`. If >= 3, call `esp_restart()`. |
| Calibration stddev too high | Play fail tone. Automatically restart calibration. No limit on retries. |
| Mutex take timeout | Log `WARN`. Skip this sample/evaluation cycle. Do NOT block. |
| Audio queue full | Log `DEBUG: audio queue full, command dropped`. Continue. |
| Any `xTaskCreatePinnedToCore` returns `pdFAIL` | Log `FATAL`. Call `abort()`. |
| Any `xSemaphoreCreateMutex` returns `NULL` | Log `FATAL`. Call `abort()`. |

**Logging TAG convention:**
```c
// Each .c file defines its own TAG at the top
static const char *TAG = "POSTURE_FSM";   // or "MPU6050", "AUDIO", "MAIN"
```

---

## 13. Development Phases

Each phase has a strict entry condition (what must be true before starting) and a pass criterion (what must be demonstrated before moving to the next phase). **No skipping phases.**

---

### Phase 0 — Toolchain & Project Scaffold ✅ COMPLETE
**Goal:** Verify toolchain, scaffold all project files, confirm build and flash work on real hardware.

**Status:** Complete. See PHASES.md for full details, discovered issues, and fixes.

**What was built:**
- `platformio.ini`, root `CMakeLists.txt`, `sdkconfig.defaults`
- `src/main/CMakeLists.txt`, `config.h`, stub `main.c`, `mpu6050.c/.h`, `audio.c/.h`, `posture_fsm.c/.h`
- Placeholder `voice/sit_up.wav`

**Key fixes required:**
- `board_build.flash_size = 4MB` in `platformio.ini`
- `CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y` in `sdkconfig.defaults`
- `CONFIG_PARTITION_TABLE_OFFSET=0x9000` in `sdkconfig.defaults` — ESP-IDF 5.5 bootloader overruns the default `0x8000` offset

**Pass criterion met:**
- Build: zero errors, zero warnings
- Flash: succeeds, no overlap errors
- Serial: `I (672) MAIN: Posture Tracker — Phase 0 scaffold build OK`

---

### Phase 1 — I2C + Raw IMU Data
**Goal:** Confirm hardware is wired correctly and sensor is responding.

**Entry condition:** Hardware wired per Section 2. PlatformIO project compiles.

**What to build:**
- `mpu6050_init()` with I2C init, PWR_MGMT_1 wake, WHO_AM_I check
- `mpu6050_read_angles()` — raw bytes only, no math yet, print raw int16 values
- `app_main()` calls init and loops printing values to serial

**Pass criterion:**
- Serial monitor shows `WHO_AM_I: 0x68` at boot
- `ax`, `ay`, `az` raw int16 values print continuously
- Values visibly change when sensor is tilted by hand
- No I2C errors in 60 seconds of running

---

### Phase 2 — Angle Computation + Low-Pass Filter
**Goal:** Convert raw sensor data to usable pitch and roll angles.

**Entry condition:** Phase 1 passed.

**What to build:**
- Full `mpu6050_read_angles()` with atan2 math and EMA filter
- Print `pitch` and `roll` in degrees to serial

**Pass criterion:**
- Pitch changes by >10° when sensor is deliberately tilted forward
- Roll changes by >10° when sensor is tilted sideways
- Values stabilise within ~1 second of the sensor being held still
- Values are stable (< ±1° fluctuation) on a flat, still surface

---

### Phase 3 — I2S + Speaker Output
**Goal:** Confirm audio hardware works end-to-end.

**Entry condition:** Phase 2 passed. MAX98357A wired per Section 2.

**What to build:**
- `audio_init()` with I2S driver install and pin config
- `audio_play_tone()` with sine generation and fade envelope
- `app_main()` plays a 1000Hz test tone for 500ms on startup, then enters IMU loop

**Pass criterion:**
- Audible tone from speaker at boot — clean, no distortion, no pop
- No I2S error codes in serial log
- IMU angle printing continues normally after tone finishes

---

### Phase 4 — Calibration
**Goal:** Validate the full calibration flow works reliably.

**Entry condition:** Phase 3 passed.

**What to build:**
- `audio_play_sequence()` for multi-tone sequences
- `calibrate_baseline()` in `main.c`
- `posture_fsm_set_baseline()` in `posture_fsm.c`

**Pass criterion:**
- Calibration prompt tones play at boot
- Holding sensor still → success chime → baseline values logged to serial
- Moving sensor during calibration → failure tone → retry automatically
- Logged baseline pitch and roll match the known orientation of the device

---

### Phase 5 — State Machine + Audio Queue + Full Integration
**Goal:** Full working posture tracker.

**Entry condition:** Phase 4 passed.

**What to build:**
- FreeRTOS task infrastructure (mutex, event group, queue)
- `task_read_imu()`, `task_posture_fsm()`, `task_audio()` full implementations
- Full state machine in `posture_fsm.c`
- Voice WAV embedded and playing

**Pass criterion:**
- Deliberately holding device at >10° from baseline for 3s → warning beep
- Deliberately holding at >20° for 5s → double alert beep
- Returning to baseline → correction confirm tone
- Correct state logged to serial throughout
- No spurious alerts from desk vibration or typing simulation

---

### Phase 6 — Endurance Test ✅ COMPLETE
**Goal:** Confirm stability for real-world sessions.

**Entry condition:** Phase 5 passed.

**What to build:** No new code. Testing only.

**Test procedure:**
- Run device continuously for 60+ minutes
- Alternate between good posture (5-10 min blocks) and deliberate bad posture (1-2 min)
- Monitor serial output throughout

**Pass criterion:**
- No crashes, no resets, no stuck states
- No false positive alerts during normal desk activity
- All expected audio fires at correct times
- Serial log shows clean state transitions throughout

---

## 14. Coding Conventions

### General
- **Language:** C99/C11 only. No C++. No `.cpp` files.
- **Naming:** `snake_case` for all identifiers. No camelCase.
- **Constants:** `UPPER_SNAKE_CASE` via `#define` in `config.h`.
- **Types:** Use `uint8_t`, `int16_t`, `float`, `bool` from `<stdint.h>` and `<stdbool.h>`.
- **No magic numbers:** If a number isn't 0 or 1, it belongs in `config.h`.

### File structure
Every `.c` file must start with:
```c
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>              // only if needed
#include "freertos/FreeRTOS.h" // only if needed
#include "esp_log.h"
#include "own_header.h"

static const char *TAG = "MODULE_NAME";
```

### Logging
```c
ESP_LOGI(TAG, "Descriptive message: val=%.2f", val);   // Normal info
ESP_LOGW(TAG, "Warning: something unexpected");         // Recoverable issue
ESP_LOGE(TAG, "Error: cannot continue");                // Serious error
ESP_LOGD(TAG, "Debug: raw=%d", raw);                    // Verbose debug (compiled out in release)
```

### Comments
- Every function with >10 lines gets a block comment above it
- Every `#define` in `config.h` gets a comment explaining units and effect of changing it
- No commented-out code committed to the repo — delete it

### Error checking
```c
// ALWAYS check return values from ESP-IDF functions
esp_err_t ret = mpu6050_init();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "MPU-6050 init failed: %s", esp_err_to_name(ret));
    // handle error — do not silently continue
}
```

---

## 15. Build & Flash Commands

```bash
# Build only
pio run

# Build and flash
pio run --target upload

# Build, flash, and open serial monitor
pio run --target upload && pio device monitor

# Open serial monitor only (device already flashed)
pio device monitor --baud 115200

# Clean build (when changing sdkconfig or CMakeLists)
pio run --target clean
pio run

# Open ESP-IDF menuconfig (for sdkconfig changes)
pio run --target menuconfig

# Erase entire flash (use when flash state is suspect)
pio run --target erase

# Check available serial ports
pio device list
```

---

## 16. Debugging & Serial Monitor

### Serial output conventions
All serial output goes through ESP-IDF's `ESP_LOG*` macros. Do not use `printf()` directly except in `app_main()` before log system is configured.

### Useful debug patterns

**Print angles continuously (Phase 2 only — remove in Phase 5):**
```c
ESP_LOGI(TAG, "pitch=%.2f roll=%.2f", pitch, roll);
```

**Print state transitions:**
```c
ESP_LOGI(TAG, "STATE: %s -> %s | delta=%.2f",
         state_name(old_state), state_name(new_state), delta);
```

**Monitor stack usage (add during Phase 6 testing):**
```c
ESP_LOGI(TAG, "Stack watermark IMU=%d FSM=%d AUDIO=%d",
         uxTaskGetStackHighWaterMark(task_imu_handle),
         uxTaskGetStackHighWaterMark(task_fsm_handle),
         uxTaskGetStackHighWaterMark(task_audio_handle));
```

**Expected boot sequence in serial monitor:**
```
I (312)  MAIN:  NVS initialised
I (418)  MPU6050: WHO_AM_I: 0x68 — device confirmed
I (420)  MPU6050: Registers configured
I (421)  AUDIO:  I2S driver installed
I (422)  MAIN:  Starting calibration...
I (3842) MAIN:  Calibration OK: pitch=-2.14 roll=0.87 stddev=0.43
I (3843) MAIN:  All tasks launched. Entering monitoring loop.
I (3845) FSM:   STATE: GOOD (baseline locked)
```

---

## 17. Known Constraints & Rules

These are hard rules. Claude must not suggest violating them under any circumstances.

1. **No Arduino framework.** The project uses ESP-IDF via PlatformIO. `#include "Arduino.h"` is banned.
2. **No third-party libraries** for I2C, I2S, audio, or math. Only ESP-IDF built-ins and C standard library (`<math.h>`).
3. **No C++.** All source files are `.c`. No `new`, no classes, no templates.
4. **No magic numbers in `.c` files.** All tunable values live in `config.h`.
5. **No WiFi, no BLE.** The wireless stacks are disabled in sdkconfig to save RAM.
6. **No global variables accessed from multiple tasks without mutex protection.** `g_imu_data` is the only shared struct and is always accessed under `g_imu_mutex`.
7. **No blocking calls in the FSM task that are not the event group wait.** The FSM must never call `vTaskDelay()`.
8. **The audio task never reads IMU data directly.** It only receives `audio_cmd_t` structs from the queue.
9. **Cross-layer calls are banned.** `mpu6050.c` must not call FreeRTOS primitives. `posture_fsm.c` must not call I2C or I2S functions.
10. **`config.h` has no `.c` pair.** It is a pure header of `#define` constants. No `extern`, no function declarations.

---

*Last updated: 2026-04-25 — Phase 0 complete.*
*Next step: Begin Phase 1 — I2C + raw IMU data (real mpu6050_init + read_raw).*
