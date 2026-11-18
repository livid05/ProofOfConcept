NUMPAD_TMR1 - REV E
================================================================================

WHAT CHANGED (from your latest Numpad_TMR1_RevD save, Sep 28 16:44)
 - One fix:
     - After your outline resize, the ENC_SW trace along the top edge sat 0.09 mm from the
       board edge. That is a DRC error.
     - The run and its 45-degree lead-in moved 0.24 mm inboard (y 56.021 -> 56.26), giving
       0.33 mm to the edge.
 - Production files regenerated from this board. They now include your J2/J3 stabilizer pins,
   the ENC reroute and the new outline.
 - Everything else is exactly as you saved it.

ORDERED ON ITS OWN, NOT IN A PANEL
 - JLC charges extra for a panel holding two different designs:
     - PCB "Panel" fee: $16.54;
     - engineering fee: $25;
     - assembly panel fee: $8.21.
   That costs more than ordering the two boards separately.
 - On its own the numpad stays a cheap 2-layer board: 81.7 x 86.8 mm, $4.00 for 5 boards
   (quoted Oct 2 2026).
 - All parts JLC places are on the bottom side, so it is still single-sided assembly.

CHECKS
 - DRC: 0 errors, 0 unconnected.
     - Warnings are only footprint-library notes and one silkscreen-at-edge note (J1's reference).
 - The schematic still lacks CL2-CL15 and LC16, which exist only on the PCB. That was already
   true in Rev C/D, and the BOM below includes them.

PRODUCTION (production/)
 - Numpad_TMR1_RevE_gerbers.zip (2-layer), gerbers/.
 - bom.csv:
     - J1 (1x31 header) and J2/J3 (single stabilizer pins) are left out; hand-solder them.
     - U1-U15 (TMR2615D) have no LCSC number. Consign them or leave them unplaced.
 - positions.csv:
     - 95 placements, all bottom side;
     - sensors and hot-swap sockets use their real part centres, the same as Rev D.
 - In JLC's preview, check the rotations of the SK6812MINI-E LEDs, the SOD-323 diodes and the
   hot-swap sockets.

ORDERING AT JLC
 - 2 layers, 1.6 mm, FR4 TG135, Single PCB, 5 pcs. The quote should show "Special Offer".
 - Economic PCBA, assemble the bottom side.
