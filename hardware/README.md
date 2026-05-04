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
| datasheets | 🔄 In progress | 4 of 9 downloaded — see datasheets/README.md |
| kicad | 🔄 In progress | Project created, Block 1 (Power) schematic complete |
| schematics | ⬜ Not started | Export PDF when schematic is complete |
| pcb | ⬜ Not started | Waiting on schematic complete + DRC clean |
| enclosure | ⬜ Not started | Waiting on PCB dimensions confirmed |
| bom | ✅ Master BOM started | Update as parts confirmed |
