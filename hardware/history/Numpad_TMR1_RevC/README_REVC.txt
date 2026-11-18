NUMPAD_TMR1 - REV C
================================================================================

BASED ON
 Your latest saved Rev B, including your hand edits. All 37 track and via items you changed
 in Rev B (the U2/U3/U4/U6/U8 +3.3V vias you moved out of the sensor pads, and the TMR_OUT
 and +3.3V ends you redrew to the new pad centres) are still in Rev C, unchanged.
 Footprints, pads, nets, zones, the board outline and silkscreen are also unchanged. Only
 tracks and vias were edited. README_REVB.txt stays in this folder for the Rev B history.

WHAT CHANGED
 1. No via-in-pad
    Rev B still had 36 vias whose copper touched a pad:
      - 15 LED data vias (LED1-LED15 DIN/DOUT);
      - 14 LC cap vias (+3.3V, LC1-LC12, LC14, LC15);
      - 5 COL vias in the MX hot-swap pads (MX5, MX9, MX10, MX13, MX14);
      - 2 CL cap vias (+5V, CL8, CL12).
    All 36 are now outside the pads. Each via:
      - moved 0.2-1.8 mm;
      - keeps at least 0.1 mm of solder mask between its ring and every pad, 0.2 mm copper
        clearance to other nets, 0.25 mm hole clearance, and 0.5 mm from the board edge
        and the LED cut-outs.
    Each pad reaches its via through a short bottom-layer track:
      - 0.25 mm on signal nets and the 0.2 mm cap feeds;
      - 0.4 mm where the old +5V/+3.3V tracks were 0.5 mm (CL8, CL12, LC6, LC14, LC15).
    Where possible the via now sits on the existing top-layer track or bus, so no new
    top-layer copper was needed. The old top-layer stubs that ran to the in-pad position were
    removed. Net connectivity is identical to Rev B (checked pad by pad for every net).
    Supporting changes this needed:
      - LED9 and LED10 DOUT: the via had nowhere to go (LED cut-out on one side, the +5V bus
        on another, the +5V feed to CL9/CL10 on the third). So I moved that +5V feed 0.28 mm
        east. It is still 0.5 mm wide and now has a 45-degree jog into the CL pad. Its bus
        via slid along the bus. The DOUT via then sits just east of the pad, as LED5 in row 2
        already did.
      - Two GND stitching vias were added, at (157.60, 72.60) and (149.73, 67.88). The first
        is in the ground pour between the row-1 LED data line and the +3.3V bus. The second is
        in the bottom pour around LED2 pad 4. Before, both pour areas reached the rest of the
        ground only through the narrow gaps that the LED2/LED3 vias now occupy. Without these
        stitching vias they would have become isolated copper.
 2. Unnecessary wires cleaned up
    Every net was checked as a connectivity graph:
      - pads, vias and track ends are the nodes;
      - T-junctions and overlapping pads are included;
      - the GND pours count as connections.
    Every removal was accepted only if every pad of that net stayed connected, KiCad still
    showed 0 unconnected, and no track end was left dangling.
    Removed from Rev B:
      - duplicate or overlapping segments: 45
          13 lying on top of another segment, e.g. the +5V top bus at (156.7, 68.7), the
             +3.3V bus under your new segments at U2/U3, and COL_1 in the MX5 pad;
          32 buried inside a wider track of the same net, e.g. the 0.2 mm +3.3V copies under
             the 0.5 mm bus at U7 (169.5, 92.7) and short +5V stubs inside the CL feeds;
      - zero-length segments: 1 (+3.3V at LC7);
      - dangling stubs: 0;
      - unconnected or one-sided vias: 0;
      - redundant loop paths: 8. The thinner of two parallel paths was removed; the 0.5 mm
        bus was never cut. Examples:
          the 4.2 mm 0.2 mm LC6-to-U10 feed at (147.1-150.7, 111.7), beside the bus;
          the LC7/U6 feeds at (147-150.6, 93.2);
          U14 (149.9, 130.8);
          U7 (169.5, 92.8);
          a doubled ROW_2 diagonal at (145.0-145.9, 97.7-98.6);
      - tracks through same-net pads that formed a loop: none separate from the loops above;
      - collinear fragments merged: 6 (e.g. TMR_OUT1 at (130.9, 73.4), TMR_OUT13 at
        (130.8, 130.6), COL_1 below MX5).
    Removed after the via moves:
      - 27 dangling stubs (the old feeds to the in-pad via positions);
      - 2 overlapping segments;
      - 2 buried segments;
      - 1 loop.
    Totals: 754 -> 738 track segments, 197 -> 199 vias, and 57 mm less top-layer track
    length (the new via stubs are on the bottom layer).

LEFT ALONE (on purpose)
 - At U2 (147.7-149.4, 73.6-73.7) and U3 (169.2-169.9, 73.6-73.7) there are four short
   0.2 mm segments of yours, two at each spot. They lie entirely inside the 0.5 mm +3.3V bus,
   so they add no copper and do no harm, but they are redundant. I kept them because they
   are your edits.
   I added a 0.14 mm tie from each loose end to the bus centre line, so KiCad does not flag
   them as dangling. Delete them in KiCad if you like; nothing changes electrically.
 - A few short track pieces lying fully inside pads (e.g. in the MX hot-swap pads) were left.
   They are covered by pad copper and change nothing.

CHECKS (Rev C, zones refilled)
 - DRC:
     Rev B: 0 errors, 0 unconnected.
     Rev C: 0 errors, 0 unconnected.
   The only warnings in both are the same 64 "library not found" ones for Mars_Library and
   marbastlib, which aren't installed on my side. No new warning types.
 - Schematic parity is unchanged: the same 15 PCB-only footprints (CL2-CL15, LC16).
 - Via-in-pad check: 36 in Rev B, 0 in Rev C.

PRODUCTION (production/)
 - Numpad_TMR1_RevC_gerbers.zip: the same 2-layer layer set as Rev B. It holds the copper,
   paste, silkscreen, mask and edge layers, the PTH/NPTH drill files with their maps, and the
   job file. The paste, silkscreen, mask, edge and NPTH files are identical to Rev B, because
   the vias are tented. Only the copper and PTH drill changed (2 more via holes).
 - positions.csv: regenerated. Identical to Rev B, since no part moved.
 - bom.csv: copied from Rev B (with LCSC numbers). The parts are unchanged.

PICTURES
 - vip_cleanup_before_after.png: the whole board, Rev B vs Rev C.
     magenta = the old via-in-pad positions;
     orange = wires removed or merged;
     green = the new via positions.
 - vip_cleanup_details.png: six close-ups (an LC cap, an LED, an MX pad, two bus clean-ups,
   and the LED9 +5V feed).
