U595 DEVBOARD - REV H
================================================================================

WHAT CHANGED
 - J5 (UART JST-SH, SM04B-SRSS-TB) is now centred left-to-right on the board.
     - Moved from x = 69.0 mm to x = 62.3 mm, the board's centreline.
     - It is 12.925 mm from each side edge.
     - Its height on the board (y = 16.95 mm) and its rotation are unchanged, so it still
       sits the same distance from the top edge.
 - Rerouted the 4 UART nets:
     - GND and +3V3: the short pad-to-via stubs and their plane vias moved with the connector.
     - PA3 (RX, pin 4): now drops to B.Cu right under its pad and joins the original
       westbound B.Cu trace, which is 6.7 mm shorter.
     - PA2 (TX, pin 3): now drops to B.Cu under its pad, runs east on B.Cu and joins the
       original trace at its diagonal, then continues down x = 69.85 as before.
 - The RX / TX / 3V3 / GND and "UART" silkscreen labels moved with the connector.
 - The board label now reads "U595 DEVBOARD REV H".
 - RevH_UART_before_after.png shows the change.

UNCHANGED
 - The schematic. It has no position data, so this is a PCB-only change.
 - All other parts, nets and copper.
 - The USB HS pair from Rev G.

CHECKS
 - DRC with schematic parity:
     - 0 errors, 0 unconnected, 0 parity issues;
     - the only warning is the same old Y2 footprint-library note as in Rev G.
 - ERC 0.
 - Via-in-pad 0 (169 vias).
 - bom.csv is identical to Rev G.
 - positions.csv: only the J5 line changed (69.0 -> 62.3).

PRODUCTION
 - Files are in production/ (gerbers zip, bom.csv, positions.csv) and are ordered the same way
   as Rev G.
 - The STM32U595RJT6 (C28563722) stock note from README_REVG.txt still applies.
