# Posture Tracker V2 — Product Grade Wearable

> **Branch:** `release/v2-product`  
> **Status:** In development  
> **V1 (MVP breadboard):** See [main branch](../../tree/main)

A product-grade evolution of the ESP32 posture tracker. V2 moves from a breadboard
prototype to a wearable device — custom PCB, LiPo battery, haptic motor, and a compact
shoulder-clip form factor small enough to wear all day.

**Hardware:** ESP32-S3-MINI-1U · MPU-6050 · DRV2605L + LRA · MAX98357A · 20mm speaker · 350mAh LiPo  
**Stack:** ESP-IDF 5.5.0 · PlatformIO · C only (no Arduino, no C++)  
**Target size:** 45×35×16mm (PCB + battery + clip)

---

## What's New in V2

| Feature | V1 | V2 |
|---|---|---|
| Alerts | Audio only | Haptic (early) + Audio (escalation) |
| MCU | ESP32 Dev Kit breakout | ESP32-S3-MINI-1U stamp module |
| Power | USB only | 350mAh LiPo — 12–15 hr battery life |
| PCB | Breadboard + breakouts | Custom 2-layer PCB, all ICs direct |
| Speaker | 3W 4Ω (50mm, large) | 0.5W 8Ω (20mm, compact) |
| USB | Micro-USB | USB-C with proper CC resistors |
| Buttons | None | 2 buttons — recalibrate + snooze |
| Form factor | ~120×80mm breadboard | 45×35×16mm shoulder clip |

---

## Alert UX — Layered Haptic + Audio

The key design principle: **haptics for early alerts, audio only for escalation.**

| Time / State | Haptic | Audio |
|---|---|---|
| 10° for 3s — WARNING entry | Single tap | Silent |
| WARNING sustained | Double tap | Silent |
| 20° for 5s — BAD entry | Strong double tap | Double beep |
| BAD +8s | Triple pulse | Double beep |
| BAD +16s | Triple pulse | Double beep |
| BAD +24s onwards | Triple pulse | "Sit up straight" — repeats every 8s |
| Posture corrected | Long reward pulse | "Thank you good work" |
| 30s good streak | Gentle pulse | Silent |

SW2 (snooze button) mutes all audio for 10 minutes while haptics continue —
for meetings, calls, or quiet environments.

---

## Hardware

### Component List

| Component | Part | Purpose |
|---|---|---|
| ESP32-S3-MINI-1U | Espressif | MCU — dual core, BLE 5.0, native USB |
| MPU-6050 | InvenSense | 6-axis IMU — pitch/roll angle sensing |
| DRV2605L | Texas Instruments | Haptic driver — 123 built-in LRA waveforms |
| Z10SC2B LRA | Jinlong | 10mm linear resonant actuator — sharp haptic taps |
| MAX98357A | Maxim | I2S Class D amplifier — voice clips + tones |
| CMS-2009-SMT-TR | CUI Devices | 20mm 8Ω 0.5W speaker |
| MCP73831 | Microchip | LiPo charging IC — 100mA from USB-C |
| AP2112K-3.3 | Diodes Inc | 600mA LDO regulator — 3.3V rail |
| 350mAh LiPo | Generic | Battery — 12–15 hour runtime |
| USB-C (GCT USB4135) | GCT | Charging + programming port |

### Pin Assignment

| Signal | GPIO | Notes |
|---|---|---|
| I2C SDA | 21 | MPU-6050 + DRV2605L shared |
| I2C SCL | 22 | MPU-6050 + DRV2605L shared |
| I2S BCLK | 26 | MAX98357A |
| I2S WS | 25 | MAX98357A |
| I2S DOUT | 27 | MAX98357A |
| SW1 BOOT/CAL | 0 | Recalibrate / ESP32 boot |
| SW2 SNOOZE | 35 | 10-min snooze / mute toggle |
| VBAT SENSE | 34 | ADC — battery voltage monitor |
| AUDIO MUTE | 48 | MAX98357A SD_MODE |

---

## Architecture

### Three FreeRTOS tasks (V1) + Two new tasks (V2)

| Task | Core | Priority | Purpose |
|---|---|---|---|
| task_read_imu | 0 | 5 | IMU read every 100ms with light sleep |
| task_posture_fsm | 0 | 4 | State machine — unchanged from V1 |
| task_audio | 1 | 3 | Audio + haptic dispatch |
| task_buttons | 0 | 2 | Debounced button events |
| task_power | 0 | 1 | Battery monitor every 60s |

### New modules vs V1

```
NEW:      haptic.c    — DRV2605L I2C driver, waveform playback (non-blocking)
NEW:      power.c     — battery ADC, low-battery warning, deep sleep entry
NEW:      buttons.c   — debounced GPIO, short/long press detection

UNCHANGED: mpu6050.c  — I2C driver, atan2 angle math, EMA filter
UNCHANGED: posture_fsm.c — state machine logic (only alert commands change)
MODIFIED:  audio.c    — adds haptic command dispatch alongside audio
MODIFIED:  main.c     — init order adds haptic, power, buttons
```

---

## Power System

```
USB-C (5V) → Polyfuse → MCP73831 LiPo charger → 350mAh LiPo
                                                        ↓
                                                  AP2112K LDO
                                                        ↓
                                                    3.3V rail → all ICs
```

**Battery life estimate:**
- Active draw (IMU reading, no alerts): ~6mA average with light sleep
- 350mAh ÷ 6mA = ~58 hours theoretical
- Real world (audio bursts, haptic, occasional BLE): **12–15 hours**

**Low battery handling:**
- < 3.5V → warning beep + haptic
- < 3.3V → shutdown tone + deep sleep

---

## Form Factor

```
Assembled dimensions:
  PCB alone:        45 × 35 × 1.6mm
  PCB + battery:    45 × 35 × 8mm
  With clip:        45 × 38 × 16mm   ← final wearable size

Comparable to a car key fob.
Clips onto shirt collar, bra strap, or epaulette.
LRA motor faces the body for best haptic transmission.
Speaker grille faces outward.
USB-C and buttons accessible on edges.
```

---

## Development Phases

| Phase | Description | Status |
|---|---|---|
| 7 | Haptic validation on breadboard (DRV2605L + LRA) | ⏳ Next |
| 8 | Speaker swap validation (20mm, 0.5W) | ⏳ Pending |
| 9 | Battery system validation (MCP73831 + AP2112K) | ⏳ Pending |
| 10 | PCB V2 design + fabrication (JLCPCB) | ⏳ Pending |
| 11 | Full integration + enclosure (3D printed clip) | ⏳ Pending |

---

## Branch Structure

```
main                        ← V1 MVP (public, breadboard, complete)
  └── release/v2-product    ← this branch — all V2 work
        └── feature/haptics
        └── feature/speaker-swap
        └── feature/power-system
        └── feature/pcb-v2
        └── feature/enclosure
```

---

## Reference Docs

- [CLAUDE_V2.md](CLAUDE_V2.md) — full architecture, component decisions, hard rules
- [CLAUDE.md](CLAUDE.md) — V1 architecture reference
- [PHASES.md](PHASES.md) — V1 phase build guide (phases 0–6)
