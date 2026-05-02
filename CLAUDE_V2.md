# CLAUDE_V2.md — Posture Tracker V2 (Product Grade)

> **Purpose:** This is the single source of truth for V2 development on the `release/v2-product` branch.
> Read this entire file before writing, reviewing, or debugging any V2 code.
> V1 architecture lives in `CLAUDE.md`. Do not mix V1 and V2 assumptions.
> This branch is never merged to `main` without an explicit product release decision.

---

## Table of Contents

1. [V2 Vision & Scope](#1-v2-vision--scope)
2. [What Changes from V1](#2-what-changes-from-v1)
3. [Hardware — Component Decisions](#3-hardware--component-decisions)
4. [Full BOM](#4-full-bom)
5. [Pin Assignment Table](#5-pin-assignment-table)
6. [Power System Design](#6-power-system-design)
7. [Haptic System](#7-haptic-system)
8. [Audio System](#8-audio-system)
9. [Alert UX — Layered Haptic + Audio](#9-alert-ux--layered-haptic--audio)
10. [Firmware Architecture](#10-firmware-architecture)
11. [Module Specifications](#11-module-specifications)
12. [FreeRTOS Task Design](#12-freertos-task-design)
13. [Posture State Machine](#13-posture-state-machine)
14. [Configuration Reference](#14-configuration-reference)
15. [PCB Design Guidelines](#15-pcb-design-guidelines)
16. [Enclosure & Form Factor](#16-enclosure--form-factor)
17. [Development Phases](#17-development-phases)
18. [Branch Strategy](#18-branch-strategy)
19. [Coding Conventions](#19-coding-conventions)
20. [Build & Flash Commands](#20-build--flash-commands)
21. [Known Constraints & Hard Rules](#21-known-constraints--hard-rules)

---

## 1. V2 Vision & Scope

### What V2 is
A product-grade wearable shoulder-mounted posture tracker. Designed for all-day wear,
discreet alerts, and a form factor comparable to a car key fob. Built to a standard
where it could be manufactured at small volume (50–200 units) and sold commercially.

### What makes V2 different from V1
- **Haptic alerts** — LRA motor via DRV2605L for discreet, office-friendly notifications
- **Layered UX** — haptics for early alerts, audio only for escalation and voice clips
- **Custom PCB** — no breakout boards, no dev kit, all ICs soldered directly
- **Battery powered** — 350mAh LiPo, ~12–15 hour runtime with light sleep
- **Compact form factor** — target 45×35×16mm total assembled with clip
- **ESP32-S3** — native USB, BLE 5.0, more capable than V1 ESP32

### What V2 explicitly does NOT include (scope boundary)
- ❌ No mobile app in V2 firmware (BLE hardware is present, firmware hooks stubbed)
- ❌ No WiFi, no OTA updates
- ❌ No display
- ❌ No injection-molded enclosure — 3D printed clip only in V2
- ❌ No medical accuracy claims — behavioural nudge only
- ❌ No Arduino framework — pure ESP-IDF + C only

### Target competitive position
| Product | Price | Haptic | Audio | Standalone | Form factor |
|---|---|---|---|---|---|
| Upright Go 2 | $80 | ✅ | ❌ | ✅ | Adhesive patch |
| Lumo Lift | discontinued | ✅ | ❌ | ✅ | Clip |
| **Posture Tracker V2** | **$79–99** | **✅** | **✅ voice** | **✅** | **Shoulder clip** |

The voice escalation is the differentiator. Keep it.

---

## 2. What Changes from V1

| Area | V1 | V2 |
|---|---|---|
| MCU | ESP32 Mini Dev Kit breakout | ESP32-S3-MINI-1U stamp module |
| IMU | GY-521 breakout (MPU-6050) | MPU-6050 bare IC, direct on PCB |
| Haptic | None | DRV2605L + 10mm LRA motor |
| Audio amp | MAX98357A breakout | MAX98357A bare IC, direct on PCB |
| Speaker | 3W 4Ω (50mm, huge) | 20mm 8Ω 0.5W (CUI CMS-2009-SMT-TR) |
| Power | USB only, no battery | MCP73831 charger + AP2112K LDO + 350mAh LiPo |
| USB | Micro-USB on dev board | USB-C (GCT USB4135) with proper CC resistors |
| PCB | Breadboard + breakout boards | Custom 2-layer PCB, ~45×35mm |
| Enclosure | None | 3D printed shoulder clip |
| Firmware alerts | Audio only | Haptic (early) + Audio (escalation) |
| Sleep | Never sleeps | Light sleep between IMU samples |

---

## 3. Hardware — Component Decisions

### 3.1 MCU — ESP32-S3-MINI-1U

**Why:** Stamp module solders directly to PCB. Native USB-OTG means no CH340/CP2102
UART bridge needed — USB-C connects directly to D+/D- for programming and serial monitor.
BLE 5.0 is present for a future app without a hardware respin.

**Package:** LCC, 2.4GHz PCB antenna built in (variant U = IPEX connector — use
non-U variant if board layout allows the onboard antenna clearance keep-out zone).

**Key specs:**
- Dual-core Xtensa LX7 @ 240MHz
- 512KB SRAM, 8MB flash (module includes flash)
- BLE 5.0, WiFi 802.11b/g/n (WiFi disabled in firmware)
- 3.3V IO, 3.3V supply
- Module size: 15.6×12×2.4mm

**Flash:** No external flash needed — module includes 8MB.

**Programming:** Via USB-C directly. Hold BOOT button, press RESET, release BOOT.
ESP-IDF USB DFU handles the rest. No UART adapter needed.

---

### 3.2 IMU — MPU-6050 Bare IC

**Why not change:** V1 driver is written, tested, and verified on real hardware across
two mounting orientations. The bare IC is identical to the GY-521 breakout minus the
LDO and pull-up resistors — those move onto the custom PCB.

**Key differences from V1 (GY-521 breakout) to bare IC:**
- Add 4.7kΩ pull-up on SDA and SCL to 3.3V on the PCB
- Add 100nF bypass cap on VCC pin to GND
- AD0 tied to GND → address 0x68 (same as V1)
- INT pin broken out to GPIO but unused in V2 firmware (stubbed for V3 wake-on-motion)

**Package:** QFN-24, 4×4mm

---

### 3.3 Haptic Driver — DRV2605L

**Why:** I2C interface shares existing bus with MPU-6050. No new GPIO pins consumed.
I2C address 0x5A — no conflict with MPU-6050 at 0x68. Has 123 built-in waveform
patterns addressable by index — no need to generate waveforms in firmware. Handles
all LRA driving complexity (resonance tracking, overdrive, brake) automatically.

**Package:** WSON-12, 3×3mm

**Key connections:**
- SDA → GPIO8 (shared I2C bus)
- SCL → GPIO9 (shared I2C bus)
- IN/TRIG → not used (using I2C mode, not GPIO trigger mode)
- EN → 3.3V (always enabled)
- LRA+ / LRA- → motor pads

**I2C init sequence:**
```
Write 0x01 to register 0x01  (MODE: internal trigger, device out of standby)
Write 0x00 to register 0x1D  (LRA mode, open-loop)
Write 0x36 to register 0x1A  (LRA drive mode)
Write waveform_id to 0x04    (select pattern from library)
Write 0xFF to 0x05           (end of waveform sequence)
Write 0x01 to 0x0C           (GO — fire the waveform)
```

---

### 3.4 LRA Motor — Vybronics VG1030001XH ✅ Confirmed & Ordered 2026-04-26

**Part:** Vybronics VG1030001XH — Digikey 1670-VG1030001XH-ND
- 10mm diameter, Z-axis LRA
- 2VAC rated, 210Hz resonant frequency, 1.5G force
- Pre-attached wire leads — no soldering to contact pads required
- Active part, not discontinued — PCB V2 safe to design around
- Ordered qty 3: 1 for Phase 7 breadboard, 2 spares

**Why LRA over ERM coin motor:**
- ERM: crude sustained buzz, no pattern differentiation
- LRA: sharp discrete taps, physically distinct from a buzz
- DRV2605L is optimised for LRA — automatic resonance frequency tracking

**Mounting:** Adhesive pad on underside of PCB. Connects via two pads to DRV2605L
output. No polarity — LRA is an AC-driven coil.

**Key specs:**
- Diameter: 10mm, thickness: 2.7mm
- Rated voltage: 1.8Vrms
- Resonant frequency: ~175Hz (DRV2605L tracks this automatically in LRA mode)
- Max force: ~0.9G

---

### 3.5 Audio Amplifier — MAX98357A Bare IC

**Why keep:** V1 driver works. I2S timing verified. WAV sample rate switching works.
Moving from breakout to bare IC requires adding the minimal support components
(MODE resistor for mono channel selection, GAIN resistor for 9dB, 1µF bypass caps).

**Package:** WLP-16, 1.62×1.12mm (very small — requires reflow, not hand-soldering)

**Key differences from V1 (breakout) to bare IC:**
- SD_MODE pin: pull to 3.3V via 100kΩ (always on) or drive from GPIO for software mute
- GAIN pin: leave floating for 9dB, or set resistor divider for different gain
- Add 1µF + 100nF on PVDD, 100nF on DVDD
- Speaker output differential (OUT+ / OUT-) connects directly to speaker pads

**IMPORTANT:** SD_MODE pulled low = shutdown. SD_MODE floating = invalid state.
Always define its state explicitly — pull high for V2.

---

### 3.6 Speaker — 20mm 8Ω 0.5W

**Part:** CUI Devices CMS-2009-SMT-TR or PUI Audio AS02008AO-3-R

**Why 20mm:** Minimum diameter for intelligible voice clip playback. Below 15mm,
frequency response drops off below 1kHz — voice becomes unintelligible. 20mm gives
adequate response down to ~500Hz.

**Why 8Ω:** MAX98357A supports 4Ω and 8Ω. 8Ω at 0.5W is fine for alert volumes.
The lower power draw vs 4Ω helps battery life.

**Mounting:** Edge-mount on PCB with SMD spring tabs, or through-hole pins.
Speaker faces outward toward shoulder — sound bounces off clothing, still audible.

**Volume expectation:** Lower than V1 3W speaker. Acceptable — haptics handle
early alerts, audio only fires for escalation and voice clips at point where
user needs to hear it clearly regardless.

---

### 3.7 LiPo Charging IC — MCP73831T-2ACI/OT

**Why:** Industry standard single-cell LiPo charger. Simple to implement —
one IC, one resistor (sets charge current), one LED indicator, two caps.
PROG resistor for 100mA charge rate from USB: R = 1000V/Icharge = 10kΩ.

**Package:** SOT-23-5

**Charge current:** 100mA (conservative — protects the 350mAh cell, ~3.5 hour charge)

**Status output:** Open-drain STAT pin → LED to 3.3V
- Charging: LED on
- Done: LED off
- No battery: LED blinks

**IMPORTANT:** MCP73831 has no reverse-polarity protection. Battery connector
must be keyed (JST-PH 2-pin, 2mm pitch) to prevent backwards insertion.

---

### 3.8 LDO Regulator — AP2112K-3.3TRG1

**Why:** LiPo outputs 3.0–4.2V depending on charge state. All ICs need stable 3.3V.
AP2112K: 600mA max, dropout 250mV, tiny SOT-23-5 package.

**Headroom check:**
- LiPo minimum: 3.0V (cutoff)
- LDO dropout at 600mA: 250mV max
- Output at 3.0V input: 3.0 - 0.25 = 2.75V — too low
- **Fix:** Set LiPo cutoff at 3.3V in firmware low-battery detection
- At 3.3V LiPo: 3.3 - 0.25 = 3.05V → below 3.3V regulation → brownout

**Better fix:** Use AP2112K-3.3 with EN pin. When LiPo < 3.4V, log warning, play
low-battery beep, enter deep sleep. User charges before brownout occurs.

**Peak current check:**
- ESP32-S3 active: 100mA
- MPU-6050: 4mA
- MAX98357A peak (audio burst): 150mA
- DRV2605L + LRA: 80mA
- Total peak: ~334mA → well within 600mA rating ✅

---

### 3.9 USB-C Connector — GCT USB4135-GF-A

**Configuration:** USB 2.0 only (no USB 3.x lanes needed)

**CC pins:** Both CC1 and CC2 must have 5.1kΩ pull-down to GND.
Without these resistors, USB-C chargers assume a cable (not a device) is connected
and may not supply current. This is the single most common USB-C design mistake.

**D+ / D-:** Connect directly to ESP32-S3 USB OTG pins for native USB programming.
No CH340 or CP2102 needed.

**VBUS:** Routes to MCP73831 VBUS input for charging. Add 1A polyfuse in series
as reverse-current protection.

---

### 3.10 Buttons — Alps SKRPABE010

- 3.9×2.9mm footprint, 1.6mm height
- SMD, 160gf actuation force — positive click feel
- **SW1 (BOOT/CAL):** Dual function — hold at power on for ESP32 boot mode,
  single press during operation to trigger recalibration
- **SW2 (SNOOZE):** Single press = 10-minute alert snooze; long press = mute toggle

Both buttons: GPIO with internal pull-up enabled, active low.

---

## 4. Full BOM

| Ref | Component | Part Number | Package | Qty | Unit Cost | Notes |
|---|---|---|---|---|---|---|
| U1 | ESP32-S3-MINI-1U | ESP32-S3-MINI-1U-N8 | LCC | 1 | $2.50 | 8MB flash variant |
| U2 | IMU | MPU-6050 | QFN-24 | 1 | $0.30 | I2C addr 0x68 |
| U3 | Haptic driver | DRV2605LDGSR | WSON-12 | 1 | $1.20 | I2C addr 0x5A |
| U4 | Audio amp | MAX98357AEWL | WLP-16 | 1 | $1.50 | Needs reflow |
| U5 | LiPo charger | MCP73831T-2ACI/OT | SOT-23-5 | 1 | $0.25 | |
| U6 | LDO 3.3V | AP2112K-3.3TRG1 | SOT-23-5 | 1 | $0.15 | 600mA |
| M1 | LRA motor | Vybronics VG1030001XH | — | 1 | $2.96 | 10mm, LRA, 210Hz, wire leads — confirmed 2026-04-26 |
| LS1 | Speaker | MECCANIXITY 20mm 8Ω 1W | SMD | 1 | $2.12 | 20mm, 8Ω, 1W — confirmed 2026-04-26 |
| J1 | USB-C | GCT USB4135-GF-A | SMD | 1 | $0.40 | 2.0 only |
| J2 | Battery | JST-PH 2-pin | THT | 1 | $0.10 | Keyed, 2mm pitch |
| SW1 | Button BOOT/CAL | Alps SKRPABE010 | SMD | 1 | $0.10 | |
| SW2 | Button SNOOZE | Alps SKRPABE010 | SMD | 1 | $0.10 | |
| BT1 | LiPo 350mAh | Adafruit #2750 | — | 1 | $6.95 | PCM protected, JST-PH, 36×20×5.6mm — confirmed 2026-04-26 |
| R1,R2 | I2C pull-up 4.7kΩ | — | 0402 | 2 | $0.01 | SDA, SCL |
| R3 | MCP73831 PROG | 10kΩ | 0402 | 1 | $0.01 | Sets 100mA charge |
| R4,R5 | USB-C CC | 5.1kΩ | 0402 | 2 | $0.01 | Both CC pins |
| R6 | Charge LED | 1kΩ | 0402 | 1 | $0.01 | |
| C1–C4 | Bypass caps | 100nF | 0402 | 4 | $0.01 | Per IC VCC pin |
| C5,C6 | Bulk caps | 10µF | 0805 | 2 | $0.05 | LDO in/out |
| C7 | Audio bulk | 22µF | 0805 | 1 | $0.05 | MAX98357A PVDD |
| F1 | Polyfuse | 500mA | 0805 | 1 | $0.10 | VBUS protection |
| LED1 | Charge indicator | Green LED | 0402 | 1 | $0.02 | |
| — | Passives misc | — | 0402 | ~10 | $0.10 | |
| **TOTAL** | | | | | **~$11.30** | |

**Manufacturing cost estimate (JLCPCB, 50 units, assembled):**
- PCB fabrication: ~$0.80/unit
- SMT assembly: ~$4.00/unit
- Components (from JLCPCB parts library): ~$8.00/unit
- **Total per unit: ~$13–15**
- **Target retail: $79–99**

---

## 5. Pin Assignment Table

```
ESP32-S3-MINI-1U
──────────────────────────────────────────────────────────────
I2C BUS (MPU-6050 + DRV2605L share this bus)
  GPIO 8    →  SDA   (MPU-6050 pin SDA, DRV2605L pin SDA)
  GPIO 9    →  SCL   (MPU-6050 pin SCL, DRV2605L pin SCL)
  [4.7kΩ pull-up on SDA and SCL to 3.3V on PCB]
  [MPU-6050 AD0 → GND → address 0x68]
  [DRV2605L address fixed 0x5A — no conflict]

I2S BUS (MAX98357A)
  GPIO 26   →  BCLK  (MAX98357A BCLK)
  GPIO 25   →  WS    (MAX98357A LRC)
  GPIO 27   →  DOUT  (MAX98357A DIN)

BUTTONS
  GPIO 0    →  SW1 BOOT/CAL   (active low, internal pull-up, also ESP32 boot pin)
  GPIO 35   →  SW2 SNOOZE     (active low, internal pull-up)

POWER
  EN        →  AP2112K enable (tie HIGH — always on)
  VBAT_SENSE→  GPIO34 ADC1 (voltage divider on LiPo for battery monitoring)

STATUS LED
  GPIO 48   →  LED1 (charge status, active high)

RESERVED / FUTURE
  GPIO 4    →  MPU-6050 INT (not used in V2 firmware, break out on PCB)
  GPIO 5    →  DRV2605L EN  (tied HIGH in V2, break out for future control)

USB (native, direct to connector)
  GPIO 19   →  USB D-
  GPIO 20   →  USB D+
──────────────────────────────────────────────────────────────
```

---

## 6. Power System Design

### Power path

```
USB-C (5V) → Polyfuse (F1) → MCP73831 → LiPo (3.7V)
                                             ↓
                                        AP2112K-3.3
                                             ↓
                                          3.3V rail
                                    ┌──────┴──────┐
                               ESP32-S3       All ICs
```

### Battery life calculation

| State | Current draw | Time per cycle |
|---|---|---|
| ESP32-S3 active (IMU read) | 80mA | 5ms |
| Light sleep | 2mA | 95ms |
| Average per 100ms cycle (no alerts) | **5.9mA** | — |
| Audio burst (voice clip, ~3s) | 180mA | occasional |
| Haptic fire (tap, ~0.5s) | 88mA | occasional |

**350mAh ÷ 6mA average ≈ 58 hours theoretical**
**Real world (audio, haptic, occasional BLE): 12–15 hours**

### Low battery handling

```c
// Check every 60 seconds via ADC on GPIO34
// Voltage divider: LiPo → 100kΩ → GPIO34 → 100kΩ → GND
// ADC reads 0–3.3V → multiply by 2 for actual LiPo voltage

if (lipo_voltage < 3.5f) {
    // Play low battery beep pattern
    // Log warning
}
if (lipo_voltage < 3.3f) {
    // Play shutdown tone
    // esp_deep_sleep_start()
}
```

### Sleep implementation

Between IMU samples, enter light sleep:
```c
// In task_read_imu — replaces vTaskDelay()
esp_sleep_enable_timer_wakeup(IMU_SAMPLE_RATE_MS * 1000);
esp_light_sleep_start();
// I2C and I2S peripherals resume automatically on wake
```

---

## 7. Haptic System

### DRV2605L initialisation sequence

```c
// drv2605l_init() — called once in app_main before tasks launch
i2c_write_reg(DRV2605L_ADDR, 0x01, 0x00);   // Mode: internal trigger, standby off
i2c_write_reg(DRV2605L_ADDR, 0x1D, 0xA8);   // LRA open loop, rated voltage
i2c_write_reg(DRV2605L_ADDR, 0x03, 0x02);   // Library: LRA
```

### Waveform index mapping

The DRV2605L has 123 built-in waveforms. Selected waveforms for this product:

| Alert type | Waveform index | Feel |
|---|---|---|
| `HAPTIC_WARN_SINGLE` | 1 | Single sharp tap |
| `HAPTIC_WARN_DOUBLE` | 10 | Double tap |
| `HAPTIC_BAD_STRONG` | 14 | Strong double tap |
| `HAPTIC_ESCALATION` | 47 | Triple pulse |
| `HAPTIC_CORRECTION` | 56 | Long smooth buzz — reward feel |
| `HAPTIC_CAL_PROMPT` | 52 | Three short pulses |
| `HAPTIC_CAL_SUCCESS` | 58 | Rising confirmation |
| `HAPTIC_CAL_FAIL` | 38 | Descending rejection |

**Note:** Waveform indices are verified against the DRV2605L datasheet Table 2.
Always cross-reference before changing — incorrect indices produce no output, not
an error.

### Firing a waveform

```c
void haptic_play(uint8_t waveform_id)
{
    i2c_write_reg(DRV2605L_ADDR, 0x04, waveform_id);  // Waveform slot 0
    i2c_write_reg(DRV2605L_ADDR, 0x05, 0x00);          // End of sequence
    i2c_write_reg(DRV2605L_ADDR, 0x0C, 0x01);          // GO
}
```

`haptic_play()` is non-blocking — DRV2605L executes the waveform autonomously.
The audio task can fire haptic and immediately return to blocking on the queue.

---

## 8. Audio System

### What changes from V1

- Speaker is smaller (20mm, 0.5W) — lower max volume, adequate for escalation alerts
- MAX98357A is bare IC on PCB — same driver, same code
- SD_MODE pin now driven by GPIO48 for software mute capability
- Alert layer mapping changes — see Section 9

### What stays identical from V1

- `audio_init()` — identical
- `audio_play_tone()` — identical
- `audio_play_sequence()` — identical
- `audio_play_wav()` — identical (sample rate switching preserved)
- All WAV files — `sit_up.wav`, `thank_you_good_work.wav` — identical

### Software mute via SD_MODE

```c
#define AUDIO_SD_MODE_GPIO  48

void audio_mute(bool mute)
{
    gpio_set_level(AUDIO_SD_MODE_GPIO, mute ? 0 : 1);
}
```

Called by SW2 snooze button handler. Mutes speaker while haptics continue.

---

## 9. Alert UX — Layered Haptic + Audio

This is the core UX design for V2. Haptics handle discreet early alerts.
Audio escalates only when haptics have been ignored.

```
────────────────────────────────────────────────────────
 GOOD STATE
 Silent. No alerts. Streak timer running.
 After 30s good streak → HAPTIC_CORRECTION (reward pulse, no sound)

────────────────────────────────────────────────────────
 10° deviation, 3s sustained → enter WARNING

 WARNING ENTRY
   Haptic: HAPTIC_WARN_SINGLE — one sharp tap
   Audio:  none
   → Office-friendly. Colleague next to you notices nothing.

 WARNING SUSTAINED (no correction after 3s more)
   Haptic: HAPTIC_WARN_DOUBLE — double tap
   Audio:  none
   → Second chance, still discreet.

────────────────────────────────────────────────────────
 20° deviation, 5s sustained → enter BAD

 BAD ENTRY
   Haptic: HAPTIC_BAD_STRONG — strong double tap
   Audio:  double beep (first time audio fires)
   → Getting serious. Audio starts here.

 BAD +8s (escalation 2)
   Haptic: HAPTIC_ESCALATION — triple pulse
   Audio:  double beep

 BAD +16s (escalation 3)
   Haptic: HAPTIC_ESCALATION
   Audio:  double beep

 BAD +24s onwards (cap reached, voice loop)
   Haptic: HAPTIC_ESCALATION — every repeat
   Audio:  "Sit up straight" WAV — repeating every 8s
   → Cannot be missed.

────────────────────────────────────────────────────────
 Posture corrected → enter RESET

 RESET ENTRY
   Haptic: HAPTIC_CORRECTION — long smooth reward pulse
   Audio:  "Thank you good work" WAV
   → Satisfying physical + audio confirmation.

 After 2s cooldown → GOOD

────────────────────────────────────────────────────────
 SW2 SNOOZE pressed
   All audio muted for 10 minutes
   Haptics continue — silent session (meetings, etc.)
   LED blinks slowly to indicate snooze active
```

---

## 10. Firmware Architecture

### New vs changed vs unchanged from V1

```
UNCHANGED (copy from V1 branch, zero modification):
  mpu6050.c / mpu6050.h      — I2C driver, angle math, EMA filter
  audio.c / audio.h          — I2S driver, tone gen, WAV playback, audio task
  posture_fsm.c / posture_fsm.h — state machine (audio commands change, logic unchanged)
  config.h                   — most values unchanged, new ones added

NEW FILES:
  haptic.c / haptic.h        — DRV2605L I2C driver, waveform playback
  power.c / power.h          — Battery ADC read, low battery detection, sleep entry
  buttons.c / buttons.h      — Debounced button handler, long press detection

MODIFIED:
  main.c                     — Add haptic_init(), power_init(), button_init()
                               Add button event task
  audio.h                    — Add AUDIO_CMD_HAPTIC_* commands to enum
  task_audio() in audio.c    — Dispatch haptic commands to haptic_play()
  config.h                   — Add haptic, power, button constants
```

### Layered architecture (same rule as V1 — no upward calls)

```
┌─────────────────────────────────────────────────────────┐
│  Layer 4: Application Logic                             │
│  posture_fsm.c — unchanged state machine logic          │
├─────────────────────────────────────────────────────────┤
│  Layer 3: Task Coordination                             │
│  FreeRTOS mutex, event group, queue — unchanged         │
│  buttons.c — new, posts events to queue                 │
├─────────────────────────────────────────────────────────┤
│  Layer 2: Driver Abstraction                            │
│  mpu6050.c   audio.c   haptic.c   power.c              │
├─────────────────────────────────────────────────────────┤
│  Layer 1: Hardware / ESP-IDF                           │
│  driver/i2c.h  driver/i2s.h  driver/adc.h  gpio.h    │
└─────────────────────────────────────────────────────────┘
```

---

## 11. Module Specifications

### 11.1 haptic.h — Public API

```c
/**
 * @brief Initialise DRV2605L over I2C. Configure for LRA open-loop mode.
 *        I2C bus must already be initialised by mpu6050_init() before calling.
 * @return ESP_OK on success, ESP_FAIL if device not found on I2C bus.
 */
esp_err_t haptic_init(void);

/**
 * @brief Fire a haptic waveform from the DRV2605L built-in library.
 *        Non-blocking — DRV2605L executes autonomously after GO bit is set.
 * @param waveform_id  Waveform index 1–123 per DRV2605L datasheet Table 2.
 */
void haptic_play(uint8_t waveform_id);
```

### 11.2 power.h — Public API

```c
/**
 * @brief Initialise ADC for battery voltage monitoring on VBAT_SENSE_GPIO.
 *        Configures voltage divider reading. Call once in app_main.
 */
esp_err_t power_init(void);

/**
 * @brief Read current LiPo voltage.
 * @return Voltage in volts (e.g. 3.85f). Returns 0.0f on ADC error.
 */
float power_read_battery_voltage(void);

/**
 * @brief Check and handle low battery condition.
 *        Plays warning below LOW_BATTERY_WARN_V.
 *        Enters deep sleep below LOW_BATTERY_CUTOFF_V.
 *        Call periodically from a low-priority task, not from FSM.
 */
void power_check_battery(void);
```

### 11.3 buttons.h — Public API

```c
typedef enum {
    BTN_CAL_SHORT_PRESS,     // SW1 < 500ms
    BTN_CAL_LONG_PRESS,      // SW1 > 2000ms (factory reset / re-cal)
    BTN_SNOOZE_SHORT_PRESS,  // SW2 < 500ms (10 min snooze)
    BTN_SNOOZE_LONG_PRESS,   // SW2 > 2000ms (mute toggle)
} button_event_t;

/**
 * @brief Initialise GPIO inputs with debounce for SW1 and SW2.
 *        Posts button_event_t to g_button_queue on each detected press.
 */
esp_err_t buttons_init(void);

extern QueueHandle_t g_button_queue;
```

### 11.4 audio_cmd_e — Updated enum (additions only)

```c
typedef enum {
    // V1 commands — unchanged
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
    // V2 additions — haptic commands routed through audio task
    AUDIO_CMD_HAPTIC_WARN_SINGLE,
    AUDIO_CMD_HAPTIC_WARN_DOUBLE,
    AUDIO_CMD_HAPTIC_BAD_STRONG,
    AUDIO_CMD_HAPTIC_ESCALATION,
    AUDIO_CMD_HAPTIC_CORRECTION,
    AUDIO_CMD_HAPTIC_CAL_PROMPT,
    AUDIO_CMD_HAPTIC_CAL_SUCCESS,
    AUDIO_CMD_HAPTIC_CAL_FAIL,
    AUDIO_CMD_SNOOZE_ON,
    AUDIO_CMD_SNOOZE_OFF,
} audio_cmd_e;
```

---

## 12. FreeRTOS Task Design

### Tasks — V2 adds one task vs V1

| Task | Core | Priority | Stack | Purpose |
|---|---|---|---|---|
| task_read_imu | 0 | 5 | 4096 | IMU read + light sleep loop |
| task_posture_fsm | 0 | 4 | 4096 | State machine — unchanged |
| task_audio | 1 | 3 | 8192 | Audio + haptic dispatch |
| task_buttons | 0 | 2 | 2048 | Button debounce + event post |
| task_power | 0 | 1 | 2048 | Battery monitor every 60s |

### Shared state additions vs V1

```c
// V1 globals — unchanged
extern imu_data_t         g_imu_data;
extern SemaphoreHandle_t  g_imu_mutex;
extern EventGroupHandle_t g_evt_group;
extern QueueHandle_t      g_audio_queue;

// V2 additions
extern QueueHandle_t      g_button_queue;
extern bool               g_snooze_active;   // set by button task, read by audio task
```

---

## 13. Posture State Machine

**No changes to state machine logic from V1.**

The only change: `enqueue_audio()` calls in `posture_fsm.c` now send haptic commands
alongside or instead of audio commands depending on alert level.

Updated command mapping:

| Event | V1 command | V2 commands |
|---|---|---|
| WARNING entry | AUDIO_CMD_WARN_BEEP | AUDIO_CMD_HAPTIC_WARN_SINGLE |
| WARNING sustained | — | AUDIO_CMD_HAPTIC_WARN_DOUBLE |
| BAD entry | AUDIO_CMD_BAD_ALERT | AUDIO_CMD_HAPTIC_BAD_STRONG + AUDIO_CMD_BAD_ALERT |
| BAD escalation | AUDIO_CMD_BAD_ALERT | AUDIO_CMD_HAPTIC_ESCALATION + AUDIO_CMD_BAD_ALERT |
| Voice loop | AUDIO_CMD_VOICE_CLIP | AUDIO_CMD_HAPTIC_ESCALATION + AUDIO_CMD_VOICE_CLIP |
| Correction | AUDIO_CMD_CORRECTION_CONFIRM | AUDIO_CMD_HAPTIC_CORRECTION + AUDIO_CMD_CORRECTION_CONFIRM |
| Reward | AUDIO_CMD_REWARD_CHIME | AUDIO_CMD_HAPTIC_CORRECTION (no audio — silent reward) |

---

## 14. Configuration Reference

All values in `config.h`. No magic numbers in `.c` files.

```c
// ── Haptic ────────────────────────────────────────────────────────────────
#define DRV2605L_ADDR           0x5A
#define HAPTIC_WARN_SINGLE      1       // DRV2605L waveform index
#define HAPTIC_WARN_DOUBLE      10
#define HAPTIC_BAD_STRONG       14
#define HAPTIC_ESCALATION       47
#define HAPTIC_CORRECTION       56
#define HAPTIC_CAL_PROMPT       52
#define HAPTIC_CAL_SUCCESS      58
#define HAPTIC_CAL_FAIL         38

// ── Power ─────────────────────────────────────────────────────────────────
#define VBAT_SENSE_GPIO         34
#define VBAT_DIVIDER_RATIO      2.0f    // Two equal resistors: actual = adc * ratio
#define LOW_BATTERY_WARN_V      3.5f    // Play warning beep below this
#define LOW_BATTERY_CUTOFF_V    3.3f    // Enter deep sleep below this
#define BATTERY_CHECK_INTERVAL_MS 60000 // Check every 60s

// ── Buttons ───────────────────────────────────────────────────────────────
#define BTN_CAL_GPIO            0
#define BTN_SNOOZE_GPIO         35
#define BTN_DEBOUNCE_MS         50      // Ignore transitions < 50ms
#define BTN_LONG_PRESS_MS       2000    // Long press threshold

// ── Snooze ────────────────────────────────────────────────────────────────
#define SNOOZE_DURATION_MS      600000  // 10 minutes

// ── Audio (SD_MODE) ───────────────────────────────────────────────────────
#define AUDIO_SD_MODE_GPIO      48

// ── All V1 values preserved unchanged ────────────────────────────────────
#define WARN_THRESH_DEG         10.0f
#define BAD_THRESH_DEG          20.0f
#define JUMP_FILTER_DEG         60.0f
#define WARN_GRACE_MS           3000
#define BAD_GRACE_MS            5000
#define RESET_COOLDOWN_MS       2000
#define REPEAT_ALERT_MS         8000
#define ESCALATION_CAP          3
#define GOOD_STREAK_REWARD_MS   30000
#define LPF_ALPHA               0.15f
#define IMU_SAMPLE_RATE_MS      100
```

---

## 15. PCB Design Guidelines

### Board specifications
- **Dimensions:** 45×35mm target, 50×38mm maximum
- **Layers:** 2 (top copper + bottom copper)
- **Thickness:** 1.6mm standard
- **Min trace width:** 0.15mm (signal), 0.5mm (power), 1.0mm (battery/speaker)
- **Min clearance:** 0.15mm
- **Surface finish:** HASL or ENIG (ENIG preferred for fine-pitch QFN pads)
- **Solder mask:** Both sides, green
- **Manufacturer:** JLCPCB (5 units for prototyping, 50 units for first batch)

### Component placement rules

```
TOP SIDE:
  ESP32-S3-MINI-1U  — center-top, antenna edge must overhang PCB edge
                       or keep 3mm copper-free keep-out under antenna
  MPU-6050          — close to center, away from switching noise sources
  DRV2605L          — near I2C bus, away from speaker lines
  MAX98357A         — near speaker connector, short differential traces
  MCP73831          — near USB-C connector
  AP2112K           — near MCP73831 output, bulk cap close to output pin
  All bypass caps   — within 1mm of each IC's VCC pin, on same side

BOTTOM SIDE:
  LRA motor         — adhesive-mounted, pads on bottom for LRA connections
  Battery connector — edge of board, opposite USB-C
  No ICs on bottom  — keeps reflow simple, one-sided assembly

ROUTING PRIORITIES:
  1. I2S lines (BCLK, WS, DOUT) — short, parallel, same layer
  2. I2C lines (SDA, SCL)       — short, away from I2S
  3. Speaker differential pair  — keep together, away from signal lines
  4. USB D+/D-                  — matched length, 90Ω differential impedance
  5. Power traces               — wide, direct from LDO to each IC
```

### Antenna keep-out (critical)
ESP32-S3-MINI-1U has an onboard PCB antenna on the edge of the module.
No copper, no traces, no ground plane within 3mm below the antenna area.
Violating this will degrade WiFi/BLE range significantly.

---

## 16. Enclosure & Form Factor

### Target dimensions
```
PCB:      45×35×1.6mm
LiPo:     35×25×5mm  (behind PCB)
Assembly: 45×35×8mm  (PCB + battery sandwiched)
Clip:     45×38×16mm (adds 8mm for clip mechanism)
```

### Clip design
3D printed in PETG (flexible enough to clip, rigid enough to hold):
- Spring clip mechanism — clips onto shirt collar, bra strap, or epaulette
- PCB slides into a recessed pocket in the clip body
- Speaker grille on top face (sound exits upward toward ear)
- USB-C port accessible from bottom edge
- Buttons exposed on front face
- LRA motor contacts back face (against body for best haptic transmission)

### Mounting orientation
Sensor axis map is unchanged from V1. Same pitch/roll interpretation.
Calibration handles any mounting offset, including the ~45° shoulder angle.

---

## 17. Development Phases

### Phase 7 — Haptic Validation on Breadboard
**Goal:** Prove DRV2605L + LRA waveforms feel right before committing to PCB.

**Hardware needed:**
- Adafruit DRV2605L breakout (~$7.95)
- 10mm LRA motor (~$3 from Digikey/Mouser)
- Connect breakout to existing V1 breadboard on I2C bus

**What to build:**
- `haptic.c` driver
- Add AUDIO_CMD_HAPTIC_* to audio task
- Remap WARNING alerts to haptic, BAD to haptic + audio
- Verify all 8 waveform indices feel distinct and appropriate

**Pass criteria:**
- Single tap on WARNING — clearly felt, not startling
- Double tap on WARNING sustained — distinct from single
- Strong double on BAD entry — noticeably stronger
- Correction reward — smooth, satisfying, clearly positive
- All waveforms distinct from each other by feel alone

---

### Phase 8 — Speaker Swap Validation
**Goal:** Confirm 20mm speaker provides adequate volume for voice clips.

**Hardware needed:** 20mm 8Ω 0.5W speaker, wire to existing MAX98357A

**Pass criteria:**
- "Sit up straight" audible at 1 metre in a quiet room
- "Thank you good work" audible and intelligible
- Boot tone audible
- No distortion at max volume

---

### Phase 9 — Battery System Validation
**Goal:** Validate MCP73831 + AP2112K + LiPo circuit on a test PCB.

**What to build:**
- Small test PCB or use evaluation board
- Confirm charging current is correct
- Confirm LDO output is stable 3.3V across LiPo discharge range
- Confirm low-battery detection thresholds trigger correctly

**Pass criteria:**
- LiPo charges at 100mA from USB-C
- STAT LED indicates charging / done correctly
- 3.3V stable from 4.2V to 3.4V LiPo input
- Low battery warning fires at correct voltage
- Deep sleep entry fires at cutoff voltage

---

### Phase 10 — PCB V2 Design & Fabrication
**Goal:** First assembled custom PCB.

**Steps:**
1. Schematic capture (KiCad or EasyEDA)
2. PCB layout per Section 15 guidelines
3. DRC — zero errors
4. Gerber export + BOM + CPL for JLCPCB
5. Order 5 prototype units
6. Assemble, flash, verify all Phase 7–9 functionality

**Pass criteria:**
- All ICs detected on I2C bus
- I2S audio plays
- Haptics fire
- USB-C charges battery
- Buttons respond
- Size fits within clip dimensions

---

### Phase 11 — Full Integration & Enclosure
**Goal:** Complete wearable device, worn for a real work session.

**Pass criteria:**
- Worn for 4+ hours continuously
- Alerts feel appropriate in context (haptic in meeting, audio at desk)
- Battery lasts full session
- No crashes, no false positives
- Clip stays attached to clothing

---

## 18. Branch Strategy

```
main                        ← V1 MVP (phases 0-6, public, breadboard)
  └── release/v2-product    ← V2 base (this branch, all V2 work starts here)
        └── feature/haptics         (Phase 7 — haptic driver)
        └── feature/speaker-swap    (Phase 8 — speaker validation)
        └── feature/power-system    (Phase 9 — battery/charging)
        └── feature/pcb-v2          (Phase 10 — PCB design files)
        └── feature/enclosure       (Phase 11 — CAD files)
```

**Merge policy:**
- Feature branches merge into `release/v2-product` via PR, not directly to `main`
- `release/v2-product` → `main` only at a deliberate product release milestone
- All feature branches: descriptive commit messages, no `WIP` commits on PRs

---

## 19. Coding Conventions

All V1 conventions apply unchanged:
- C99/C11 only, no C++
- `snake_case` identifiers, `UPPER_SNAKE_CASE` constants
- No magic numbers — all values in `config.h`
- Error check all ESP-IDF return values
- Each `.c` file: `static const char *TAG = "MODULE_NAME";`

**Additional V2 conventions:**
- Haptic waveform indices must be `#define` in `config.h` with comment citing DRV2605L datasheet table row
- Battery voltages in constants with `_V` suffix (e.g. `LOW_BATTERY_WARN_V`)
- Button GPIO constants with `_GPIO` suffix
- Any stub for future BLE feature: comment `// BLE_STUB — implement in V3`

---

## 20. Build & Flash Commands

```bash
# Build
pio run

# Flash via USB-C native USB (ESP32-S3)
# Hold BOOT (SW1), press RESET, release BOOT, then:
pio run --target upload

# Serial monitor
pio device monitor --baud 115200

# Clean rebuild
pio run --target clean && pio run
```

**ESP32-S3 note:** Native USB DFU flashing may require first run to manually enter
bootloader mode. Subsequent flashes can use auto-reset if RTS/DTR are wired
(they are via USB OTG on ESP32-S3, handled automatically by esptool).

---

## 21. Known Constraints & Hard Rules

All V1 rules apply. Additional V2 rules:

1. **DRV2605L waveform indices are 1-indexed.** Index 0 = stop. Never send 0 as a
   waveform ID — it silently stops playback with no error.

2. **LRA motor connects to DRV2605L only.** Never connect LRA directly to a GPIO.
   The motor requires AC drive at its resonant frequency — GPIO square wave will
   damage the motor and produce no useful vibration.

3. **Battery connector must be keyed.** JST-PH with key. MCP73831 has no reverse-
   polarity protection. Reversed battery = immediate IC destruction.

4. **USB-C CC pins are mandatory.** Both CC1 and CC2 need 5.1kΩ to GND. Without
   them, USB-C chargers will not supply current. This is non-negotiable.

5. **ESP32-S3 antenna keep-out is mandatory.** No copper under the antenna area.
   Violating this is not caught by DRC — it must be checked manually.

6. **haptic_play() is non-blocking.** Do not add delays after calling it expecting
   the waveform to finish. If sequencing matters, use the DRV2605L waveform
   sequence registers (slots 0–7) to chain patterns, not application-level delays.

7. **Speaker impedance must be 8Ω.** MAX98357A configured for 8Ω in V2. Using 4Ω
   will overdrive the amplifier and cause thermal shutdown.

8. **g_snooze_active is read by audio task, written by button task.** Access must
   be atomic. Use `atomic_bool` or a FreeRTOS mutex — not a raw `bool`.

---

*Branch: release/v2-product*
*Created: 2026-04-26*
*V1 reference: CLAUDE.md on main branch*
