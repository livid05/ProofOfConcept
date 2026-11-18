NUMPAD_TMR1 - REV B
================================================================================

BASED ON
 The version in the Keypad Firmware project zip (production files dated 2026-04-03),
 i.e. the board you had fabricated. The Numpad_TMR1 folder under Capstone Keyboard Design
 is an older Mar 31 save, so it was not used.

WHAT CHANGED
 1. Sensor land for the TMR2615D (U1-U15)
    Rev A's sensor land was sized for the 1.6 x 1.6 mm TMR2615F. Your parts are the 2 x 2 mm
    TMR2615D, so their two small pins landed mostly off the pads. That is why every sensor
    output read "open".
    New footprint: TMR2615D:SW_MX_TMR2615D_1u, built from datasheet Figure 7 (DFN3L 2x2x0.55):
      - pin 1 = VOUT, pin 2 = VDD: 0.30 mm terminals at 1.30 mm pitch.
        Pads are 0.35 x 0.80 mm and reach 0.4 mm past the body, so you can hand solder them.
      - pin 3 = GND: the large pad (1.50 x 0.98 mm) plus its neck. The neck pad also reaches
        0.4 mm past the body, so an iron can heat the big pad from outside.
      - The pin mapping is unchanged from Rev A (1 VOUT, 2 VDD, 3 GND, the D-package order).
        It was already right for the D. Only the size was wrong.
      - The package centre is in the same place as before. The sensing element is 0.49 mm from
        the pin-3 edge, about 3.1 mm from the switch centre. Its sensing axis still points at
        the switch centre (radially).
    The plain part footprint TMR2615D:TMR2615D_DFN3L_2x2mm is also in the library, for other
    boards. The 60% keyboard uses it.
 2. Seven traces rerouted around the larger pads: TMR_OUT1, 5, 9, 10, 13, 14 and one +3.3V
    stub at U4. All are bottom layer, 0.2 mm wide.
 3. J2 = GND pin, exactly one 0.1" pitch left of J1 pin 1, with a "GND" silkscreen label.
    Rev A had no ground on the header, which is why you needed the jumper wire. J1's pinout is
    unchanged (pins 1-30 are the same), so your breadboard wiring still works; just add one more
    row for GND.
 4. Schematic:
      - The sensor symbol is renamed TMR2615D-AAC, value TMR2615D-ACC-1.000-500.
      - J2 has been added.
      - The rev note is on the sheet.
    The TMR2615D symbol and footprints now ship in this folder (TMR2615D.kicad_sym,
    TMR2615D.pretty) and are registered in the project library tables.

CHECKS
 - DRC: 0 errors, 0 unconnected.
   The only warnings are "library not found" for your personal libraries (Mars_Library,
   marbastlib), which aren't installed on my side. On your PC they will resolve.
 - Schematic parity: the only mismatches are the same 15 as Rev A. CL2-CL15 and LC16 are on
   the PCB but not in the schematic. J2 and U1-U15 match.
 - ERC: unchanged from Rev A. There are still 3 "power pin not driven" errors (the schematic has
   no PWR_FLAG on +5V/+3.3V/GND) and 1 dangling wire near LED12. I left your ERC items alone.
 - sensor_before_after.png shows the old and new land on U1.

PRODUCTION (production/)
 - Numpad_TMR1_RevB_gerbers.zip (2-layer, same stack-up as Rev A), bom.csv, positions.csv.

LCSC PART NUMBERS
 - Every schematic symbol and PCB footprint now carries an "LCSC" field, so a BOM regenerated from
   KiCad or the JLC Fabrication Toolkit keeps the part numbers. production/bom.csv already has them.
 - U1-U15 (TMR2615D) have no LCSC number on purpose: they are customer-supplied (consigned) parts.
   The CL2-CL15 / LC16 capacitors exist only on the PCB (same as Rev A), and their footprints carry C1525.

SOLDERING THE TMR2615D
 - Flux, then tin the three pads lightly. Place the chip with the large pad over the large
   land; pin 1 (VOUT) is the pad with the silkscreen dot.
 - Hot air (about 300-330 C) until it self-centres. Or with an iron, heat each toe pad from
   outside the chip.
 - Check before you connect it: VOUT to GND should read about 1.65 V (half of 3.3 V) with no
   magnet near.
