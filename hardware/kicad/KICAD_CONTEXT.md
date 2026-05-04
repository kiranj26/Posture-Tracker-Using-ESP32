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
| KiCad installed | ✅ Done | |
| Project created | ✅ Done | posture_tracker_v2.kicad_sch |
| Power block schematic | ✅ Done | ERC clean, 0 errors |
| MCU block schematic | ✅ Done | ERC clean, 0 errors |
| IMU block schematic | ✅ Done | ERC clean, 0 errors |
| Haptic block schematic | ✅ Done | ERC clean, 0 errors |
| Audio block schematic | ✅ Done | ERC clean, 0 errors |
| Buttons + LED schematic | ✅ Done | ERC clean, 0 errors |
| Battery sense schematic | ✅ Done | ERC clean, 0 errors |
| Full schematic DRC | ✅ Done | 0 errors, 5 warnings (all acceptable) |
| Schematic critique fixes | ✅ Done | SW3 reset, MPU_INT, HAPTIC_EN+R12, SKRPABE010 buttons, LED2+R13 snooze, C14 VBAT bulk, antenna note |
| Reddit review fixes | ✅ Done | USB-C B6/B7 cross-connection, C17 VBUS_FUSED bypass cap, U7 ESD protection, C15/C16 decoupling, U1 footprint fixed to ESP32-S3-MINI-1 |
| PCB layout started | 🔄 In progress | 46 components placed, 148 unconnected pads — routing not yet done |
| PCB layout complete | ⬜ Not started | |
| PCB DRC clean | ⬜ Not started | |
| Gerbers exported | ⬜ Not started | Must regenerate after routing |
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
| 2026-05-03 | Changed U1 to ESP32-S3-MINI-1 (not MINI-1U) | MINI-1 has built-in PCB antenna, 65 pads; MINI-1U needs external antenna. Both use same pin assignments |
| 2026-05-03 | Changed J1 to HRO TYPE-C-31-M-12 | Better KiCad library support, correct 14-pin mid-mount footprint for 2-layer PCB |
| 2026-05-03 | Added U7 USBLC6-2SC6 ESD protection on USB D+/D- | Protects ESP32-S3 USB PHY from ESD events on connector; essential for wearable handling |
| 2026-05-03 | Added C15 (10µF, 0805) on ESP32-S3 VDD33 | Bulk decoupling for MCU power rail; reduces voltage sag during Bluetooth bursts |
| 2026-05-03 | Added C16 (1µF, 0402) on DRV2605L VDD | Datasheet recommended local bypass; original design only had 100nF |
| 2026-05-03 | Added C17 (4.7µF, 0402) on VBUS_FUSED | MCP73831 VDD decoupling — was missing. Reviewer flagged; 4.7µF per MCP73831 datasheet recommendation |
| 2026-05-03 | USB-C B6/B7 cross-connection per USB-IF spec | Per spec Table 3-1: A6+B7=D+, A7+B6=D- (diagonal). Both cable orientations now work |

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

### Block 2 — MCU (ESP32-S3-MINI-1)
```
ESP32-S3-MINI-1 (U1)
  → 3V3: 3.3V rail + 100nF bypass cap to GND
  → GND: GND
  → GPIO8:  SDA net (GPIO22 does not exist on ESP32-S3 — corrected)
  → GPIO9:  SCL net (GPIO22 does not exist on ESP32-S3 — corrected)
  → GPIO21: no-connect (unused)
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
| U1 | ESP32-S3-MINI-1 (built-in PCB antenna) |
| U2 | MPU-6050 |
| U3 | DRV2605L |
| U4 | MAX98357A |
| U5 | MCP73831 |
| U6 | AP2112K-3.3 |
| U7 | USBLC6-2SC6 — USB ESD protection (SOT-23-6) |
| M1 | LRA motor (VG1030001XH) |
| LS1 | Speaker (20mm 8Ω) |
| J1 | USB-C connector (HRO TYPE-C-31-M-12) |
| J2 | JST-PH battery connector |
| J3 | Debug/programming header (2-pin) |
| SW1 | Button — BOOT/CAL (Alps SKRPABE010) |
| SW2 | Button — SNOOZE (Alps SKRPABE010) |
| SW3 | Button — RESET (Alps SKRPABE010) |
| LED1 | Charge indicator (green) |
| LED2 | Snooze indicator |
| F1 | Polyfuse 500mA |
| R1 | 4.7kΩ — I2C SDA pull-up |
| R2 | 4.7kΩ — I2C SCL pull-up |
| R3 | 10kΩ — MCP73831 PROG (charge current) |
| R4 | 5.1kΩ — USB-C CC1 |
| R5 | 5.1kΩ — USB-C CC2 |
| R6 | 1kΩ — LED1 current limit |
| R7 | 10kΩ — SW1 pull-up |
| R8 | 10kΩ — SW2 pull-up |
| R9 | 100kΩ — VBAT sense divider top |
| R10 | 100kΩ — VBAT sense divider bottom |
| R11 | 10kΩ — ESP32 EN pull-up |
| R12 | 10kΩ — HAPTIC_EN pull-up |
| R13 | 1kΩ — LED2 current limit |
| C1 | 100nF — U1 VDD33 bypass |
| C2 | 100nF — U2 bypass |
| C3 | 100nF — U3 VDD bypass |
| C4 | 100nF — U4 DVDD bypass |
| C5 | 22µF — U4 PVDD bulk |
| C6 | 10µF — U6 LDO input bulk |
| C7 | 10µF — U6 LDO output bulk |
| C8 | 100nF — SW1 debounce |
| C9 | 100nF — SW2 debounce |
| C10 | 100nF — VBAT sense filter |
| C11 | 100nF — ESP32 EN filter |
| C12 | 100nF — U3 extra bypass |
| C13 | 100nF — SW3 debounce |
| C14 | 47µF — VBAT bulk cap |
| C15 | 10µF — U1 VDD33 bulk (0805) |
| C16 | 1µF — U3 VDD local bulk (0402) |
| C17 | 4.7µF — VBUS_FUSED bypass / U5 VDD decoupling (0402) |

---

## KiCad Libraries Needed

These symbols and footprints are not in KiCad's default library.
Must be downloaded or created manually.

| Component | Symbol source | Footprint source |
|---|---|---|
| ESP32-S3-MINI-1 | KiCad RF_Module lib | KiCad RF_Module:ESP32-S3-MINI-1 (65 pads) |
| MPU-6050 | KiCad default (Sensor lib) | KiCad default |
| DRV2605L | SnapEDA / KiCad official | TSSOP-10 3x3mm |
| MAX98357A | SnapEDA | TQFN-16-1EP 3x3mm |
| MCP73831 | KiCad default | KiCad default (SOT-23-5) |
| AP2112K | SnapEDA | SOT-23-5 (standard) |
| USBLC6-2SC6 | KiCad default | SOT-23-6 |
| HRO TYPE-C-31-M-12 | KiCad Connector lib | Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12 |
| VG1030001XH | Create manually (2 pads) | Create manually |
| Alps SKRPABE010 | Custom / SnapEDA | SW_SKRPABE010 |

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
| 2026-04-27 | Block 1 (Power) schematic complete — J1, F1, U5, J2, U6, bypass caps, PWR_FLAG. ERC: 0 errors. | Draw Block 2 (MCU — ESP32-S3-MINI-1U) |
| 2026-05-02 | Blocks 2–7 complete — ESP32-S3, MPU-6050, DRV2605L, MAX98357A, Buttons+LED, VBAT sense. All 37 footprints assigned. Full ERC: 0 errors, 30 warnings. | Schematic critique fixes |
| 2026-05-02 | Schematic critique complete — added SW3 (RESET), MPU_INT net, HAPTIC_EN+R12 pull-up, SKRPABE010 button footprints, LED2+R13 snooze indicator, C14 47µF VBAT bulk cap, antenna keep-out note. 42 components, ERC: 0 errors, 30 warnings. | Start PCB layout |
| 2026-05-03 | Reddit review fixes: fixed USB-C B6/B7 cross-connection (A6+B7=D+, A7+B6=D- per USB-IF spec), added C17 4.7µF on VBUS_FUSED (MCP73831 VDD decoupling), confirmed U7 USBLC6-2SC6 ESD protection, added C15 (10µF U1 VDD33) and C16 (1µF U3 VDD). Fixed U1 footprint from ESP32-S2-MINI-1 → ESP32-S3-MINI-1 (critical — wrong pad count). Deleted premature Gerbers. ERC: 0 errors, 5 warnings. DRC: 4 cosmetic warnings, 148 unconnected pads (routing not done). | PCB routing |
