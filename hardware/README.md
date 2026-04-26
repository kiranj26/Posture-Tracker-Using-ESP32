# Hardware

All hardware design files for Posture Tracker V2.

## Folder Structure

```
hardware/
├── datasheets/    ← PDF datasheets for every component on the BOM
├── schematics/    ← KiCad schematic files (.kicad_sch)
├── pcb/           ← KiCad PCB layout + Gerber exports for JLCPCB
├── enclosure/     ← 3D printable clip files (STL + source)
└── bom/           ← Master BOM and JLCPCB-formatted exports
```

## Current Status

| Folder | Status | Waiting on |
|---|---|---|
| datasheets | ⬜ Empty — download needed | Nothing — do this now |
| schematics | ⬜ Not started | Phase 7 + 8 + 9 validation |
| pcb | ⬜ Not started | Schematic complete |
| enclosure | ⬜ Not started | PCB dimensions confirmed |
| bom | ✅ Master BOM started | Update as parts confirmed |

## First thing to do

Download all datasheets listed in [datasheets/README.md](datasheets/README.md)
and drop them in the datasheets folder. This is the only hardware task
that can be done right now before parts arrive.
