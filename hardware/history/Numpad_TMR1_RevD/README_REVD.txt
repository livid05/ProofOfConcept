NUMPAD_TMR1 - REV D
================================================================================

BASED ON
 Your latest Rev C save, including your own edits:
   - the sensor footprint moved north;
   - the extra board clearance;
   - the header clean-up (J2 merged into J1, now a 1x31).
 No footprint was moved. Parts, nets, zones, the outline and silkscreen are as you left them.

1. SENSOR SHIFT + REROUTE
 The shift in your files is 0.51 mm north, not 1 mm. The pads and the body both moved by the
 same amount, and the footprint origin is still the switch centre.
 Because of the shift, some tracks and vias no longer reached the pads, or sat too close:
   - 14 +3.3V vias;
   - the +3.3V feeds to the LC caps;
   - one or two stubs on every TMR_OUT1-15 line.
 What was redone:
   - Old copper removed: 130 track/via items.
   - New copper added: 170 items (about 220 mm of track, the same length as before).
   - Main work areas: the LC2-LC5, LC13 and LC15 cap feeds, and the TMR_OUT1/5/9/10/13/14 lines
     that now use a via pair to cross.
 Rules kept:
   - 0.2 mm clearance, 0.6/0.3 mm vias, 0.2 mm signal tracks, +3.3V bus at 0.25-0.5 mm;
   - no via in any pad (0 vias touching an SMD pad).
 Results:
   - DRC: 0 errors, 0 warnings, 0 unconnected;
   - no dangling track ends.

2. WHY THE SENSOR FOOTPRINT HAD "ANOTHER SET OF HOLES"
 The TMR2615D footprint was built around the switch centre. It carried its own copies of the MX
 holes (4.0 mm stem hole plus 1.6 mm peg holes) so it could be placed on its own.
 The hot-swap socket footprint (MX1-MX15) already drills the same holes at the same centres
 (3.99 mm / 1.75 mm). So every key position had two sets of holes stacked on top of each other,
 with slightly different sizes. That is 45 extra NPTH hits: the drill file went from 90 NPTH
 holes to 45.
 Changes:
   - Removed the NPTH pads from SW_MX_TMR2615D_1u (TMR2615D.pretty, in this folder).
   - The footprint description now says the switch holes come from the socket footprint.
   - The copy on the board was updated to match.
 After the shift, your clearances are:
   - sensor body edge to stem-hole edge: about 0.15 mm (no overlap);
   - sensor pads to hole edge: at least 0.50 mm.

3. PRODUCTION (production/)
 - gerbers/ plus Numpad_TMR1_RevD_gerbers.zip: 2-layer. Upload the zip to JLC.
 - bom.csv:
     - J2 removed (it no longer exists);
     - J1 left out of the JLC BOM (see note below);
     - U1-U15 (TMR2615D) still have no LCSC number, the same as in Rev C.
 - positions.csv (CPL):
     - J1/J2 removed.
     - Fix: U1-U15 and MX1-MX15 were at the switch centre, which is about 3-4 mm off the part.
       JLC places at the given point, so that would have been wrong.
     - They now use the real part centres: sensor = pad/body centre, socket = pad-span centre.
     - Rotations are unchanged. Check the rotation preview on the JLC website as usual.

 NOTE - J1 (1x31 2.54 mm header)
   - JLC's only stocked 1x31 is a right-angle part (C2883869). It does not fit the vertical
     footprint.
   - Either hand-solder a snapped-off 1x40 strip, or give JLC a vertical part number and add it
     to bom.csv/positions.csv.
   - It is a through-hole part, so JLC hand-solders it anyway.

4. FILES
 Numpad_TMR1.kicad_pro/.kicad_sch/.kicad_pcb, TMR2615D.pretty, TMR2615D.kicad_sym, lib tables,
 README_REVC.txt (history), production/.
