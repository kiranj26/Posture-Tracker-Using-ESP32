# KiCad Beginner Guide — Posture Tracker V2

> This guide is written specifically for this project. It covers only what you
> need to know to draw this schematic and lay out this PCB. Not a general tutorial.
> Read KICAD_CONTEXT.md first to understand what needs to be built.

---

## Step 0 — Install KiCad

1. Go to kicad.org → Download → KiCad 8.x for macOS
2. Install and open KiCad
3. On first launch it will download the standard symbol and footprint libraries (~2GB)
4. Let it finish before doing anything else

---

## Step 1 — Create the Project

1. Open KiCad
2. File → New Project
3. Navigate to `hardware/kicad/posture_tracker_v2/`
4. Name it: `posture_tracker_v2`
5. KiCad creates three files automatically:
   - `posture_tracker_v2.kicad_pro` — project file
   - `posture_tracker_v2.kicad_sch` — schematic (blank)
   - `posture_tracker_v2.kicad_pcb` — PCB layout (blank)

---

## Step 2 — Understand the KiCad Window

When you open KiCad you see the **Project Manager**:
```
┌─────────────────────────────────────┐
│  KiCad Project Manager              │
│                                     │
│  [Schematic Editor]  ← draw here first
│  [PCB Editor]        ← do this after schematic
│  [Symbol Editor]     ← for custom symbols
│  [Footprint Editor]  ← for custom footprints
│  [Gerber Viewer]     ← check output files
└─────────────────────────────────────┘
```

**Always work in this order: Schematic first → PCB second.**
Never open the PCB editor until the schematic is complete.

---

## Step 3 — Schematic Editor Basics

Click **Schematic Editor** to open it.

### Essential keyboard shortcuts (memorise these)

| Key | Action |
|---|---|
| `A` | Add symbol (place a component) |
| `W` | Add wire |
| `P` | Add power symbol (+3V3, GND etc) |
| `L` | Add net label (name a wire) |
| `R` | Rotate selected item |
| `G` | Grab (move while keeping connections) |
| `E` | Edit properties of selected item |
| `Delete` | Delete selected item |
| `Ctrl+Z` | Undo |
| `Scroll` | Zoom in/out |
| `Middle click drag` | Pan |

### The golden rule of schematics
**Wires must touch pins to make a connection.**
A wire that passes near a pin but does not touch it is NOT connected.
You will see a small green dot where a proper connection is made.

---

## Step 4 — How to Place a Component

1. Press `A` → Symbol chooser opens
2. Search by name (e.g. "MPU-6050" or "MCP73831")
3. Click the symbol → click OK → click on schematic to place
4. Press `Escape` when done placing

**If a component is not in the library:**
- Go to snapeda.com
- Search by exact part number
- Download KiCad format
- In KiCad: Preferences → Manage Symbol Libraries → add the downloaded file

---

## Step 5 — How to Add Power Symbols

Power symbols (+3V3, GND, VBAT) are special symbols that connect any wire
with the same label without drawing a physical wire between them.

1. Press `P` → Power symbol chooser
2. Search "GND" → place wherever you need ground
3. Search "+3V3" → if not found, create it:
   - Add a power symbol named `+3V3`
   - All nets labelled `+3V3` connect automatically

**Use power symbols everywhere instead of drawing long wires across the schematic.**
Your schematic will be much cleaner.

---

## Step 6 — How to Label a Net

Net labels connect wires that are far apart without drawing a line between them.
Same as power symbols but for signal nets (SDA, SCL, I2S_BCLK etc).

1. Press `L` → type the net name exactly as in KICAD_CONTEXT.md
2. Place the label at the end of the wire
3. Any other wire with the same label is connected — even on a different part of the schematic

**Example:** Put label `SDA` on the MPU-6050 SDA pin wire.
Put label `SDA` on the DRV2605L SDA pin wire.
They are now connected even though they're not physically joined by a wire.

---

## Step 7 — How to Add a Resistor or Capacitor

1. Press `A` → search "R" for resistor or "C" for capacitor
2. Choose from the Device library (Device:R or Device:C)
3. Place it
4. Press `E` to edit → set Value field to "4.7k" or "100n" etc

**For bypass caps:** place them right next to the IC they bypass.
**For pull-up resistors:** place them between the signal net and +3V3.

---

## Step 8 — No-Connect Markers

Unused pins must have a no-connect marker (X) so KiCad doesn't flag them as errors.

1. Press `Q` → no-connect tool
2. Click on any pin that is intentionally unconnected

Do this for all unused GPIO pins on the ESP32-S3.

---

## Step 9 — Running ERC (Electrical Rules Check)

After finishing each block, run ERC to catch mistakes.

Inspect → Electrical Rules Checker → Run ERC

Common errors and fixes:
| Error | Fix |
|---|---|
| Pin not connected | Add wire or no-connect marker |
| Power pin not driven | Add PWR_FLAG symbol to +3V3 and GND nets |
| Net has no driver | Check power symbols are correct |
| Duplicate reference | Two components have same ref — renumber |

**Target: zero ERC errors before moving to PCB.**

---

## Step 10 — Assigning Footprints

Every schematic symbol needs a PCB footprint assigned to it.
Without this, the PCB editor doesn't know what physical shape to use.

Tools → Assign Footprints

For each component, find the footprint:
| Component | Footprint to assign |
|---|---|
| Resistors 0402 | Resistor_SMD:R_0402_1005Metric |
| Caps 0402 | Capacitor_SMD:C_0402_1005Metric |
| Caps 0805 | Capacitor_SMD:C_0805_2012Metric |
| MCP73831 | Package_TO_SOT_SMD:SOT-23-5 |
| AP2112K | Package_TO_SOT_SMD:SOT-23-5 |
| MPU-6050 | Download from SnapEDA |
| DRV2605L | Download from SnapEDA (WSON-12) |
| MAX98357A | Download from SnapEDA (WLP-16) |
| ESP32-S3-MINI-1U | Download from SnapEDA or Espressif |

---

## Step 11 — Exporting Netlist to PCB

When schematic is complete and ERC is clean:

Tools → Update PCB from Schematic

This pushes all components and their connections into the PCB editor.
Components appear as a pile in the middle — you then arrange and route them.

---

## Step 12 — PCB Editor Basics (do after schematic)

### Essential keyboard shortcuts

| Key | Action |
|---|---|
| `X` | Route track (draw copper trace) |
| `R` | Rotate |
| `F` | Flip to other side |
| `E` | Edit properties |
| `U` | Select connected track |
| `I` | Select entire net |
| `Ctrl+B` | Fill all zones (pour copper) |
| `D` | DRC (design rule check) |

### PCB layer guide

| Layer | Colour | Purpose |
|---|---|---|
| F.Cu | Red | Top copper — component side |
| B.Cu | Blue | Bottom copper — solder side |
| F.Silkscreen | Yellow | Component labels on top |
| F.Courtyard | Pink | Component keepout boundary |
| Edge.Cuts | Yellow | Board outline |

### Trace width rules for this board

| Net type | Min width |
|---|---|
| Signal (SDA, SCL, GPIO) | 0.15mm |
| 3.3V power rail | 0.5mm |
| Battery / speaker | 1.0mm |
| USB D+/D- | 0.2mm matched length |

---

## Step 13 — Board Outline

Before placing any components, draw the board outline.

1. Select layer: Edge.Cuts
2. Place → Rectangle
3. Draw 45×35mm rectangle
4. This is your board boundary — nothing goes outside it

---

## Step 14 — Component Placement Order

Place in this order (most critical first):

1. **ESP32-S3-MINI-1U** — centre-top, antenna overhanging or at board edge
2. **MPU-6050** — centre of board, away from switching ICs
3. **DRV2605L** — near I2C bus traces
4. **MAX98357A** — near speaker connector, bottom-right area
5. **MCP73831 + AP2112K** — near USB-C connector
6. **USB-C J1** — bottom edge
7. **Battery J2** — top edge, opposite USB-C
8. **Buttons SW1, SW2** — front face (left edge)
9. **Speaker LS1** — right edge, facing outward
10. **All passives** — next to their respective ICs

---

## Step 15 — Antenna Keep-Out Zone

This is critical and easy to forget.

1. Select layer: F.Cu (or any copper layer)
2. Place → Zone → choose "No copper zone" (keep-out)
3. Draw a rectangle covering 3mm below the antenna edge of U1
4. Set it to exclude all copper layers

Without this, your BLE/WiFi range will be terrible.

---

## Step 16 — Running DRC (Design Rule Check)

Inspect → Design Rules Checker → Run DRC

Common errors:
| Error | Fix |
|---|---|
| Clearance violation | Traces too close — move or reroute |
| Unrouted net | A connection from schematic not yet drawn |
| Footprint outside board | Move component inside Edge.Cuts |
| Silkscreen overlap | Move labels |

**Target: zero DRC errors before exporting Gerbers.**

---

## Step 17 — Exporting Gerbers for JLCPCB

File → Fabrication Outputs → Gerbers

Settings for JLCPCB:
- Include layers: F.Cu, B.Cu, F.Silkscreen, B.Silkscreen, F.Mask, B.Mask, Edge.Cuts
- Format: Gerber X2
- Output folder: `hardware/kicad/posture_tracker_v2/gerbers/`

Also export:
- Drill files: File → Fabrication Outputs → Drill Files
- BOM: File → Fabrication Outputs → BOM
- Component placement: File → Fabrication Outputs → Component Placement

Zip the gerbers folder → upload to jlcpcb.com

---

## Order of Work — Session by Session

```
Session 1: Install KiCad, create project, download missing symbols from SnapEDA
Session 2: Draw Block 1 (Power) — USB-C, MCP73831, AP2112K, LiPo connector
Session 3: Draw Block 2 (MCU) — ESP32-S3-MINI-1U, bypass caps, GPIO assignments
Session 4: Draw Block 3+4 (IMU + Haptic) — MPU-6050, DRV2605L, shared I2C
Session 5: Draw Block 5 (Audio) — MAX98357A, speaker connector
Session 6: Draw Block 6+7 (Buttons, LED, VBAT sense)
Session 7: Run ERC, fix all errors, assign all footprints
Session 8: Export netlist to PCB, draw board outline, place components
Session 9: Route all traces, pour ground plane
Session 10: Run DRC, fix errors, export Gerbers
```

---

## Resources

- KiCad official docs: docs.kicad.org
- SnapEDA (symbols + footprints): snapeda.com
- Espressif KiCad library: github.com/espressif/kicad-libraries
- JLCPCB design rules: jlcpcb.com/capabilities/pcb-capabilities
- Phil's Lab KiCad tutorial (YouTube): search "Phil's Lab KiCad 8"
