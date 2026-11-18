U595 DEVBOARD - REV I
================================================================================

WHY
 - JLC prices a single 4-layer design at or under 102 x 102 mm as an $8 special (5 boards).
 - Anything larger pays a $25 engineering fee plus a board-area charge.
 - Rev H was 25.85 x 107.2 mm, so the dev board alone cost about $26.90 for 5 boards.
 - Rev I is 25.85 x 101.6 mm, which comes to $8.10 (quoted on jlcpcb.com, Oct 2 2026).

WHAT CHANGED (from your latest Devboard_RevH save; nothing below the mux moved)
 1. Top edge moved down 5.6 mm (board 107.2 -> 101.6 mm).
 2. J5 UART connector moved down 5.6 mm with the edge.
     - It is still flush with the top edge, so the cable plugs in exactly as before.
     - Its GND and 3V3 vias moved with it.
 3. To make room for J5's pads, the mux block moved down 1.2 mm, along with its tracks, vias
    and silkscreen:
     - J8;
     - the 16 RC filters (R13-R28, C26-C41);
     - C25;
     - U5.
    It used empty space above header pin 1.
 4. J9/J10 stabilizer pins did NOT move. They are still exactly 11 pitches (27.94 mm) above
    header pin 1, so they still drop into the breadboard.
 5. PA2 (TX) and PA3 (RX) from J5 were rerouted:
     - each pin drops to a via just below J5;
     - the vias sit in the gaps between J8's top-row labels;
     - B.Cu runs then join the old PA2/PA3 paths.
    Both traces got shorter (PA2 85.2 -> 79.8 mm, PA3 87.9 -> 83.1 mm).
 6. J5's pin labels moved beside the connector, because their old spot is now under it:
     - "UART" and "RX TX" on the left;
     - "3V3" and "GND" on the right.
    The pin order left to right is still RX, TX, 3V3, GND.
 7. The back label now reads "U595 DEVBOARD REV I".
 - RevI_vs_RevH_top.png compares the top end before and after.

UNCHANGED
 - The schematic connections and every part.
 - The USB section and its 90 ohm pair: D+ is 12.757 mm and D- is 12.289 mm, the same as Rev H.
 - The MCU area, the headers, and everything from header pin 1 down.

SCHEMATIC NOTE
 - This folder uses the last clean Rev H schematic, not your Sep 28 re-save.
     - That re-save has 5 tiny dangling wire scraps (ERC errors), 0.01-0.25 mm long.
     - KiCad drops 9 nets from it: C3/VCAP, BOOT0 (R7/SW2/PH3), D1/R5, the USB-C CC
       resistors, and the J1 SBU / J7 pin 7-8 no-connects.
 - Apart from that, the two files have the same parts, labels and wiring. The re-save only
   changed formatting.

CHECKS
 - ERC: 0.
 - DRC with schematic parity:
     - 0 errors, 0 unconnected, 0 parity issues;
     - the only warning is the old Y2 footprint-library note.
 - Every pad is on the same net as in Rev H.
 - Via-in-pad: none (169 vias).
 - production/bom.csv is identical to Rev H.
 - positions.csv changes only for the parts that moved: J5, J8 area, R13-R28, C25-C41, U5.
 - Gerber job size: 25.85 x 101.6 mm.

ORDERING AT JLC (PCB)
 - 4 layers, 1.6 mm, FR-4. Material Type: FR4 TG135 (free; S1000H TG155 adds about $8.81).
 - Specify Stackup: Yes -> JLC04161H-3313. This is free and gives the 90 ohm USB geometry.
 - Impedance Control: "No requirement". The +-10% guarantee costs $33.08. The stackup sets the
   impedance either way; the guarantee only adds JLC's control and test.
 - Single PCB, 5 pcs. Confirm the quote shows "Special Offer" and no engineering fee.
 - Assembly: Economic PCBA, top side, upload production/bom.csv and positions.csv.
 - STM32U595RJT6 (C28563722) is still low-stock. Reserve it in your parts library first.
 - Hand-solder, not in the BOM: J2/J3 (1x26 headers, bottom), J8 (2x10), J9/J10 (single pins).

FILES
 Capstone Keyboard Design.kicad_pro / .kicad_sch / .kicad_pcb / .kicad_dru / .kicad_prl
 production/  Capstone_Keyboard_Design_RevI_gerbers.zip, gerbers/, bom.csv, positions.csv
 README_REVH.txt, README_REVG.txt, README_REVF.txt, README_REVE.txt, REVD_CHANGES.txt (history)
