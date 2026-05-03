# BOM (Bill of Materials)

## bom_v2_master.csv — master BOM (source of truth)

Full component list for PCB V2. Update this as components are confirmed.
Export JLCPCB-formatted BOM from KiCad when schematic is complete.

## Current confirmed components (as of 2026-05-03)

| Ref | Component | Part Number | Supplier | Unit Cost | Status |
|---|---|---|---|---|---|
| U1 | ESP32-S3-MINI-1-N8 | ESP32-S3-MINI-1-N8 | Espressif / Digikey | $2.50 | ⬜ Not ordered |
| U2 | MPU-6050 | MPU-6050 | Digikey 1428-1011-1-ND | $0.30 | ⬜ Not ordered |
| U3 | DRV2605L haptic driver | DRV2605LDGSR | Digikey 296-43883-1-ND | $1.20 | ⬜ Not ordered |
| U4 | MAX98357A audio amp | MAX98357AEWL | Digikey MAX98357AEWL+CT-ND | $1.50 | ⬜ Not ordered |
| U5 | MCP73831 LiPo charger | MCP73831T-2ACI/OT | Digikey MCP73831T-2ACI/OTCT-ND | $0.25 | ⬜ Not ordered |
| U6 | AP2112K-3.3 LDO | AP2112K-3.3TRG1 | Digikey AP2112K-3.3TRG1DICT-ND | $0.15 | ⬜ Not ordered |
| U7 | USBLC6-2SC6 ESD protection | USBLC6-2SC6Y | Digikey 497-5235-1-ND | $0.20 | ⬜ Not ordered |
| M1 | LRA motor | Vybronics VG1030001XH | Digikey 1670-VG1030001XH-ND | $2.96 | ✅ Ordered × 3 |
| LS1 | 20mm 8Ω 1W speaker | MECCANIXITY 20mm | Amazon | $2.12 | ✅ Ordered × 4 |
| BT1 | 350mAh LiPo | Adafruit #2750 | Digikey 1528-1858-ND | $6.95 | ✅ Ordered × 2 |
| — | LiPo charger breakout | Adafruit #1904 | adafruit.com | $6.95 | ✅ Ordered × 1 — Phase 9 validation only, not on PCB |
| J1 | USB-C connector | HRO TYPE-C-31-M-12 | LCSC C165948 / Digikey | $0.35 | ⬜ Not ordered |
| J2 | JST-PH 2-pin | JST-PH | Digikey 455-1719-ND | $0.10 | ⬜ Not ordered |
| SW1 | Button BOOT/CAL | Alps SKRPABE010 | Digikey SKRPABE010CT-ND | $0.10 | ⬜ Not ordered |
| SW2 | Button SNOOZE | Alps SKRPABE010 | Digikey SKRPABE010CT-ND | $0.10 | ⬜ Not ordered |
| SW3 | Button RESET | Alps SKRPABE010 | Digikey SKRPABE010CT-ND | $0.10 | ⬜ Not ordered |
| LED2 | Snooze indicator LED | Generic 0805 green | Digikey 160-1446-1-ND | $0.05 | ⬜ Not ordered |

## Passives (order with Phase 10)

| Value | Package | Digikey Part # | Qty | Status |
|---|---|---|---|---|
| 4.7kΩ resistor | 0402 | 311-4.7KLRCT-ND | 50 | ⬜ Not ordered |
| 10kΩ resistor | 0402 | 311-10.0KLRCT-ND | 50 | ⬜ Not ordered |
| 100kΩ resistor | 0402 | 311-100KLRCT-ND | 20 | ⬜ Not ordered |
| 5.1kΩ resistor | 0402 | 311-5.10KLRCT-ND | 50 | ⬜ Not ordered |
| 1kΩ resistor | 0402 | 311-1.00KLRCT-ND | 50 | ⬜ Not ordered |
| 100nF cap | 0402 | 311-1375-1-ND | 50 | ⬜ Not ordered |
| 1µF cap | 0402 | 311-1543-1-ND | 20 | ⬜ Not ordered |
| 4.7µF cap | 0402 | 399-C17 (C17 VBUS_FUSED) | 10 | ⬜ Not ordered |
| 10µF cap | 0805 | 399-8419-1-ND | 20 | ⬜ Not ordered |
| 22µF cap | 0805 | 399-8423-1-ND | 10 | ⬜ Not ordered |
| 47µF cap | 0805 | 399-8425-1-ND | 5 | ⬜ Not ordered |
| 500mA polyfuse | 0805 | MINISMDC050F/24-2CT-ND | 5 | ⬜ Not ordered |
| Green LED | 0805 | 160-1446-1-ND | 10 | ⬜ Not ordered |

## Estimated total BOM cost per unit

| Category | Cost |
|---|---|
| ICs | ~$6.00 |
| Passives | ~$1.00 |
| Connectors + buttons | ~$0.70 |
| Motor + speaker | ~$5.00 |
| Battery | ~$7.00 |
| **Total per unit** | **~$20** |
