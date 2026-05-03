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
| posture_tracker_v2 | R1 | ✅ Complete | All 7 blocks done. ERC: 0 errors, 5 warnings. Reddit review fixes applied. |

## Schematic blocks progress

| Block | Description | Status |
|---|---|---|
| 1 | Power — USB-C (HRO TYPE-C-31-M-12), MCP73831, AP2112K, LiPo connector | ✅ Done |
| 2 | MCU — ESP32-S3-MINI-1, bypass caps (C1+C15), GPIO assignments | ✅ Done |
| 3 | IMU — MPU-6050, I2C pull-ups | ✅ Done |
| 4 | Haptic — DRV2605L, LRA connector, C16 VDD bulk | ✅ Done |
| 5 | Audio — MAX98357A, speaker connector | ✅ Done |
| 6 | Buttons (SW1/SW2/SW3) + LED1 + LED2 + ESD protection (U7) | ✅ Done |
| 7 | Battery voltage sense | ✅ Done |

## Post-schematic fixes applied (2026-05-03)

- USB-C B6/B7 cross-connection added (both cable orientations now work)
- C17 (4.7µF) added on VBUS_FUSED — MCP73831 VDD decoupling
- U1 footprint corrected from ESP32-S2-MINI-1 to ESP32-S3-MINI-1
- Total components: 46 (42 ICs + passives + 4 added via critique/review)

## KiCad project location

Active KiCad files are in `hardware/kicad/posture_tracker_v2/`.
See `hardware/kicad/KICAD_CONTEXT.md` for full design context and session log.
See `hardware/kicad/KICAD_BEGINNER_GUIDE.md` for step-by-step drawing instructions.
