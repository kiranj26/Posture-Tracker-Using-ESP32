# Schematics

KiCad schematic files live here. One schematic per design revision.

## File naming convention

```
posture_tracker_v2_r1.kicad_sch    ← first revision
posture_tracker_v2_r2.kicad_sch    ← after first revision corrections
```

## Status

| File | Revision | Status | Notes |
|---|---|---|---|
| posture_tracker_v2 | R1 | 🔄 In progress | KiCad project created, Block 1 (Power) complete, ERC clean |

## Schematic blocks progress

| Block | Description | Status |
|---|---|---|
| 1 | Power — USB-C, MCP73831, AP2112K, LiPo connector | ✅ Done |
| 2 | MCU — ESP32-S3-MINI-1U, bypass caps, GPIO assignments | ⬜ Not started |
| 3 | IMU — MPU-6050, I2C pull-ups | ⬜ Not started |
| 4 | Haptic — DRV2605L, LRA connector | ⬜ Not started |
| 5 | Audio — MAX98357A, speaker connector | ⬜ Not started |
| 6 | Buttons + LED | ⬜ Not started |
| 7 | Battery voltage sense | ⬜ Not started |

## KiCad project location

Active KiCad files are in `hardware/kicad/posture_tracker_v2/`.
See `hardware/kicad/KICAD_CONTEXT.md` for full design context and session log.
See `hardware/kicad/KICAD_BEGINNER_GUIDE.md` for step-by-step drawing instructions.
