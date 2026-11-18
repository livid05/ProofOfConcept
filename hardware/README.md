# Hardware

KiCad projects, production files and revision notes. Open the `.kicad_pro` files in KiCad 9 or later. Library tables and the custom TMR2615D symbol and footprint are included with the numpad project.

## Latest revisions

| Folder | Board | Revision | Notes |
|---|---|---|---|
| `devboard-u5a5/RevK/` | STM32U5A5 development board, 4 layers, on-board 16:1 mux and 2x10 sensor header (J8), USB-C high speed | K | `production/` holds gerbers, BOM and positions. Read `README_REVK.txt` first |
| `numpad-tmr/RevE/` | 15-key TMR numpad, 2 layers, hot-swap MX sockets, SK6812 LEDs | E | Production variants: `production`, `production_economic`, `production_economic_handsolder`, `production_jlc_fixed`; each has a README |
| `trimode60/RevE/` | 60% tri-mode keyboard (STM32U595, nRF52840 module, charger, PD sink), 4 layers | E | Design only. Per-sheet schematics: `mcu`, `power`, `keys`, `radio`. Renders in `renders/` |

Hand-solder parts are listed in each board's README (for the dev board: J2, J3, J8, J9, J10; for the numpad: J1, J2, J3). The sensors (U1 to U15 on the numpad) have no LCSC number and are fitted by hand.

## History

[`history/`](history/) holds the README or change notes for each earlier revision (dev board B to J, numpad B to D, TriMode60 A to D, and the panel experiment). They are the source for [docs/02-hardware-evolution.md](../docs/02-hardware-evolution.md).

## Images

[`images/`](images/) holds layout comparisons and renders exported from the KiCad projects during each revision, grouped by board.

| Image | Shows |
|---|---|
| `images/devboard/RevB_layout.png`, `RevC_layout.png`, `RevD_layout.png` | Early layouts: MCU move, on-board mux, stabilizer pins |
| `images/devboard/RevI_vs_RevH_top.png`, `RevJ_vs_RevI_USB.png`, `RevH_UART_before_after.png` | Later layout changes |
| `images/numpad/sensor_before_after.png`, `vip_cleanup_before_after.png`, `vip_cleanup_details.png`, `j2_check.png` | Sensor footprint fix and via-in-pad cleanup |
| `images/trimode60/TriMode60_*.png` | 3D renders and copper layers of Rev E |
| `images/panel/RENDER_top.png`, `RENDER_bottom.png` | The dev board plus numpad panel experiment |

These are not schematic exports. To get schematic PDFs or fresh board renders, run KiCad's export on the projects above.

## Earlier work

The `schematics/` folder at the repository root holds the earlier KiCad projects from the RP2040 stage (capstone design, matrix and mux, numpad TMR1 first version). They are kept for the record; the folders here supersede them.
