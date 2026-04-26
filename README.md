# Posture Tracker — ESP32 + MPU-6050

## Demo

https://github.com/user-attachments/assets/0a654031-a435-47b8-a605-cfffa6a3042a





A wearable shoulder-mounted posture tracker. Detects slouching in real time using an
IMU sensor and alerts the user through audio feedback.

**Hardware:** ESP32 Mini Dev Kit · GY-521 (MPU-6050) · Adafruit MAX98357A · 3W 4Ω speaker  
**Stack:** ESP-IDF 5.5.0 · PlatformIO · C only (no Arduino, no C++)

---

## Phase Status

| Phase | Description | Status |
|---|---|---|
| 0 | Toolchain verification + project scaffold | ✅ Complete |
| 1 | I2C driver + raw IMU data | ✅ Complete |
| 2 | Angle computation + low-pass filter | ✅ Complete |
| 3 | I2S driver + speaker output | ✅ Complete |
| 4 | Calibration system | ✅ Complete |
| 5 | Full integration (FreeRTOS + state machine + audio) | ✅ Complete |
| 6 | Endurance testing | ⏳ Pending |

---

## Hardware Wiring

### MPU-6050 (GY-521) → ESP32

| GY-521 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO 22 |
| SDA | GPIO 21 |

> XDA, XCL, AD0, INT — leave unconnected for MVP.
> GY-521 has onboard 4.7kΩ pull-ups on SDA and SCL — do NOT add external ones.

---

## Phase 1 — Sensor Axis Map (verified on real hardware, 2026-04-25)

This axis map was confirmed by physically tilting the sensor on a breadboard.
Raw values are at ±2g range (16384 LSB = 1g). Sensor was sitting at ~45° on
breadboard so az at rest reads ~10940 instead of 16384 — normal, calibration
handles this in Phase 4.

### Confirmed axis orientations

| Movement | Axis | Direction | Typical raw value |
|---|---|---|---|
| Flat on desk | az | positive | ~10940 |
| Tilt toward you — takeoff / nose up | ay | large positive | ~+7300 |
| Tilt away from you — landing / nose down | ay | large negative | ~-6800 |
| Tilt left | ax | large positive | ~+6500 |
| Tilt right | ax | large negative | ~-8000 |

### Quick reference

```
ax large +  →  tilted LEFT
ax large -  →  tilted RIGHT
ay large +  →  tilted TOWARD you  (takeoff / nose up)
ay large -  →  tilted AWAY        (landing / nose down)
az ~10000   →  sitting flat on breadboard
```

### Raw data sample — sensor at rest
```
I (57177) MAIN: ax=   276  ay=    20  az= 10926
I (57277) MAIN: ax=   276  ay=    48  az= 10888
I (57377) MAIN: ax=   300  ay=    44  az= 11006
```

### Raw data sample — tilted away / nose down (ay large negative)
```
I (22077) MAIN: ax=   366  ay= -6506  az=  9508
I (22177) MAIN: ax=   198  ay= -6502  az=  9494
I (22277) MAIN: ax=   248  ay= -6582  az=  9472
```

### Raw data sample — tilted toward / nose up (ay large positive)
```
I (30177) MAIN: ax=   706  ay=  7304  az=  9322
I (30277) MAIN: ax=   712  ay=  7338  az=  9300
I (30377) MAIN: ax=   446  ay=  7466  az=  9054
```

### Raw data sample — tilted left (ax large positive)
```
I (43177) MAIN: ax=  6442  ay=    32  az=  9802
I (43277) MAIN: ax=  6490  ay=    70  az=  9774
I (43377) MAIN: ax=  6540  ay=    70  az=  9646
```

### Raw data sample — tilted right (ax large negative)
```
I (52677) MAIN: ax= -7628  ay=    22  az=  8708
I (52777) MAIN: ax= -7726  ay=    10  az=  8618
I (52877) MAIN: ax= -7832  ay=    12  az=  8566
```

---

## Phase 2 — Angle Output (verified on real hardware, 2026-04-25)

Sensor sitting at the same ~45° breadboard angle from Phase 1.
EMA filter (alpha=0.15) applied. Resting values are non-zero by design —
Phase 4 calibration will capture these as baseline.

### Resting values (sensor still on breadboard)
```
pitch ≈ +1.3°  roll ≈ +0.2°   (fluctuation < ±0.3°)
```

### Verified angle ranges

| Movement | Axis | Direction | Observed range | Pass threshold |
|---|---|---|---|---|
| Tilt left | pitch | large positive | up to **+86°** | >10° |
| Tilt right | pitch | large negative | down to **-87°** | >10° |
| Nose up (takeoff) | roll | large positive | up to **+176°** | >10° |
| Nose down (landing) | roll | large negative | down to **-86°** | >10° |

### Quick reference
```
pitch large +  →  tilted LEFT
pitch large -  →  tilted RIGHT
roll  large +  →  nose up  (takeoff)
roll  large -  →  nose down (landing)
```

### Filter behaviour
- Convergence after tilt release: ~10–15 samples (1–1.5 seconds) ✅
- At-rest noise floor: < ±0.3° ✅

### Raw data sample — sensor at rest
```
I (9591) MAIN: pitch=   1.65  roll=   0.47
I (9691) MAIN: pitch=   1.73  roll=   0.26
I (9791) MAIN: pitch=   1.72  roll=   0.25
```

### Raw data sample — tilt left (pitch large positive)
```
I (16591) MAIN: pitch=  37.51  roll=   0.07
I (16691) MAIN: pitch=  39.49  roll=   0.15
I (16791) MAIN: pitch=  41.74  roll=   0.26
```

### Raw data sample — tilt right (pitch large negative)
```
I (26591) MAIN: pitch= -63.62  roll=   1.35
I (26691) MAIN: pitch= -66.73  roll=   1.79
I (26791) MAIN: pitch= -69.72  roll=   2.40
```

### Raw data sample — nose up / takeoff (roll large positive)
```
I (33591) MAIN: pitch=   1.06  roll=  34.53
I (33691) MAIN: pitch=   1.49  roll=  36.91
I (33791) MAIN: pitch=   1.86  roll=  39.22
```

### Raw data sample — nose down / landing (roll large negative)
```
I (43591) MAIN: pitch=   3.25  roll= -26.89
I (43691) MAIN: pitch=   3.77  roll= -30.55
I (43791) MAIN: pitch=   4.35  roll= -34.24
```

---

## Phase 5 — Full Integration (verified on real hardware, 2026-04-26)

Three FreeRTOS tasks running concurrently:
- **Task A (IMU, Core 0, prio 5):** reads angles every 100ms, writes to `g_imu_data` under mutex, sets event group bit
- **Task B (FSM, Core 0, prio 4):** blocks on event group, evaluates posture state machine, enqueues audio commands
- **Task C (Audio, Core 1, prio 3):** blocks on queue, dispatches tone sequences and WAV playback

### Escalation sequence when in BAD state

| Time in BAD | Audio |
|---|---|
| Entry | Double beep |
| +8s | Double beep (escalation 2) |
| +16s | Double beep (escalation 3) |
| +24s onwards | "Sit up straight" voice clip, repeating every 8s until corrected |

Correcting posture at any point → 2-note confirm tone → RESET → GOOD.

### Key implementation notes
- WAV playback switches I2S clock to 16kHz then back to 44.1kHz — prevents chipmunk effect
- WAV embedded as C array (`sit_up_wav.c`) — `EMBED_FILES` not supported by PlatformIO ESP-IDF build
- Calibration validates pitch stddev only — roll singularity at vertical mounting (gz≈0) excluded
- FSM never calls `vTaskDelay()` — all timing via `esp_timer_get_time()`

---

## Phase 4 — Calibration System (verified on real hardware, 2026-04-26)

### Key design decision — pitch-only stddev validation
`roll = atan2(gy, gz)` is numerically unstable when the sensor is vertical
(gz ≈ 0), e.g. mounted on a human back. Even with sensor perfectly still,
roll stddev reads ~11° at 90° mounting due to formula singularity. Pitch
`= atan2(gx, sqrt(gy²+gz²))` has no singularity — stable at every orientation.
Calibration validity check uses pitch stddev only.

### Pass criteria met — verified in both mounting orientations

| Orientation | Cal pitch_stddev | Result |
|---|---|---|
| Flat on desk | 0.38° | ✅ Accepted |
| Vertical (like human back, roll≈90°) | 0.93° | ✅ Accepted |

- Prompt tones → 3s collection → success arpeggio → baseline locked ✅
- Deliberate movement during calibration → fail tone → auto-retry ✅
- Baseline consistent across repeated calibrations ✅

---

## Phase 3 — I2S + Speaker Output (verified on real hardware, 2026-04-26)

MAX98357A wired to ESP32: BCLK→GPIO26, LRC→GPIO25, DIN→GPIO27, VIN→5V.

### Pass criteria met
- Audible 500Hz boot tone at every power-on — clean, no pop, no distortion ✅
- No I2S error codes in serial log ✅
- IMU angle loop continues normally after tone finishes ✅

### Wiring reference

| MAX98357A Pin | ESP32 Pin |
|---|---|
| VIN | 5V |
| GND | GND |
| BCLK | GPIO 26 |
| LRC (WS) | GPIO 25 |
| DIN | GPIO 27 |
| GAIN | leave floating (9dB) |
| SD | leave floating (always on) |

---

## Build & Flash

```bash
# Build
pio run

# Flash
pio run --target upload

# Serial monitor
pio device monitor --baud 115200

# Clean rebuild (after sdkconfig changes)
pio run --target clean && pio run
```

---

## Known Setup Issues (solved)

### ESP-IDF 5.x bootloader overlap
ESP-IDF 5.5.0 bootloader is ~29KB. The default partition table offset `0x8000`
is only 28KB from the bootloader start — causes an overlap error at flash time.
**Fix:** `CONFIG_PARTITION_TABLE_OFFSET=0x9000` in `sdkconfig.defaults`.

### Flash size mis-detection
PlatformIO auto-detects 2MB on first run. Board has 4MB.
**Fix:** `board_build.flash_size = 4MB` in `platformio.ini` and
`CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y` in `sdkconfig.defaults`.

---

## Reference Docs

- [CLAUDE.md](CLAUDE.md) — architecture, hardware specs, coding conventions
- [PHASES.md](PHASES.md) — phase-by-phase build guide with pass criteria
