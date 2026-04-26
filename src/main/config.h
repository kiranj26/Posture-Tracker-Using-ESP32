#pragma once

// ── I2C (MPU-6050) ────────────────────────────────────────────────────────
#define I2C_MASTER_PORT         I2C_NUM_0
#define I2C_MASTER_SDA_IO       21
#define I2C_MASTER_SCL_IO       22
#define I2C_MASTER_FREQ_HZ      400000      // 400kHz Fast Mode
#define I2C_TIMEOUT_MS          100

// ── MPU-6050 registers ────────────────────────────────────────────────────
#define MPU6050_ADDR            0x68
#define MPU_REG_PWR_MGMT_1      0x6B
#define MPU_REG_WHO_AM_I        0x75
#define MPU_REG_SMPLRT_DIV      0x19
#define MPU_REG_CONFIG          0x1A
#define MPU_REG_ACCEL_CONFIG    0x1C
#define MPU_REG_GYRO_CONFIG     0x1B
#define MPU_REG_ACCEL_XOUT_H    0x3B
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
#define IMU_SAMPLE_RATE_MS      100         // Task A loop delay (ms)
#define LPF_ALPHA               0.15f       // EMA coefficient — lower = smoother

// ── Calibration ───────────────────────────────────────────────────────────
#define CAL_TOTAL_SAMPLES       30          // 3 seconds at 100ms per sample
#define CAL_DISCARD_SAMPLES     5           // Discard first 500ms (filter settle)
#define CAL_MAX_STDDEV_DEG      5.0f        // Reject cal if stddev exceeds this
#define CAL_VALID_SAMPLES       (CAL_TOTAL_SAMPLES - CAL_DISCARD_SAMPLES)

// ── Posture thresholds ────────────────────────────────────────────────────
#define WARN_THRESH_DEG         10.0f       // ° from baseline → WARNING state
#define BAD_THRESH_DEG          20.0f       // ° from baseline → BAD state
#define JUMP_FILTER_DEG         60.0f       // ° — discard violent movement sample

// ── State machine timings ─────────────────────────────────────────────────
#define WARN_GRACE_MS           3000        // ms above WARN_THRESH before alert fires
#define BAD_GRACE_MS            5000        // ms above BAD_THRESH before BAD fires
#define RESET_COOLDOWN_MS       2000        // ms in RESET before returning to GOOD
#define REPEAT_ALERT_MS         30000       // ms between repeated BAD alerts
#define ESCALATION_CAP          3           // max escalation cycles before silence
#define GOOD_STREAK_REWARD_MS   300000      // ms of GOOD before reward chime (5 min)

// ── Audio volumes (0.0 – 1.0) ─────────────────────────────────────────────
#define VOL_WARNING             0.35f
#define VOL_BAD                 0.80f
#define VOL_CHIME               0.55f
#define VOL_VOICE               0.90f
#define VOL_BOOT                0.50f
#define TONE_FADE_MS            10          // Fade in/out ms — prevents speaker click

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
#define MUTEX_TIMEOUT_MS        5           // Max ms to wait for imu_mutex before skip
