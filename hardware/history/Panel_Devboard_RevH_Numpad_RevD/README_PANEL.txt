PANEL - U595 DEVBOARD REV H + NUMPAD_TMR1 REV D
(new project; Devboard_RevH and Numpad_TMR1_RevD are untouched)
================================================================================

WHAT IT IS
 - One KiCad board file (Panel_DevH_NumpadD.kicad_pcb) holding both boards.
     - They are joined by snap-off mouse-bite tabs.
     - There is a 5 mm rail on the left and on the right.
 - Panel size: 123.5 x 107.2 mm.
 - Built from your latest saves:
     - Devboard_RevH, saved Sep 28 03:22. Its copper is identical to the Rev H I delivered.
     - Numpad_TMR1_RevD, saved Sep 28 16:44. This includes your new J2/J3 stabilizer pins,
       the ENC_SW/ENC_DT reroute and the resized outline.

LAYOUT (looking at the side JLC assembles)
   | rail | 2 mm | dev board, USB-C at the bottom | 2 mm | numpad, flipped | 2 mm | rail |
 - Both ends of the dev board face open air, so nothing gets in the way of:
     - the USB-C connector, which sticks out past the board edge;
     - the UART header at the other end.
 - The numpad is lined up with the top of the dev board.
 - RENDER_top.png and RENDER_bottom.png show both sides.

WHY THE NUMPAD IS FLIPPED
 - All 98 numpad parts were on its bottom side.
 - In the panel the numpad is mirrored, so its bottom faces the same way as the dev board's top.
 - It is physically the same board: nothing moved relative to the board, and the switches still
   go in from the other side.
 - Result:
     - every part JLC places (163 placements) is on one side;
     - the bottom paste layer is empty, so there is one stencil and single-sided assembly.
 - Only the dev board's J2, J3, J9 and J10 sit on the other side. Those are the breadboard pins,
   which you hand-solder anyway.

WHAT IS DIFFERENT FROM THE SOURCE BOARDS (in this copy only)
 1. Numpad references are +100, so every reference in the BOM/CPL is unique:
    U1 -> U101, MX1 -> MX101, CL2 -> CL102, LED1 -> LED101, J1 -> J101, ENC1 -> ENC101, and so on.
    The dev board keeps its own references.
 2. Net names have a DEV- or NUM- prefix. Each board's GND/+3V3 stays its own net, so DRC does
    not report the two boards as unconnected from each other.
 3. The numpad is now part of a 4-layer board.
     - The panel uses the dev board's JLC04161H-3313 stackup.
     - The numpad's two inner layers are empty.
     - Electrically it is unchanged.
 4. One fix to the numpad:
     - Your outline resize left the ENC_SW trace along the top edge only 0.09 mm from the edge.
       That is a DRC error in Numpad_TMR1_RevD as saved.
     - Here that run (and its 45-degree lead-in) is moved 0.24 mm inboard, giving 0.33 mm.
     - The original file still has the error.

TABS
 - Mouse bites follow JLC's mouse-bite guide:
     - 0.6 mm holes at 1.0 mm pitch (0.4 mm web);
     - hole centres 0.2 mm inside the board edge;
     - 5 mm tabs with 7 holes each (one 3 mm tab with 5 holes, see below);
     - 2 mm routed gaps, 1 mm mill fillets.
 - 9 tabs, 12 rows of holes: dev to left rail x3, dev to numpad x3 (holes on both boards), and
   numpad to right rail x3.
 - Tabs are placed only where the board edge has at least 0.8 mm clear of copper. On the dev
   board that means:
     - above J9/J10;
     - between J8 and header pin 1;
     - below pin 26 (this one is a 3 mm tab, so its holes clear J3's silkscreen).
   There are no tabs along the 2.54 mm header rows.
 - To separate: bend each board along its slot, then sand the small nubs left by the holes.

RAILS
 - 3 tooling holes, 1.152 mm (JLC's size), and 3 fiducials (1 mm copper, 2 mm opening).
 - "JLCJLCJLCJLC" is printed on the left rail. JLC puts its order number there instead of on your
   boards; choose "Specify a location" when ordering.

CHECKS
 - Connectivity: every pad is on the same net as in its source board, and the panel has the same
   track and via counts as the two boards:
     - dev board: 99 nets, 944 tracks, 169 vias;
     - numpad: 63 nets, 781 tracks, 211 vias.
 - DRC:
     - 0 errors, 0 unconnected.
     - Mouse-bite holes clear all copper.
 - DRC warnings:
     - 31 courtyard overlaps and 77 silkscreen warnings, all on the numpad. The numpad's own
       project ignores these.
     - For each courtyard pair (diode/socket, LED/sensor, encoder/J103) I compared the part
       bodies (fab outlines). None collide.
     - Footprint-library warnings: the footprint libraries are not linked to this project.
 - Via-in-pad: none (380 vias).

ORDERING AT JLC
 PCB
  - Upload production/Panel_DevH_NumpadD_gerbers.zip.
  - 4 layers, 1.6 mm, impedance control ON, stackup JLC04161H-3313 (needed for the dev
    board's 90 ohm USB pair).
  - Delivery format: "Panel by Customer". Set the number of different designs to 2. JLC charges
    extra for more than one design, and the calculator shows the amount.
  - Remove order number: "Specify a location".
 Assembly
  - Economic PCBA, top side only, tooling holes added by customer.
  - Assemble 2 panels (JLC's minimum). That gives 2 dev boards + 2 numpads.
  - Upload production/bom.csv and production/positions.csv.
  - Check every rotation in JLC's preview. Pay extra attention to the numpad parts
    (SK6812MINI-E LEDs, SOD-323 diodes, hot-swap sockets, encoder), since the numpad has never
    been through JLC's preview.
  - Parts to deal with before ordering:
      - U101-U115 (TMR2615D): no LCSC number, the same as in Rev D. Consign them or leave them
        unplaced.
      - U1 STM32U595RJT6 (C28563722): low stock. Reserve it in your JLC parts library, 1 per panel.
  - Not in the BOM; hand-solder these:
      - dev board: J2/J3 (1x26), J8 (2x10), J9/J10 (single pins);
      - numpad: J101 (1x31), J102/J103 (single pins).

COST
 - What gets cheaper:
     - one PCB order instead of two;
     - one assembly setup fee;
     - one stencil, top side only;
     - one shipment.
 - What gets more expensive:
     - the numpad's area is now billed as 4-layer instead of 2-layer;
     - the rails add area;
     - JLC charges for the second design.
 - Before committing, compare this quote with two separate orders in JLC's calculator.

SCHEMATICS
 - The panel is a PCB-only project, which is the normal way to do panels.
     - The schematics stay in Devboard_RevH and Numpad_TMR1_RevD.
     - Make changes there, then regenerate the panel. scripts/ has the three steps (they need
       KiKit), or ask me.
 - Heads-up: the Devboard_RevH schematic as saved on Sep 28 no longer matches its PCB.
     - 15 pins no longer reach their nets: C3/VCAP, the BOOT0 network (R7/SW2), D1/R5, the
       USB-C CC and SBU pins, and J7 pins 7-8.
     - ERC on that file now reports 5 dangling-wire errors: tiny wire scraps,
       0.01-0.25 mm long, that connect to nothing.
     - The PCB copper is fine, so the panel is not affected.
     - Don't run "Update PCB from Schematic" in that project until the schematic is fixed. I can
       repair it in a copy if you want.

FILES
 Panel_DevH_NumpadD.kicad_pro / .kicad_pcb / .kicad_prl / .kicad_dru
 production/  Panel_DevH_NumpadD_gerbers.zip, gerbers/, bom.csv (30 lines), positions.csv (163)
 RENDER_top.png, RENDER_bottom.png
 scripts/     prep_numpad.py (flip + renumber + fix), build_panel.py (KiKit), prod_panel.py
