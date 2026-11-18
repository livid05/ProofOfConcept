# TriMode60 Rev A: JLC CPL rotation and position corrections

Board: `route/r3.kicad_pcb`. All 457 placed parts are on the bottom (B.Cu, flipped). prod2.py applies everything below (the `CORR` table at its top).

## 1. Convention used

- JLC rotation is in degrees, CCW positive ([JLC: Pick & Place File](https://jlcpcb.com/help/article/pick-place-file-for-pcb-assembly)). Mid X/Mid Y is the component centroid in mm. prod2.py writes top-view board coordinates, absolute origin with Y negated, the same frame as the gerbers and drill files (absolute origin).
- Top-side CPL rotation = `(kicad_rot + c) mod 360`.
- Bottom-side CPL rotation = `(180 - kicad_rot + c) mod 360`, where `c` is the same correction a top-side part would get. It is not negated.

Four tools agree on the bottom-side formula:

| Tool | Code | Bottom rotation |
|---|---|---|
| JLC Fabrication Toolkit (bennymeg), `plugins/process.py` | `if layer == 'bottom': rotation = (180.0 - rotation)` then `rotation += rotation_offset_db` then `+ manual offset`, `% 360` | 180 - rot + c |
| kicad-jlcpcb-tools (Bouni), `fabrication.py` `fix_rotation()` | `if footprint.GetLayer() != 0: rotation = (180 - rotation) % 360` ("bottom angles need to be mirrored on Y-axis"), then `rotate()` = `(rotation + correction) % 360` | 180 - rot + c |
| KiBot `rot_footprint` filter, JLC settings (`mirror_bottom: true`, `negative_bottom: false`) | `footprint_rot = 180 - footprint_rot; footprint_rot += angle`. The source comment says: "The real value is (180-comp.footprint_rot)+angle" | 180 - rot + c |
| KiKit `footprintOrientation(..., MirrorBottom)` | `(180 - footprint.GetOrientation().AsDegrees() + compensation[2]) % 360` | 180 - rot + c |

- Empirical check: [KiKit discussion #664](https://github.com/yaqwsx/KiKit/discussions/664). JLC placed bottom parts at 0 and 180 degrees correctly but rotated the 90-degree parts wrongly until the tool switched to `180 - Rotation`. The maintainer's comment: "JLC PCB has changed the interpretation of the POS files".
- Why this is right: KiCad stores a flipped footprint as the library footprint mirrored top-to-bottom, then rotated by `rot`. JLC views the bottom side flipped left-to-right. Composing the two mirrors gives a 180-degree rotation, so the part seen from below is the unmirrored library footprint rotated by `180 - rot`. JLC also sees the part from its own top side, so the part-vs-library offset `c` has the same sign as on the top side.
- Dissent: [kicad-jlcpcb-tools #636](https://github.com/Bouni/kicad-jlcpcb-tools/issues/636) reports one bottom QFN that needed a different value. It has no maintainer resolution, so treat it as another reason to check the preview.
- Verified here: prod2.py's footprint transform (flipped = mirror in Y, then rotate) matches KiCad's pad positions exactly (self-check error 0.000 mm). The mirror convention was checked on SW1 (library pad 1 at (-7.085, +2.54) reads back as (-7.085, -2.54)).

## 2. Position (centroid)

- The Fabrication Toolkit uses the footprint anchor for SMD footprints.
- kicad-jlcpcb-tools uses the bounding box of all pads, including NPTH holes. That would be wrong on this board:
  - hot-swap socket: 1.5 mm off, because the switch holes are counted;
  - BMD-340: 2.0 mm off, because its pads are not centred on the body.
- prod2.py uses the anchor plus an explicit library-frame offset. That offset is non-zero only for the hot-swap socket: (-0.622, +3.810) mm, the centre of the two socket pads, which is the socket body centre (the body Fab outline gives (-0.60, +3.80)). The footprint origin is the switch centre. prod2.py was checked against the pad bounding box of all 61 sockets (max difference 0.0005 mm).
- For every other footprint the anchor is the body centre: SMD footprints under KiCad's library rules, and the custom parts by construction. The USB-C, JST and MSK12C02 anchors are the KiCad body/pad centres. JLC's own model centre may differ by up to about 0.6 mm; JLC engineers fix that routinely.

## 3. Footprint table

`c` = correction applied. "CPL" = the rotation prod2.py writes (KiCad rotation → CPL, all bottom side).
Sources: **FT** = Fabrication Toolkit `transformations.csv`; **JKT** = JLCKicadTools `cpl_rotations_db.csv` (the file kicad-jlcpcb-tools tells users to import into its Rotation manager); **KB** = KiBot `DEFAULT_ROTATIONS`. "none" = no pattern matches this footprint name.

| Footprint | Parts (refs) | c | KiCad → CPL | Source | Confidence |
|---|---|---|---|---|---|
| R_0402_1005Metric | 84 resistors | 0 | 0→180, 90→90, 180→0, 270→270 | FT/JKT/KB none; non-polar | High |
| C_0402_1005Metric | 148 caps | 0 | 0→180, 90→90, 180→0, 270→270 | none; non-polar | High |
| C_0603_1608Metric | 9 caps | 0 | 0→180, 90→90 | none; non-polar | High |
| C_0805_2012Metric | 4 caps (22 uF) | 0 | 0→180, 90→90, 180→0 | none; non-polar | High |
| L_0603_1608Metric | FB1 | 0 | 0→180 | none; non-polar | High |
| Fuse_1206_3216Metric | F1 | 0 | 180→0 | none; non-polar | High |
| L_Changjiang_FNR4020S | L1 | 0 | 180→0 | none; non-polar | High |
| Crystal_SMD_2012-2Pin | Y2 32.768 kHz | 0 | 180→0 | none; 2-pin, non-polar | High |
| Crystal_SMD_3225-4Pin | Y1 16 MHz (C13738) | 0 | 0→180 | none. A 180-degree error is harmless (1↔3 crystal, 2↔4 GND); a 90-degree error is not | Medium |
| SOT-23 | Q1 AO3401A, D62 BAT54C | 180 | 180→180, 270→90 | FT 180, KB 180; JKT -90 (older outlier) | Medium |
| SOT-23-5 | U9 SN74LV1T125 | 180 | 180→180 | FT/KB `^SOT-23` 180; JKT -90 | Medium |
| TSOT-23-5 | U6 RT9080 | 180 | 180→180 | FT 180, JKT 180, KB 180 | Medium |
| SOT-23-6 | U3 USBLC6-2SC6 | 180 | 90→270 | FT/KB 180; JKT -90; Bouni #472 (USBLC6-2SC6) expected 180 | Medium |
| SOT-563 | U7 TPS61023DRL | 180 | 180→180 | none. By analogy: same 3+3, pin-1 top-left layout as SOT-23-6 | **Low** |
| SOT-583-8 | U8 TPS2116DRL | 180 | 180→180 | none. By analogy with SOT-563 (same TI DRL family) | **Low** |
| VQFN-16-1EP_3x3mm | U4 BQ24073RGT | 270 | 180→270 | JKT/KB 270 (`^(.*?_\|V)?QFN-(16\|…)`); FT none (its `^QFN-` does not match) | **Low** |
| TSSOP-16_4.4x5mm_P0.65mm | U201-U208 CD74HC4051PWR (8 pcs) | 270 | 180→270 | FT/JKT/KB `^TSSOP-` 270 | Medium |
| TDFN-8-1EP_2x2mm | U5 MAX17048 | 270 | 180→270 | none (`^DFN-` 270 in all three does not match `TDFN-`). By analogy | **Low** |
| LQFP-64_10x10mm | U1 STM32U595 | 270 | 90→0 | FT 270, JKT 270, KB 270 | Medium (the most expensive part: check anyway) |
| SW_SPST_B3U-1000P | SW63 BOOT, SW64 RESET | 90 | 180→90 | FT 90, JKT 90, KB 90 | Medium |
| JST_PH_S2B-PH-SM4-TB | J2 battery | 180 | 180→180 | FT 180, JKT 180, KB 180 | Medium (polarity matters) |
| USB_C_Receptacle_HRO_TYPE-C-31-M-12 | J1 (C165948) | 180 | 0→0 | JKT 180, KB 180; FT none | Medium |
| SW_SPDT_Shouhan_MSK12C02 | SW62 power | 0 | 0→180 | none | **Low** |
| K60:u-blox_BMD-340 (custom) | U2 | 0 | 90→90 | none (custom footprint) | **Low** |
| K60:LED_SK6812MINI-E_Reverse (custom) | D1-D61 (61 pcs) | 0 | 180→0 | none (custom) | **Low, high risk** |
| K60:SW_MX_Hotswap_Kailh_Magnetic_1u (custom) | SW1-SW61 socket (61 pcs, C41430893) | 0, offset (-0.622, +3.810) | 180→0 | none (custom); offset computed from the pads | **Low** |
| K60:TMR2615D_DFN3L_2x2mm (custom, consigned) | U101-U161 (61 pcs) | 0 | 0→180 | none; JLC will draw the part | **Low** |

Not in the CPL: TP1-TP13 test pads, H1-H4 mounting holes/slots, ST_* stabilizers (all excluded from BOM/position files, board-only or no attribute).

## 4. Check these in JLC's 2D/3D preview (in order of risk)

1. **D1-D61 SK6812MINI-E**: the pin-1/VDD mark must sit on the pad next to the B.SilkS triangle. Pin 1 (VDD) is diagonal to GND, so a 180-degree error reverses the supply on all 61 LEDs.
2. **U101-U161 TMR2615D (consigned)**: the large GND pad goes on the large land (with the neck). The pin-1 dot (VOUT) goes next to the silkscreen dot. A 180-degree error shorts VDD/VOUT to GND.
3. **U201-U208 CD74HC4051PWR (TSSOP-16)**: pin-1 dot on pad 1 (the pin rows must lie along the land rows).
4. **U4 BQ24073, U5 MAX17048**: the pin-1 corner. A QFN/DFN at 90 or 180 degrees off still "fits" the land, so look at the dot.
5. **U7 TPS61023 (SOT-563), U8 TPS2116 (SOT-583)**: the pin-1 dot.
6. **U2 BMD-340**: the antenna end at the board edge (the copper keep-out end), and the pin-1 corner.
7. **SW1-SW61 hot-swap sockets**: the body lies over both pads, and the socket body sits between the switch holes, not on the switch centre.
8. **J1 USB-C**: the opening faces the board edge and the shell legs are in the plated slots. **J2 JST-PH**: the cable entry faces outward and pin 1 is VBAT. **SW62 MSK12C02**: the lever overhangs the edge and the pegs are in the NPTH holes.
9. **U1 LQFP-64, U3/U6/U9 SOT-23-x, Q1/D62 SOT-23, Y1, SW63/SW64**: quick look at the pin-1 dot or outline.

JLC's FAQ says their engineers correct rotation and polarity "based on the silkscreen markings" ([PCB Assembly FAQs part 2](https://jlcpcb.com/help/article/pcb-assembly-faqs-part-2)). Keep the pin-1 marks on B.SilkS visible (not covered by the ref text or clipped by the mask).

## 5. Sources

- Fabrication Toolkit: https://github.com/bennymeg/Fabrication-Toolkit (`plugins/process.py`, `plugins/transformations.csv`)
- kicad-jlcpcb-tools: https://github.com/Bouni/kicad-jlcpcb-tools (`fabrication.py`), issues #472 and #636
- JLCKicadTools rotation DB: https://github.com/matthewlai/JLCKicadTools/blob/master/jlc_kicad_tools/cpl_rotations_db.csv
- KiBot: https://kibot.readthedocs.io/en/latest/notes_position.html, `kibot/fil_rot_footprint.py`, `kibot/misc.py` (DEFAULT_ROTATIONS)
- KiKit: `kikit/fab/common.py`, discussion https://github.com/yaqwsx/KiKit/discussions/664
- JLC: https://jlcpcb.com/help/article/pick-place-file-for-pcb-assembly, https://jlcpcb.com/help/article/pcb-assembly-faqs-part-2
- I could not read the EasyEDA/JLC library footprints (their API is blocked for fetching), so the rows marked "by analogy" or "custom" are not checked against JLC's own model.
