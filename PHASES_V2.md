# PHASES_V2.md — Posture Tracker V2 Build Guide

> **Branch:** `release/v2-product`
> This file documents every V2 development phase with entry conditions, exact steps,
> and pass criteria. No phase may be skipped. No PR merged to `release/v2-product`
> without the phase pass criteria verified on real hardware.
> V1 phases (0–6) are in [PHASES.md](PHASES.md) on the main branch.

---

## Phase Map

```
Phase 7 ──► Phase 8 ──► Phase 9 ──► Phase 10 ──► Phase 11
Haptics     Speaker     Battery      PCB V2        Enclosure
breadboard  swap        system       fabrication   + full
validation  validation  validation                 integration
```

---

## Phase 7 — Haptic Validation on Breadboard

### Status: ⏳ Next

### Objective
Prove the DRV2605L + LRA motor combination works, feels right, and produces
clearly distinguishable feedback patterns — all before committing to PCB layout.
This phase has zero PCB risk. If something feels wrong, fix it in firmware.

### Entry conditions
- V1 breadboard (phases 0–6) working and flashed
- Adafruit DRV2605L breakout board purchased
- 10mm LRA motor purchased (Jinlong Z10SC2B or equivalent)
- `release/v2-product` branch checked out

### Hardware needed
| Item | Where to buy | Approx cost |
|---|---|---|
| Adafruit DRV2605L breakout (#2305) | adafruit.com | $7.95 |
| 10mm LRA motor | Digikey / Mouser / Adafruit #1201 | $3–5 |
| 4× jumper wires | Already have | — |

### Wiring — DRV2605L breakout to ESP32

```
DRV2605L breakout   →   ESP32
─────────────────────────────────
VIN                 →   3.3V
GND                 →   GND
SDA                 →   GPIO 21   (shared with MPU-6050)
SCL                 →   GPIO 22   (shared with MPU-6050)
LRA+ / MOTOR+       →   LRA motor wire 1
LRA- / MOTOR-       →   LRA motor wire 2
```

> The Adafruit breakout has onboard pull-ups on SDA/SCL. The MPU-6050 GY-521 breakout
> also has onboard pull-ups. With both on the same bus you now have two sets of pull-ups
> in parallel — effective pull-up ~2.35kΩ. This is within I2C spec. Do NOT add more.

> LRA motor has no polarity. Either wire order works.

### What to build

**New file: `src/main/haptic.c`**
```c
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "haptic.h"

static const char *TAG = "HAPTIC";

static esp_err_t drv_write(uint8_t reg, uint8_t val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DRV2605L_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t haptic_init(void)
{
    // Exit standby, internal trigger mode
    if (drv_write(0x01, 0x00) != ESP_OK) {
        ESP_LOGE(TAG, "DRV2605L not found on I2C bus at 0x%02X", DRV2605L_ADDR);
        return ESP_FAIL;
    }
    drv_write(0x1D, 0xA8);  // LRA open-loop
    drv_write(0x03, 0x02);  // LRA library
    drv_write(0x16, 0x53);  // Rated voltage ~1.8Vrms for 10mm LRA
    drv_write(0x17, 0x74);  // Overdrive clamp voltage
    ESP_LOGI(TAG, "DRV2605L initialised — LRA mode");
    return ESP_OK;
}

void haptic_play(uint8_t waveform_id)
{
    drv_write(0x04, waveform_id);  // Waveform slot 0
    drv_write(0x05, 0x00);          // End of sequence
    drv_write(0x0C, 0x01);          // GO
}
```

**New file: `src/main/haptic.h`**
```c
#pragma once
#include "esp_err.h"
#include <stdint.h>

esp_err_t haptic_init(void);
void      haptic_play(uint8_t waveform_id);
```

**`src/main/audio.h` — add to `audio_cmd_e`:**
```c
AUDIO_CMD_HAPTIC_WARN_SINGLE,
AUDIO_CMD_HAPTIC_WARN_DOUBLE,
AUDIO_CMD_HAPTIC_BAD_STRONG,
AUDIO_CMD_HAPTIC_ESCALATION,
AUDIO_CMD_HAPTIC_CORRECTION,
AUDIO_CMD_HAPTIC_CAL_PROMPT,
AUDIO_CMD_HAPTIC_CAL_SUCCESS,
AUDIO_CMD_HAPTIC_CAL_FAIL,
```

**`src/main/audio.c` — add to `task_audio()` switch:**
```c
case AUDIO_CMD_HAPTIC_WARN_SINGLE:
    haptic_play(HAPTIC_WARN_SINGLE);
    break;
case AUDIO_CMD_HAPTIC_WARN_DOUBLE:
    haptic_play(HAPTIC_WARN_DOUBLE);
    break;
case AUDIO_CMD_HAPTIC_BAD_STRONG:
    haptic_play(HAPTIC_BAD_STRONG);
    break;
case AUDIO_CMD_HAPTIC_ESCALATION:
    haptic_play(HAPTIC_ESCALATION);
    break;
case AUDIO_CMD_HAPTIC_CORRECTION:
    haptic_play(HAPTIC_CORRECTION);
    break;
```

**`src/main/posture_fsm.c` — remap alert commands:**
```c
// WARNING entry (was AUDIO_CMD_WARN_BEEP)
enqueue_audio(AUDIO_CMD_HAPTIC_WARN_SINGLE);

// BAD entry (was AUDIO_CMD_BAD_ALERT alone)
enqueue_audio(AUDIO_CMD_HAPTIC_BAD_STRONG);
enqueue_audio(AUDIO_CMD_BAD_ALERT);

// BAD escalation (was AUDIO_CMD_BAD_ALERT alone)
enqueue_audio(AUDIO_CMD_HAPTIC_ESCALATION);
enqueue_audio(AUDIO_CMD_BAD_ALERT);

// Voice loop (was AUDIO_CMD_VOICE_CLIP alone)
enqueue_audio(AUDIO_CMD_HAPTIC_ESCALATION);
enqueue_audio(AUDIO_CMD_VOICE_CLIP);

// Correction (was AUDIO_CMD_CORRECTION_CONFIRM alone)
enqueue_audio(AUDIO_CMD_HAPTIC_CORRECTION);
enqueue_audio(AUDIO_CMD_CORRECTION_CONFIRM);
```

**`src/main/CMakeLists.txt` — add haptic.c:**
```cmake
idf_component_register(
    SRCS
        "main.c"
        "mpu6050.c"
        "audio.c"
        "posture_fsm.c"
        "haptic.c"
        "voice/sit_up_wav.c"
        "voice/thank_you_good_work_wav.c"
    INCLUDE_DIRS "."
)
target_link_libraries(${COMPONENT_LIB} PUBLIC m)
```

**`src/main/main.c` — add `haptic_init()` to boot sequence:**
```c
// After mpu6050_init(), before audio_init()
ret = haptic_init();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Haptic init failed — continuing without haptics");
    // Non-fatal — device still works without haptics
}
```

### Testing procedure

After flash, work through each alert level deliberately:

1. **Single tap test** — hold sensor at 12° for 4 seconds → feel single tap
2. **BAD entry test** — hold sensor at 25° for 6 seconds → feel strong double tap + hear beep
3. **Escalation test** — hold BAD for 30 seconds → feel triple pulse + hear voice clip
4. **Correction test** — return to baseline → feel long reward pulse + hear "thank you"
5. **Pattern distinction test** — trigger each pattern, verify you can tell them apart by feel alone, eyes closed

### Pass criteria — ALL must be true

- [ ] DRV2605L detected on I2C bus at boot — `I (...) HAPTIC: DRV2605L initialised — LRA mode`
- [ ] MPU-6050 still working — no I2C conflict between 0x68 and 0x5A
- [ ] Single tap on WARNING — clearly felt, not startling
- [ ] Double tap on WARNING sustained — distinct from single tap
- [ ] Strong double tap on BAD entry — noticeably stronger than warning taps
- [ ] Triple pulse on escalation — distinct from BAD entry feel
- [ ] Long smooth buzz on correction — clearly positive, feels like a reward
- [ ] All 5 patterns distinguishable by feel alone (eyes closed test)
- [ ] No I2C errors in 10 minutes of active testing
- [ ] Audio still works correctly alongside haptics — no interference

### Feature branch
`feature/haptics` → PR into `release/v2-product`

---

## Phase 8 — Speaker Swap Validation

### Status: ⏳ Pending

### Objective
Confirm a 20mm 0.5W 8Ω speaker provides adequate volume for voice clips before
the speaker is committed to the PCB layout. This is the cheapest point to discover
the speaker is too quiet.

### Entry conditions
- Phase 7 complete and merged
- 20mm 8Ω 0.5W speaker purchased (CUI CMS-2009-SMT-TR or equivalent)

### Hardware needed
| Item | Where to buy | Approx cost |
|---|---|---|
| 20mm 8Ω 0.5W speaker | Digikey / Mouser / CUI | $1–3 |
| 2× alligator clips or solder | — | — |

### Wiring
Disconnect existing 3W 4Ω speaker from MAX98357A output terminals.
Connect 20mm 8Ω speaker in its place. No firmware changes needed.

> **IMPORTANT:** The existing speaker is 4Ω. The replacement is 8Ω. MAX98357A
> supports both. Output power at 8Ω from 5V supply ≈ 0.5W — correct and safe.
> Do NOT reconnect the 4Ω speaker after this test — it will not match the V2 PCB.

### Testing procedure

1. Flash unchanged V1 firmware (no code changes for this phase)
2. Power on — listen to boot tone volume at 0.5 metre distance
3. Trigger calibration — listen to cal prompt and success tones
4. Trigger WARNING → BAD → voice clip — evaluate "sit up straight" intelligibility
5. Trigger correction — evaluate "thank you good work" intelligibility
6. Test at 1 metre in a quiet room
7. Test at 1 metre with background noise (fan, music at low volume)

### Pass criteria — ALL must be true

- [ ] Boot tone audible at 1 metre — no strain to hear it
- [ ] "Sit up straight" clearly intelligible at 1 metre, quiet room
- [ ] "Sit up straight" intelligible at 1 metre with light background noise
- [ ] "Thank you good work" clearly intelligible at 1 metre
- [ ] Calibration tones (ascending 3-note) audible and distinct
- [ ] No distortion at maximum volume
- [ ] No rattling or resonance from the small speaker cone
- [ ] Volume acceptable for open-plan office use (not embarrassingly loud)

### If volume is insufficient
- Increase `VOL_VOICE` in `config.h` from 0.90f toward 1.0f
- If still insufficient at 1.0f, evaluate 28mm speaker as alternative
- Document the finding and decision in this file before proceeding

### Feature branch
`feature/speaker-swap` → PR into `release/v2-product`

---

## Phase 9 — Battery System Validation

### Status: ⏳ Pending

### Objective
Validate the MCP73831 charging circuit + AP2112K LDO + LiPo battery combination
before they appear on the V2 PCB. Catch any design errors at evaluation board
stage, not after PCB fabrication.

### Entry conditions
- Phase 8 complete and merged
- Battery components purchased or evaluation board available

### Hardware options (pick one)

**Option A (recommended) — Adafruit LiPo charger breakout:**
- Adafruit Micro LiPo charger #1904 (MCP73831 based)
- 350mAh LiPo with JST-PH connector
- AP2112K-3.3 SOT-23-5 breakout (available on eBay/AliExpress)
- Validates charging behaviour and LDO stability without custom PCB

**Option B — Small test PCB:**
- Design a minimal test PCB: MCP73831 + AP2112K + USB-C + JST + caps
- Order 5 units from JLCPCB (~$15 total)
- More rigorous, closer to final design

### What to build in firmware

**New file: `src/main/power.c`**

```c
#include "config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "power.h"
#include "audio.h"

static const char *TAG = "POWER";
static adc_oneshot_unit_handle_t s_adc_handle;

esp_err_t power_init(void)
{
    adc_oneshot_unit_init_cfg_t cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&cfg, &s_adc_handle);
    if (ret != ESP_OK) return ret;

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_12,
    };
    return adc_oneshot_config_channel(s_adc_handle, VBAT_SENSE_CHANNEL, &chan_cfg);
}

float power_read_battery_voltage(void)
{
    int raw = 0;
    adc_oneshot_read(s_adc_handle, VBAT_SENSE_CHANNEL, &raw);
    float adc_v = (raw / 4095.0f) * 3.3f;
    return adc_v * VBAT_DIVIDER_RATIO;
}

void power_check_battery(void)
{
    float v = power_read_battery_voltage();
    ESP_LOGI(TAG, "Battery: %.2fV", v);

    if (v < LOW_BATTERY_CUTOFF_V) {
        ESP_LOGE(TAG, "Battery critical — entering deep sleep");
        audio_cmd_t cmd = { .type = AUDIO_CMD_BOOT_READY };  // repurpose as shutdown tone
        xQueueSend(g_audio_queue, &cmd, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_deep_sleep_start();
    } else if (v < LOW_BATTERY_WARN_V) {
        ESP_LOGW(TAG, "Battery low");
        // Enqueue low battery beep — add AUDIO_CMD_LOW_BATTERY to enum
    }
}
```

**`config.h` additions:**
```c
#define VBAT_SENSE_CHANNEL      ADC_CHANNEL_6   // GPIO34 = ADC1 channel 6
#define VBAT_DIVIDER_RATIO      2.0f
#define LOW_BATTERY_WARN_V      3.5f
#define LOW_BATTERY_CUTOFF_V    3.3f
#define BATTERY_CHECK_INTERVAL_MS  60000
```

### Testing procedure

1. Connect LiPo charger breakout to USB-C — verify STAT LED shows charging
2. Measure LiPo voltage with multimeter while charging — confirm rises to 4.2V
3. Disconnect USB — confirm LDO still outputs 3.3V from LiPo
4. Run firmware with `power_check_battery()` logging every 60s
5. Let LiPo discharge to 3.5V — confirm warning fires
6. Let discharge to 3.3V — confirm deep sleep triggers
7. Charge back to full — confirm device wakes normally on reconnect
8. Time the discharge: run full firmware (IMU + FSM + occasional audio + haptic) — measure hours to 3.3V cutoff

### Pass criteria — ALL must be true

- [ ] LiPo charges via USB-C at ~100mA (verify with multimeter or USB meter)
- [ ] STAT LED: on while charging, off when done
- [ ] 3.3V stable on LDO output from 4.2V → 3.4V LiPo input
- [ ] Low battery warning fires correctly at 3.5V
- [ ] Deep sleep triggers at 3.3V cutoff — device does not crash before this
- [ ] Device wakes correctly on USB reconnect after deep sleep
- [ ] Full runtime (active firmware, no aggressive audio): ≥ 10 hours to cutoff
- [ ] No brownout or voltage glitch during audio playback (most current-intensive moment)

### Feature branch
`feature/power-system` → PR into `release/v2-product`

---

## Phase 10 — PCB V2 Design & Fabrication

### Status: ⏳ Pending

### Objective
Design and fabricate the first custom PCB integrating all validated V2 components.
This is the highest-risk phase — layout errors cost money and 2–3 weeks of turnaround.
Every component must be validated in Phase 7–9 before being placed on this PCB.

### Entry conditions
- Phases 7, 8, and 9 complete and merged
- All waveform indices confirmed satisfactory (Phase 7)
- Speaker volume confirmed adequate (Phase 8)
- Battery circuit confirmed stable (Phase 9)
- KiCad or EasyEDA installed

### Schematic checklist (complete before layout)

- [ ] ESP32-S3-MINI-1U with all bypass caps placed
- [ ] Antenna keep-out zone marked as no-copper zone (3mm below module edge)
- [ ] MPU-6050: 4.7kΩ pull-ups on SDA/SCL, 100nF bypass on VCC, AD0 to GND
- [ ] DRV2605L: correct WSON-12 footprint, LRA pads broken out, EN tied HIGH
- [ ] MAX98357A: correct WLP-16 footprint, SD_MODE to GPIO48, GAIN floating
- [ ] MCP73831: PROG resistor 10kΩ (100mA charge), STAT pin to LED via 1kΩ
- [ ] AP2112K: bulk caps 10µF in + out, 100nF bypass
- [ ] USB-C: both CC pins to GND via 5.1kΩ, VBUS to polyfuse to MCP73831
- [ ] USB-C D+/D- directly to ESP32-S3 GPIO19/20
- [ ] JST-PH 2-pin battery connector with correct pinout (+ on pin 1)
- [ ] Both buttons with 10kΩ pull-up to 3.3V, 100nF debounce cap to GND
- [ ] VBAT sense: 100kΩ–100kΩ divider from LiPo+ to GPIO34 to GND
- [ ] Charge LED: green LED + 1kΩ from STAT to 3.3V

### Layout checklist (complete before Gerber export)

- [ ] Board dimensions ≤ 50×38mm
- [ ] Antenna keep-out enforced — no copper within 3mm of antenna area
- [ ] All bypass caps within 1mm of their IC VCC pin
- [ ] I2S traces (BCLK, WS, DOUT) short and parallel, same layer
- [ ] USB D+/D- matched length, routed as differential pair
- [ ] Speaker OUT+ / OUT- kept together, away from I2C/I2S signal lines
- [ ] Power traces ≥ 0.5mm (3.3V rail), ≥ 1.0mm (battery, speaker)
- [ ] LRA pads on bottom side with exposed copper for motor adhesive mount
- [ ] Battery connector on edge, accessible without opening enclosure
- [ ] USB-C on opposite edge to battery
- [ ] Buttons on front face (long dimension edge)
- [ ] Speaker pads on top face (edge-mount or through-hole)
- [ ] DRC passes with zero errors

### JLCPCB order checklist

- [ ] Gerber files exported correctly (all layers)
- [ ] BOM exported in JLCPCB CSV format
- [ ] CPL (Component Placement List) exported
- [ ] All SMT components matched to JLCPCB parts library
- [ ] SMT assembly requested for top side only
- [ ] Order 5 prototype units (not 10 — there will be a revision)
- [ ] Specify ENIG surface finish for fine-pitch QFN/WLP pads
- [ ] Specify 1.6mm board thickness

### Post-receipt testing procedure

For each of the 5 PCBs:

1. Visual inspection — all ICs present, no shorts, no bridged pads
2. Continuity check on power rail — 3.3V and GND not shorted
3. Connect USB-C — verify 3.3V on LDO output before connecting battery
4. Connect LiPo — verify system powers on
5. Flash firmware via USB-C (hold BOOT button on SW1, press RESET)
6. Verify serial monitor output shows all modules initialised
7. Run Phase 7 haptic test — all 5 patterns
8. Run Phase 8 speaker test — voice clips intelligible
9. Run Phase 9 battery test — charge and discharge cycle
10. Soak test: run full firmware for 2 hours, monitor for faults

### Pass criteria — ALL must be true

- [ ] All 5 boards power on from USB-C
- [ ] All 5 boards flash successfully via native USB
- [ ] Boot log shows: MPU-6050, DRV2605L, MAX98357A all initialised
- [ ] Haptics working — all 5 waveform patterns fire
- [ ] Audio working — voice clips and tones play at adequate volume
- [ ] Battery charges from USB-C — STAT LED indicates correctly
- [ ] Both buttons respond — SW1 triggers recal, SW2 triggers snooze
- [ ] Board fits within enclosure dimensions (Phase 11 check)
- [ ] 2-hour soak test: no crashes, no resets, no stuck states
- [ ] At least 3 of 5 boards fully pass (accept 2 duds for manufacturing variance)

### Feature branch
`feature/pcb-v2` → PR into `release/v2-product`
Commit all KiCad project files, Gerber exports, BOM, and CPL.

---

## Phase 11 — Full Integration, Enclosure & Wearable Validation

### Status: ⏳ Pending

### Objective
A complete wearable device worn for a real work session. PCB inside 3D printed
shoulder clip, battery connected, all features functional. This is the V2 product.

### Entry conditions
- Phase 10 complete — at least 3 fully passing PCBs in hand
- 3D printer available, or Shapeways/JLCPCB 3D printing ordered
- PETG filament (preferred: flexible enough to clip, rigid enough to hold)

### Enclosure design requirements

**Functional:**
- PCB slides into recessed pocket — retained without screws if possible (friction fit)
- Spring clip mechanism — clips onto shirt collar, bra strap, 1–2cm fabric
- USB-C port accessible from bottom edge without removing from clip
- Both buttons accessible on front face with short press travel
- Speaker grille on top face — 5–8 holes, 1mm diameter
- LRA motor recess on back face — motor contacts body when worn

**Dimensional targets:**
- Clip opening: 15mm max gap, 5N clip force (firm but not damaging)
- Overall: 45×38×16mm
- Pocket for PCB: 47×37×9mm (1mm tolerance each side)
- Battery slot: behind PCB pocket, 37×27×6mm

**CAD files:**
- Design in Fusion 360, FreeCAD, or Onshape (all free tier)
- Export STL for printing
- Commit STL + source file to `feature/enclosure` branch

### Wearability testing procedure

1. Print 2 enclosure prototypes — check fit with PCB before soldering anything
2. Assemble one complete unit: PCB + LiPo + enclosure
3. Clip to shirt collar at left shoulder — sensor mounting orientation
4. Power on — confirm calibration prompt plays and is audible when worn
5. Calibrate in sitting position at desk
6. Work for 4 hours wearing the device:
   - Normal desk work (typing, reading)
   - One 30-minute meeting (test snooze button)
   - Deliberate slouching every 30 minutes to verify alerts fire
   - Check battery level at end of session
7. Repeat with 2 different people if possible (sensor position may vary)

### Pass criteria — ALL must be true

- [ ] Device clips securely to shirt — does not fall during normal movement
- [ ] Calibration completes correctly in shoulder-mounted orientation
- [ ] WARNING haptic fires correctly during desk work slouch — not during typing
- [ ] BAD audio fires after sustained poor posture — not from vibration/walking
- [ ] Snooze button accessible and works during meeting scenario
- [ ] Voice clip audible while wearing — speaker facing outward is sufficient
- [ ] Device worn for 4+ hours — no crashes, no false positives from desk vibration
- [ ] Battery lasts full 4-hour session with >50% charge remaining
- [ ] Enclosure: no sharp edges, no accidental button presses, clip does not mark clothing
- [ ] Serial log: clean state transitions, no spurious events throughout session

---

## Phase Summary Table

| Phase | Description | Branch | Status |
|---|---|---|---|
| 7 | Haptic validation — breadboard | `feature/haptics` | ⏳ Next |
| 8 | Speaker swap validation | `feature/speaker-swap` | ⏳ Pending |
| 9 | Battery system validation | `feature/power-system` | ⏳ Pending |
| 10 | PCB V2 design + fabrication | `feature/pcb-v2` | ⏳ Pending |
| 11 | Full integration + enclosure | `feature/enclosure` | ⏳ Pending |

---

## Hardware Shopping List (Phases 7–9)

Order these before starting Phase 7. Most arrive in 2–5 days.

| Item | Source | Est. cost | Needed for |
|---|---|---|---|
| Adafruit DRV2605L breakout #2305 | adafruit.com | $7.95 | Phase 7 |
| 10mm LRA motor (Adafruit #1201 or Digikey) | adafruit.com / digikey.com | $4.00 | Phase 7 |
| 20mm 8Ω 0.5W speaker | digikey.com / mouser.com | $2.00 | Phase 8 |
| Adafruit LiPo charger breakout #1904 | adafruit.com | $6.95 | Phase 9 |
| 350mAh LiPo with JST-PH #2750 | adafruit.com | $6.95 | Phase 9 |
| AP2112K-3.3 breakout or bare SOT-23-5 | digikey.com | $1.00 | Phase 9 |
| **Total** | | **~$30** | Phases 7–9 |

---

## Known Risks & Mitigations

| Risk | Phase | Mitigation |
|---|---|---|
| I2C address conflict (MPU-6050 + DRV2605L) | 7 | Different addresses: 0x68 vs 0x5A — no conflict, verified in spec |
| LRA waveforms feel similar / undifferentiated | 7 | Test all 123 indices, document best candidates before committing |
| 20mm speaker too quiet for voice clips | 8 | Test before PCB order — can switch to 28mm if needed |
| Brownout during audio playback on LiPo | 9 | AP2112K has 600mA headroom, add 22µF bulk cap on output |
| ESP32-S3 antenna detuned by PCB copper | 10 | Enforce keep-out in layout — check with BLE range test post-fab |
| MAX98357A WLP-16 bridged pads (tiny IC) | 10 | Use ENIG finish, inspect under magnification, reflow stencil |
| Clip spring force too strong / marks clothing | 11 | Print multiple prototypes with varying wall thickness |
| Sensor calibration offset differs per person | 11 | Calibration routine handles this — test with 2+ people |

---

*Branch: release/v2-product*
*Created: 2026-04-26*
*Reference: CLAUDE_V2.md — full architecture and component decisions*
