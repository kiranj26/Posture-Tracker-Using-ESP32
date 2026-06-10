# PCB Fabrication Outputs

This folder contains the fabrication outputs sent to JLCPCB — not the KiCad source files.
KiCad source files live in `hardware/kicad/posture_tracker_v2/`.

## Folder structure

```
hardware/pcb/
└── v2/
    ├── gerbers/                          ← individual Gerber + drill files
    │   ├── posture_tracker_v2-F_Cu.gtl  ← top copper
    │   ├── posture_tracker_v2-B_Cu.gbl  ← bottom copper
    │   ├── posture_tracker_v2-F_Mask.gts
    │   ├── posture_tracker_v2-B_Mask.gbs
    │   ├── posture_tracker_v2-F_Paste.gtp
    │   ├── posture_tracker_v2-B_Paste.gbp
    │   ├── posture_tracker_v2-F_Silkscreen.gto
    │   ├── posture_tracker_v2-B_Silkscreen.gbo
    │   ├── posture_tracker_v2-Edge_Cuts.gm1
    │   ├── posture_tracker_v2-F_Courtyard.gbr
    │   ├── posture_tracker_v2-PTH.drl   ← plated through-holes
    │   └── posture_tracker_v2-NPTH.drl  ← non-plated holes (mounting)
    └── posture_tracker_v2_gerbers.zip   ← upload this to JLCPCB
```

## Board specs (V2)

| Spec | Value |
|---|---|
| Dimensions | ~47.6 × 28.3 mm |
| Layers | 2 |
| Thickness | 1.6mm |
| Copper weight | 1oz |
| Min trace width | 0.2mm signal / 0.3mm power / 0.4mm high-current |
| Surface finish | HASL (lead-free) or ENIG |
| Manufacturer | JLCPCB |
| Prototype qty | 5 units |

## DRC status (2026-06-09)

| Check | Result |
|---|---|
| DRC errors | 0 |
| Unconnected nets | 0 |
| Silk warnings | 2 (U1 silkscreen clips board edge — cosmetic, JLCPCB trims automatically) |
| Footprint errors | 0 |

## How to re-export Gerbers

1. Open `hardware/kicad/posture_tracker_v2/posture_tracker_v2/posture_tracker_v2.kicad_pcb` in KiCad
2. File → Plot → Format: Gerber, Output: `gerbers/`
3. Check: Use Protel filename extensions, uncheck: X2 format, Include netlist attributes
4. Click Plot, then Generate Drill Files
5. Copy outputs to `hardware/pcb/v2/gerbers/` and re-zip

## JLCPCB order checklist

- [x] Gerbers generated and verified
- [x] DRC clean — 0 errors, 0 unconnected
- [x] ZIP ready: `posture_tracker_v2_gerbers.zip`
- [ ] Upload to jlcpcb.com and confirm auto-detection (2 layers, correct dimensions)
- [ ] Order 5 prototype units
- [ ] Generate BOM + CPL for SMT assembly (optional)
