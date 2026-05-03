# PCB Layout

KiCad PCB layout files and Gerber exports live here.

## File naming convention

```
posture_tracker_v2_r1.kicad_pcb       ← KiCad PCB file
gerbers/posture_tracker_v2_r1/        ← Gerber export folder for JLCPCB
bom_jlcpcb_v2_r1.csv                  ← BOM in JLCPCB format
cpl_jlcpcb_v2_r1.csv                  ← Component placement list
```

## Target specs

| Spec | Value |
|---|---|
| Dimensions | 45 × 35mm |
| Layers | 2 |
| Thickness | 1.6mm |
| Min trace width | 0.15mm signal / 0.5mm power |
| Surface finish | ENIG |
| Manufacturer | JLCPCB |
| Prototype qty | 5 units |

## Status

| File | Revision | Status |
|---|---|---|
| posture_tracker_v2_r1.kicad_pcb | R1 | 🔄 In progress — components placed, routing not started |

## Layout status (2026-05-03)

- All 46 components placed on board (45×35mm outline confirmed)
- Board outline: `gr_rect (start 165 100) (end 210 135)` = exactly 45×35mm ✅
- DRC: 4 cosmetic silkscreen warnings, 148 unconnected pads (all expected — routing not done)
- Premature Gerbers deleted — must regenerate after routing complete
- U1 footprint: corrected to `RF_Module:ESP32-S3-MINI-1` (65 pads)

## Do not start layout until

- [x] Schematic R1 complete and reviewed
- [x] ERC passes with zero errors (0 errors, 5 warnings — all acceptable)
- [x] All component footprints confirmed against physical parts
- [ ] PCB routing complete
- [ ] DRC passes with zero errors after routing
