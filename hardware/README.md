# Hardware

All hardware design files for Posture Tracker V2.

## Folder Structure

```
hardware/
├── datasheets/    ← PDF datasheets for every component on the BOM
├── kicad/         ← KiCad project files (schematic + PCB)
├── schematics/    ← Exported schematic PDFs per revision
├── pcb/           ← KiCad PCB layout + Gerber exports for JLCPCB
├── enclosure/     ← 3D printable clip files (STL + source)
└── bom/           ← Master BOM and JLCPCB-formatted exports
```

## Current Status

| Folder | Status | Notes |
|---|---|---|
| datasheets | ✅ Done | All component datasheets present — see datasheets/README.md |
| kicad | ✅ Complete | Schematic (ERC clean) + PCB layout (DRC clean) + Gerbers exported |
| schematics | ✅ Done | Exported PDF at `schematics/posture_tracker_v2_v2.pdf` |
| pcb | ✅ Gerbers ready | `pcb/v2/gerbers/` — 12 files + ZIP ready for JLCPCB upload |
| enclosure | ⬜ Not started | Waiting on PCB prototypes received (Phase 10) |
| bom | ✅ Master BOM started | Update as parts ordered for Phase 10 |
