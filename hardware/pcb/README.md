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
| posture_tracker_v2_r1.kicad_pcb | R1 | ⬜ Not started |

## Do not start layout until

- [ ] Schematic R1 complete and reviewed
- [ ] DRC passes on schematic with zero errors
- [ ] All component footprints confirmed against physical parts in hand
