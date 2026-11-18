U575 DEVBOARD - REV E  (Rev D with unnecessary copper removed)
==============================================================

Only tracks and vias changed, plus the board label now reads "U575 DEVBOARD REV E". Footprints,
nets, zones, board outline and schematic are identical to Rev D.

REMOVED / CLEANED (7 fewer track segments, 2 fewer vias)
 1. Numpad, MUX_I0: the trace hopped F.Cu -> via -> B.Cu -> via -> F.Cu along a
    straight line that is free on F.Cu. Removed both vias (50.99,38.53) and (57.40,44.94)
    and the 9.1 mm B.Cu piece. Now one F.Cu diagonal to U5.9, over the In1 GND plane
    like the other 15 mux inputs.
 2. Numpad, TMR_OUT1: 3 collinear pieces from R13.1 to (53.34,31.40), with a 2 um
    mis-joint between two of them. Merged into 1 segment.
 3. GND at C9.2: a 0.35 mm, 0.2 mm-wide segment lying exactly on top of the
    0.25 mm C9.2-to-via track. Removed.
 4. +3V3 at U1 pin 48: 2 short pieces inside the pad and inside the pad-to-via
    track (a redundant loop). Removed.
 5. PC9 near U1: 2 collinear pieces merged into 1 segment at (56.07,101.86).

LEFT AS IS (reviewed, not unnecessary copper)
 - 6 sub-0.1 mm jogs between parallel 45-degree runs (PA4 x2, PA7, PA9, PB15, PH3).
   They add no copper. Straightening them would bend the runs off 45 degrees.
 - PA0 takes a long route (about 102 mm, 10 vias, for a 34 mm connection). No shortcut
   is free. Shortening it needs a reroute, not a cleanup.
 - PA15 and SWCLK each use one via where their B.Cu run from J2 could also be on F.Cu.
   This is a layer choice, not extra copper.

CHECKS
 - Zones refilled before the final DRC.
 - DRC with schematic parity: 0 errors, 0 unconnected, 0 parity issues. The only
   warning is the old Y2 footprint-library note, same as Rev D.
 - Via-in-pad check: 0 (178 vias).
 - Connectivity graph re-check: no duplicate, zero-length, dangling, loop or in-pad
   redundant copper left.
 - Diff against Rev D: only the tracks and vias listed above changed.

PRODUCTION (production/)
 - Capstone_Keyboard_Design_RevE_gerbers.zip, gerbers/, bom.csv, positions.csv
 - bom.csv and positions.csv are byte-identical to Rev D, so the LCSC parts are unchanged.
 - Only the copper layers and the PTH drill file changed (2 fewer drill hits).
   Mask, paste and edge layers are unchanged; silkscreen differs only in the REV E label.
 - RevD_vs_RevE_cleanup.png shows each change, before and after.
