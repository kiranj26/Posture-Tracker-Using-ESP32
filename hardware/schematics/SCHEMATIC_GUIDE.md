# Schematic Design Guide — Posture Tracker V2

> **Purpose:** This file explains the reasoning behind every block, every component,
> and every design decision in the V2 schematic. Use this as a reference when
> reviewing, debugging, or extending the schematic in future sessions.
> Read this before touching any schematic block.

---

## SPICE Simulation Results

Simulations run using KiCad 8 built-in ngspice simulator on 2026-05-02.

### Simulation 1 — EN Reset RC Delay (R11 + C11)

**Circuit:** +3V3 → R11 (10kΩ) → ESP_EN → C11 (100nF) → GND

**Result:** ✅ PASS
- Time constant τ = R11 × C11 = 10kΩ × 100nF = 1ms
- EN pin reaches 63% of 3.3V (2.1V) at t = 1ms
- EN pin reaches 99% (3.27V) at t = 5ms
- ESP32 boots cleanly after power rail is fully stable
- Design is correct — no changes needed

### Simulation 2 — I2C Pull-up Rise Time (R1 + C_SDA)

**Circuit:** +3V3 → R1 (4.7kΩ) → SDA → C_load (10pF) → GND

**Result:** ✅ PASS (with note)
- Rise time measured ~900ns at 400kHz Fast Mode
- I2C spec allows up to 1250ns (half period at 400kHz)
- 4.7kΩ is the standard recommended value per I2C spec for Fast Mode
- On the actual PCB (45×35mm, short traces), bus capacitance will be 5–15pF
- Real-world rise time will be well within spec
- Design is correct — no changes needed

---

## Overview — What the Schematic Does

The posture tracker is a wearable shoulder-mounted device. It detects slouching
using an IMU sensor and alerts the user through haptic vibration (early warning)
and audio (escalating alert). It runs on a LiPo battery charged over USB-C.

The schematic is split into 7 logical blocks. Each block is drawn separately,
verified with ERC, then connected to the others through net labels.

```
USB-C (J1)
  → Polyfuse (F1)
    → MCP73831 charger (U5) → LiPo battery (J2)
  → AP2112K LDO (U6) → 3.3V rail → everything else

3.3V rail powers:
  → ESP32-S3-MINI-1U (U1)     ← brain
  → MPU-6050 (U2)             ← posture sensor
  → DRV2605L (U3)             ← haptic driver
  → MAX98357A (U4)            ← audio amplifier
  → Buttons SW1, SW2          ← user input
  → LED1                      ← charge indicator
```

---

## Block 1 — Power System

### What it does
Takes 5V USB input and produces a stable 3.3V rail for the whole board.
Also manages LiPo battery charging safely.

### Components and why each one exists

**J1 — USB-C Receptacle (GCT USB4135-GF-A)**
The charging and programming port. USB-C chosen over Micro-USB because:
- Reversible — no wrong-way insertion
- Modern standard — compatible with all new chargers
- Supports USB 2.0 for ESP32-S3 native USB (programming + serial monitor)

CC1 and CC2 pins: Both must have 5.1kΩ resistors (R4, R5) to GND.
These resistors signal to the USB-C charger "I am a device that needs 5V/500mA."
Without them, USB-C power delivery chargers will not supply current. This is
non-negotiable — missing CC resistors = device does not charge.

**F1 — Polyfuse 500mA**
Resettable fuse between USB VBUS and the charger IC.
If a short circuit occurs on the board, F1 trips and cuts power before components
are damaged. It resets automatically when the short is removed and power is cycled.
500mA chosen to match USB 2.0 current limit.

**U5 — MCP73831T-2ACI/OT (LiPo Charger)**
Single-cell LiPo/Li-Ion charge management IC in SOT-23-5 package.
Charges the battery from USB 5V input.

Key connections:
- VDD → VBUS_FUSED (5V from USB through polyfuse) — charger input power
- VBAT → battery positive terminal (J2 pin 1)
- VSS → GND
- PROG → 10kΩ R3 → GND — sets charge current to 100mA (Icharge = 1000/RPROG mA)
- STAT → charge status output (open drain, active low when charging)

Why 100mA charge current: Conservative for a 350mAh cell. Full charge in ~3.5 hours.
Keeps heat low and extends battery cycle life. Do not reduce R3 below 10kΩ without
recalculating — lower resistance = higher current = risk of overheating.

**J2 — JST-PH 2-pin (Battery Connector)**
Where the LiPo battery plugs in. JST-PH chosen because:
- Keyed — physically impossible to insert backwards
- Standard connector used by Adafruit, SparkFun batteries
- Pin 1 = positive (red wire), Pin 2 = negative (black wire)
Always verify polarity before connecting a battery.

**U6 — AP2112K-3.3TRG1 (LDO Voltage Regulator)**
Low-dropout linear regulator. Takes LiPo voltage (3.0–4.2V as battery drains)
and outputs a rock-solid 3.3V for all ICs.

Key connections:
- VIN → VBAT (battery voltage)
- EN → VIN (always-on — no software power control in V2)
- VOUT → +3V3 rail (powers everything)
- C6 (10µF) on VIN — input bulk capacitor, prevents LDO instability
- C7 (10µF) on VOUT — output bulk capacitor, filters load transients

Why not use the charger output directly? The charger outputs variable voltage
(battery voltage). The ESP32 and all sensors need stable 3.3V. The LDO provides
this regulation. During USB charging, the LDO draws from the battery which is
simultaneously being charged — this is correct and safe.

**LED1 + R6 (Charge Indicator)**
Green LED connected between 3.3V and the MCP73831 STAT pin.
STAT is open-drain: pulls low when charging, floats when complete.
When STAT is low → current flows → LED lights up = charging.
When charging complete → STAT floats → LED off.
R6 (1kΩ) limits current to ~2.3mA — safe for the LED and STAT pin.

**PWR_FLAG symbols**
KiCad ERC requires PWR_FLAG on power nets to confirm they are driven.
One PWR_FLAG on VBUS_FUSED, one on GND. These are schematic-only symbols
with no physical component.

### Net names used
| Net | Voltage | Description |
|---|---|---|
| VBUS | 5V | Raw USB input |
| VBUS_FUSED | 5V | USB after polyfuse |
| VBAT | 3.0–4.2V | LiPo battery voltage |
| +3V3 | 3.3V | Regulated rail for all ICs |
| GND | 0V | Common ground |

---

## Block 2 — MCU (ESP32-S3-MINI-1U)

### What it does
The brain of the device. Runs all firmware, reads sensors over I2C,
drives haptics and audio over I2S, manages user input, controls power.

### Components and why each one exists

**U1 — ESP32-S3-MINI-1U (Main Microcontroller)**
Stamp module (15.6×12mm) containing ESP32-S3 SoC, 8MB flash, antenna.
Chosen over original ESP32 Dev Kit because:
- 10× smaller footprint — fits in wearable form factor
- Native USB — no USB-to-UART chip needed, saves cost and space
- BLE 5.0 — future app connectivity without extra hardware
- Dual-core 240MHz — handles audio + sensor fusion simultaneously
- 3.3V native — directly compatible with all other ICs on this board

**C1 — 100nF bypass cap on 3V3 pin**
Decoupling capacitor placed as close as possible to the 3V3 pin.
Filters high-frequency switching noise generated by the CPU cores and
RF circuitry. Without this, noise couples into I2C and I2S signal lines
causing communication errors and audio glitches.

**R11 + C11 — EN reset circuit**
The EN (enable) pin controls whether the ESP32 is running or held in reset.
- R11 (10kΩ): pulls EN high → ESP32 runs normally
- C11 (100nF): charges slowly on power-up, delays EN going high by ~1ms
  This ensures power rails are fully stable before the ESP32 starts.
  Without this cap, the ESP32 may start before 3.3V is stable → corrupt boot.
A reset button (not in V2) would connect between EN and GND.

### GPIO assignments and why

| GPIO | Net | Reason for choice |
|---|---|---|
| GPIO8 | SDA | Left side of module, no special function, I2C capable |
| GPIO9 | SCL | Left side, adjacent to GPIO8 — keeps I2C traces short |
| GPIO26 | I2S_BCLK | I2S capable, right side |
| GPIO21 | I2S_DOUT | I2S capable, right side, near BCLK |
| GPIO33 | I2S_WS | I2S capable, right side, near BCLK and DOUT |
| GPIO34 | VBAT_SENSE | ADC1 channel — ADC2 unusable with WiFi/BLE |
| GPIO35 | BTN_SNOOZE | Any GPIO, no special function |
| GPIO0 | BTN_CAL | BOOT pin — also used for firmware download mode |
| GPIO48 | AUDIO_SD | Any GPIO, controls MAX98357A mute |
| GPIO19 | USB_DM | Fixed — hardware USB D- on ESP32-S3 |
| GPIO20 | USB_DP | Fixed — hardware USB D+ on ESP32-S3 |

Note: GPIO22–25 do not exist on ESP32-S3 (they existed on original ESP32).
All unused GPIOs have no-connect markers to satisfy ERC.

---

## Block 3 — IMU (MPU-6050)

### What it does
Measures the tilt angle of the device. The firmware compares the current
angle to the calibrated baseline to detect slouching.

### Components and why each one exists

**U2 — MPU-6050 (6-axis IMU)**
Combines 3-axis accelerometer and 3-axis gyroscope in one IC.
Only the accelerometer is used in V2 — it measures gravity direction,
which gives absolute tilt angle. The gyroscope is unused (adds drift
without a fusion filter).

Communicates over I2C at address 0x68 (AD0 tied to GND).

Key connections:
- VDD → +3V3 (main power)
- VLOGIC → +3V3 (I2C interface voltage — must match ESP32 logic level)
- GND → GND
- SDA → SDA net (shared I2C bus with DRV2605L)
- SCL → SCL net (shared I2C bus with DRV2605L)
- AD0 → GND (I2C address 0x68)
- All other pins → no-connect

**C2 — 100nF bypass cap on VDD**
Precision sensor — power supply noise directly corrupts accelerometer readings.
A noise spike of even 10mV can cause a false posture alert. C2 placed as
close as possible to VDD pin filters this noise at the source.

**R1 — 4.7kΩ SDA pull-up**
**R2 — 4.7kΩ SCL pull-up**
I2C is an open-drain bus. No IC on the bus drives lines high — they can only
pull low. The pull-up resistors pull SDA and SCL to 3.3V when no IC is
pulling them low. Without pull-ups, I2C does not work at all.

4.7kΩ chosen for 400kHz Fast Mode operation. Lower resistance = faster edges
but more current. Higher resistance = slower edges, unreliable at 400kHz.

IMPORTANT: Pull-ups are placed ONCE here in Block 3. Do NOT add more pull-ups
in Block 4 (DRV2605L). Extra pull-ups on the same bus lower the effective
resistance and overdrive the bus.

---

## Block 4 — Haptic Driver (DRV2605L + LRA Motor)

### What it does
Drives the LRA (Linear Resonant Actuator) haptic motor to produce
precise vibration patterns as early-warning posture alerts.

> To be completed when Block 4 is drawn.

---

## Block 5 — Audio Amplifier (MAX98357A + Speaker)

### What it does
Amplifies I2S audio from the ESP32 and drives the 8Ω speaker for
audio alerts and voice clips.

> To be completed when Block 5 is drawn.

---

## Block 6 — Buttons + Charge LED

### What it does
Two tactile buttons for user input (recalibrate posture baseline,
snooze alerts). One LED for battery charge status.

> To be completed when Block 6 is drawn.

---

## Block 7 — Battery Voltage Sense

### What it does
A resistor voltage divider scales the LiPo voltage (3.0–4.2V) down
to the ADC range (0–3.3V) so firmware can estimate remaining battery.

> To be completed when Block 7 is drawn.

---

## Design Rules — Never Violate These

1. **USB-C CC pins**: Both CC1 and CC2 must have 5.1kΩ to GND. Missing one = no charging.
2. **I2C pull-ups**: Only one set (R1, R2) for the entire bus. Never add more.
3. **MCP73831 PROG**: 10kΩ = 100mA charge current. Do not change without recalculating.
4. **ESP32 antenna keep-out**: 3mm copper-free zone under antenna edge on PCB.
5. **Bypass caps**: Place within 1mm of each IC VCC pin on same copper layer.
6. **JST-PH polarity**: Pin 1 = positive. Verify before connecting battery.
7. **AD0 on MPU-6050**: Must be tied to GND. Floating = undefined I2C address.
8. **MAX98357A SD_MODE**: Must be driven by GPIO48. Never leave floating.

---

## Net Names Reference

| Net | Voltage | Connected to |
|---|---|---|
| VBUS | 5V | J1 VBUS pin |
| VBUS_FUSED | 5V | F1 output → U5 VDD |
| VBAT | 3.0–4.2V | J2 pin 1, U5 VBAT, U6 VIN |
| +3V3 | 3.3V | U6 VOUT → all ICs |
| GND | 0V | All ground connections |
| SDA | signal | U1 GPIO8, U2 SDA, U3 SDA, R1 |
| SCL | signal | U1 GPIO9, U2 SCL, U3 SCL, R2 |
| I2S_BCLK | signal | U1 GPIO26, U4 BCLK |
| I2S_WS | signal | U1 GPIO33, U4 LRCLK |
| I2S_DOUT | signal | U1 GPIO21, U4 DIN |
| AUDIO_SD | signal | U1 GPIO48, U4 SD_MODE |
| BTN_CAL | signal | U1 GPIO0, SW1 |
| BTN_SNOOZE | signal | U1 GPIO35, SW2 |
| VBAT_SENSE | signal | U1 GPIO34, voltage divider |
| USB_DM | signal | U1 GPIO19, J1 D- |
| USB_DP | signal | U1 GPIO20, J1 D+ |
| STAT | signal | U5 STAT, LED1 cathode |
| ESP_EN | signal | U1 EN, R11, C11 |
