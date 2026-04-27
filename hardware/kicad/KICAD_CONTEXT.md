# KICAD_CONTEXT.md — PCB & Schematic Design Context

> **Branch:** `feature/pcb-schematic`
> **Purpose:** This file is the living context document for all KiCad schematic
> and PCB design work. Update this file every session — what was done, what decisions
> were made, what is left. Future Claude sessions must read this file first before
> giving any PCB or schematic advice.
> **KiCad version:** 8.x (download from kicad.org)

---

## Current Status

| Stage | Status | Notes |
|---|---|---|
| KiCad installed | ⬜ Not yet | Download from kicad.org |
| Project created | ⬜ Not yet | |
| Power block schematic | ⬜ Not started | |
| MCU block schematic | ⬜ Not started | |
| IMU block schematic | ⬜ Not started | |
| Haptic block schematic | ⬜ Not started | |
| Audio block schematic | ⬜ Not started | |
| Buttons + LED schematic | ⬜ Not started | |
| Battery sense schematic | ⬜ Not started | |
| Full schematic DRC | ⬜ Not started | |
| PCB layout started | ⬜ Not started | |
| PCB layout complete | ⬜ Not started | |
| PCB DRC clean | ⬜ Not started | |
| Gerbers exported | ⬜ Not started | |
| JLCPCB order placed | ⬜ Not started | |

---

## Design Decisions Log

> Add an entry here every time a significant design decision is made.
> Include the date, what was decided, and why.

| Date | Decision | Reason |
|---|---|---|
| 2026-04-26 | ESP32-S3-MINI-1U chosen over ESP32 devkit | Smaller, native USB, BLE 5.0 |
| 2026-04-26 | 8Ω speaker instead of 4Ω | Lower power draw, MAX98357A supports both |
| 2026-04-26 | LRA motor: Vybronics VG1030001XH | Active part, wire leads, 1.5G force |
| 2026-04-26 | MCP73831 charge current: 100mA | Conservative, protects 350mAh cell |
| 2026-04-26 | AP2112K-3.3 LDO | 600mA headroom, covers audio peak current |
| 2026-04-26 | Board size target: 45×35mm | Fits enclosure clip, smaller than credit card |

---

## Schematic Blocks — What to Draw

The schematic is split into 7 logical blocks. Draw one block at a time,
verify it, then move to the next. Do not mix blocks.

### Block 1 — Power System (draw first)
```
USB-C (J1)
  → CC1 pin: 5.1kΩ to GND
  → CC2 pin: 5.1kΩ to GND
  → VBUS: → Polyfuse F1 (500mA) → MCP73831 VIN

MCP73831 (U5)
  → VDD: 3.3V (from LDO)
  → VBAT: → JST-PH J2 pin 1 (battery +)
  → GND: → JST-PH J2 pin 2 (battery -)
  → PROG: → 10kΩ R3 → GND  (sets 100mA charge current)
  → STAT: → 1kΩ R6 → LED1 → 3.3V

AP2112K-3.3 (U6)
  → VIN: LiPo+ (from JST J2 pin 1)
  → EN: tie to VIN (always on)
  → GND: GND
  → VOUT: 3.3V rail
  → C_in: 10µF to GND (close to VIN pin)
  → C_out: 10µF to GND (close to VOUT pin)

Power flags:
  Add PWR_FLAG on 3.3V net and GND net (stops ERC errors)
```

### Block 2 — MCU (ESP32-S3-MINI-1U)
```
ESP32-S3-MINI-1U (U1)
  → 3V3: 3.3V rail + 100nF bypass cap to GND
  → GND: GND
  → GPIO21: SDA net
  → GPIO22: SCL net
  → GPIO25: I2S_WS net
  → GPIO26: I2S_BCLK net
  → GPIO27: I2S_DOUT net
  → GPIO0: BTN_CAL net (also BOOT pin)
  → GPIO35: BTN_SNOOZE net
  → GPIO34: VBAT_SENSE net
  → GPIO48: AUDIO_SD net
  → GPIO19: USB_DM net
  → GPIO20: USB_DP net
  → EN: 10kΩ pull-up to 3.3V + 100nF to GND (reset circuit)
  → All unused GPIO: leave unconnected (add no-connect marker)

Antenna keep-out note:
  Add a note on schematic: "3mm copper keep-out under antenna edge of U1"
```

### Block 3 — IMU (MPU-6050)
```
MPU-6050 (U2)
  → VCC: 3.3V + 100nF bypass cap to GND
  → GND: GND
  → SDA: SDA net (shared with DRV2605L)
  → SCL: SCL net (shared with DRV2605L)
  → AD0: GND (sets I2C address 0x68)
  → INT: test point only, no connection in V2
  → XDA, XCL, FSYNC: GND

I2C pull-ups (shared, place once in this block):
  → SDA net: 4.7kΩ R1 to 3.3V
  → SCL net: 4.7kΩ R2 to 3.3V
```

### Block 4 — Haptic (DRV2605L + LRA)
```
DRV2605L (U3)
  → VDD: 3.3V + 100nF bypass cap to GND
  → GND: GND
  → SDA: SDA net (shared with MPU-6050)
  → SCL: SCL net (shared with MPU-6050)
  → EN: 3.3V (always enabled)
  → IN/TRIG: GND (using I2C mode, not GPIO trigger)
  → LRA+: LRA motor pad 1
  → LRA-: LRA motor pad 2

LRA Motor (M1):
  → 2-pin connector or spring pads
  → No polarity (LRA is AC driven)
```

### Block 5 — Audio (MAX98357A + Speaker)
```
MAX98357A (U4)
  → PVDD: 3.3V + 22µF bulk cap + 100nF bypass cap to GND
  → DVDD: 3.3V + 100nF bypass cap to GND
  → GND: GND
  → BCLK: I2S_BCLK net
  → LRCLK: I2S_WS net
  → DIN: I2S_DOUT net
  → SD_MODE: AUDIO_SD net (GPIO48) — pull HIGH = on, pull LOW = off
  → GAIN: floating (9dB default)
  → OUT+: speaker pad 1
  → OUT-: speaker pad 2

Speaker (LS1):
  → 2-pin connector, 8Ω
```

### Block 6 — Buttons + LED
```
SW1 — BOOT/CAL button
  → One side: GND
  → Other side: BTN_CAL net → 10kΩ R7 pull-up to 3.3V
                            → 100nF C_debounce to GND
                            → GPIO0 (BOOT pin on ESP32-S3)

SW2 — SNOOZE button
  → One side: GND
  → Other side: BTN_SNOOZE net → 10kΩ R8 pull-up to 3.3V
                               → 100nF C_debounce to GND
                               → GPIO35

LED1 — Charge indicator
  → Anode: 3.3V via 1kΩ R6
  → Cathode: STAT pin of MCP73831
  (Already in Block 1 — just note it here)
```

### Block 7 — Battery Voltage Sense
```
Voltage divider on LiPo+ rail:
  LiPo+ → 100kΩ R9 → VBAT_SENSE net → GPIO34 (ADC1 CH6)
  VBAT_SENSE net → 100kΩ R10 → GND

  ADC reads 0–3.3V → multiply by 2 in firmware = actual LiPo voltage
  Add 100nF cap on VBAT_SENSE to GND for noise filtering
```

---

## Net Names (use exactly these in KiCad)

| Net name | Description |
|---|---|
| `+3V3` | 3.3V regulated rail |
| `VBAT` | LiPo battery voltage (3.0–4.2V) |
| `VBUS` | USB 5V input |
| `GND` | Ground |
| `SDA` | I2C data — shared MPU-6050 + DRV2605L |
| `SCL` | I2C clock — shared MPU-6050 + DRV2605L |
| `I2S_BCLK` | I2S bit clock to MAX98357A |
| `I2S_WS` | I2S word select to MAX98357A |
| `I2S_DOUT` | I2S data out to MAX98357A |
| `AUDIO_SD` | MAX98357A SD_MODE — software mute |
| `BTN_CAL` | Calibrate button signal |
| `BTN_SNOOZE` | Snooze button signal |
| `VBAT_SENSE` | Voltage divider output to ADC |
| `USB_DP` | USB D+ to ESP32-S3 |
| `USB_DM` | USB D- to ESP32-S3 |

---

## Component Reference Designators

| Ref | Component |
|---|---|
| U1 | ESP32-S3-MINI-1U |
| U2 | MPU-6050 |
| U3 | DRV2605L |
| U4 | MAX98357A |
| U5 | MCP73831 |
| U6 | AP2112K-3.3 |
| M1 | LRA motor (VG1030001XH) |
| LS1 | Speaker (20mm 8Ω) |
| BT1 | LiPo 350mAh |
| J1 | USB-C connector |
| J2 | JST-PH battery connector |
| SW1 | Button — BOOT/CAL |
| SW2 | Button — SNOOZE |
| LED1 | Charge indicator (green) |
| F1 | Polyfuse 500mA |
| R1 | 4.7kΩ — I2C SDA pull-up |
| R2 | 4.7kΩ — I2C SCL pull-up |
| R3 | 10kΩ — MCP73831 PROG (charge current) |
| R4 | 5.1kΩ — USB-C CC1 |
| R5 | 5.1kΩ — USB-C CC2 |
| R6 | 1kΩ — LED current limit |
| R7 | 10kΩ — SW1 pull-up |
| R8 | 10kΩ — SW2 pull-up |
| R9 | 100kΩ — VBAT sense divider top |
| R10 | 100kΩ — VBAT sense divider bottom |
| R11 | 10kΩ — ESP32 EN pull-up |
| C1 | 100nF — U1 bypass |
| C2 | 100nF — U2 bypass |
| C3 | 100nF — U3 bypass |
| C4 | 100nF — U4 DVDD bypass |
| C5 | 22µF — U4 PVDD bulk |
| C6 | 10µF — U6 LDO input bulk |
| C7 | 10µF — U6 LDO output bulk |
| C8 | 100nF — SW1 debounce |
| C9 | 100nF — SW2 debounce |
| C10 | 100nF — VBAT sense filter |
| C11 | 100nF — ESP32 EN filter |

---

## KiCad Libraries Needed

These symbols and footprints are not in KiCad's default library.
Must be downloaded or created manually.

| Component | Symbol source | Footprint source |
|---|---|---|
| ESP32-S3-MINI-1U | SnapEDA or create manually | SnapEDA or Espressif KiCad lib |
| MPU-6050 | KiCad default (Sensor lib) | KiCad default |
| DRV2605L | SnapEDA / KiCad official | SnapEDA (WSON-12) |
| MAX98357A | SnapEDA | SnapEDA (WLP-16) |
| MCP73831 | KiCad default | KiCad default (SOT-23-5) |
| AP2112K | SnapEDA | SOT-23-5 (standard) |
| GCT USB4135 | SnapEDA / GCT website | SnapEDA |
| VG1030001XH | Create manually (2 pads) | Create manually |
| Alps SKRPABE010 | SnapEDA | SnapEDA |

> SnapEDA (snapeda.com) — search by part number, download KiCad format.
> Always verify footprint pad dimensions against the component datasheet.

---

## Critical Design Rules (must not violate)

1. ESP32-S3 antenna area: 3mm copper keepout on PCB under antenna edge
2. USB-C CC pins: BOTH CC1 and CC2 need 5.1kΩ to GND — non-negotiable
3. I2C pull-ups: only ONE set (R1, R2) — do not add more
4. MAX98357A SD_MODE: must be driven, never floating
5. MCP73831 PROG: 10kΩ sets 100mA — do not change without recalculating
6. LiPo connector: JST-PH keyed, pin 1 = positive — verify before soldering
7. Bypass caps: place within 1mm of each IC VCC pin on same copper layer
8. Board outline: 45×35mm — check all components fit before routing

---

## Session Log

> Add an entry after every work session. One line per session.

| Date | What was done | What is next |
|---|---|---|
| 2026-04-26 | Branch created, folder structure set up, context file written | Install KiCad, create project, start Block 1 power schematic |
