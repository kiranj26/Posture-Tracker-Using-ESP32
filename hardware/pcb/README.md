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
| posture_tracker_v2 | R1 | 🔄 Routing in progress — power rails next |

## Layout status (2026-05-03)

- All 46 components placed ✅
- Board outline adjusted to 45.5 × 27mm for wearable form factor ✅
- DRC baseline: **154 unconnected, 2 acceptable silk warnings (U1 antenna), 0 copper errors** ✅
- GND routing strategy: B.Cu copper pour filled at end — no manual GND traces
- Net classes set: Power (0.5mm trace / 0.8mm via) and Default (0.2mm / 0.6mm via)
- Connector footprints: J3 + LS1 updated to JST GH 1.25mm (matches physical cables)
- Gerbers: NOT generated yet — routing incomplete

## Routing progress

| Net group | Connections | Status |
|---|---|---|
| GND (~72) | — | ⏳ B.Cu copper pour — done last |
| +3V3 | 23 | ⏳ Next |
| VBAT | 7 | ⏳ Pending |
| VBUS / VBUS_FUSED | 4 | ⏳ Pending |
| I2C (SDA/SCL) | 6 | ⏳ Pending |
| USB_DP / USB_DM | 8 | ⏳ Pending |
| I2S (BCLK/WS/DOUT) | 3 | ⏳ Pending |
| Signals + buttons + misc | ~31 | ⏳ Pending |

## Checklist before Gerber export

- [x] Schematic R1 complete and ERC clean
- [x] All 46 components placed
- [x] DRC clean (0 copper errors)
- [ ] All nets routed
- [ ] B.Cu copper pour filled and DRC re-run
- [ ] Final DRC: 0 errors, 0 unconnected
