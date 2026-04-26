# Posture Tracker — ESP32 + MPU-6050

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
| 2 | Angle computation + low-pass filter | 🔄 Next |
| 3 | I2S driver + speaker output | ⏳ Pending |
| 4 | Calibration system | ⏳ Pending |
| 5 | Full integration (FreeRTOS + state machine + audio) | ⏳ Pending |
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
