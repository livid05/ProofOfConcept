U575 DEVBOARD - REV F
================================================================================

BASED ON
 Your latest save of RevE, including your SWDIO via tweak near (60.6, 101.6).
 That tweak is kept exactly as you left it.

CHANGES
 1. J6 (MUX JST-SH, 8-pin, top edge) removed
    Why it could go: the on-board mux (U5) and the J8 TMR breakout already cover what it did.
    - Schematic: the J6 symbol, its 8 wire stubs, 5 global labels and 3 power symbols were
      removed. Nothing else in the schematic changed.
    - PCB: the J6 footprint was removed, along with every track and via that only fed it.
      That is 48 track segments and 9 vias. For each net, the J6 branch was pruned back to
      the junction where it split off. Nothing that the MCU, U5 or the headers need was
      touched.
    - Copper saved per net: PA6 93 -> 47 mm, PA7 99 -> 62 mm, PB0 88 -> 56 mm,
      PB1 91 -> 58 mm, PB2 111 -> 61 mm.
    - Silkscreen:
        - the S3/S2/S1/S0/GND/SIG/GND/3V3 pin labels and the "MUX2" label are gone;
        - the back-side legend line "J6 MUX2 SIG=PA6" is gone, and the lines below it moved
          up to close the gap;
        - the board label now reads "U575 DEVBOARD REV F".
    - PA6 is still a free MCU pin on the side header. PA7/PB0/PB1/PB2 still drive U5 S0-S3.

 2. C25 (100 nF, U5 decoupling) moved next to U5 pin 24 (VCC)
    What C25 is for:
      - It is not a short. It is the mux's local bypass capacitor.
      - When the mux switches channels it draws fast current spikes. The cap supplies them
        from right at the pin, instead of through the long path back to the regulator.
      - To high-frequency noise it looks like a short, but to DC it is open.
    Before: C25 sat 8 mm away at (74.05, 40.5), and only reached the pin through the planes.
    Now:
      - C25 is at (64.80, 38.90), rotated 180. Its +3V3 pad is about 1 mm above pin 24.
      - The +3V3 pad connects straight down onto pin 24 and to pin 24's existing +3V3 plane
        via.
      - The GND pad has its own new GND via at (64.45, 37.75), 0.25 mm track.
      - The old C25 stubs and their 2 plane vias were removed.

CHECKS
 - ERC: 0 violations.
 - DRC with schematic parity:
     - 0 errors, 0 unconnected, 0 parity issues;
     - the only warning is the old Y2 footprint-library note, same as Rev E.
 - Via-in-pad: 0 (169 vias).
 - Copper totals: 988 -> 941 track segments; 178 -> 169 vias (10 removed, 1 added).

PRODUCTION (production/)
 - Capstone_Keyboard_Design_RevF_gerbers.zip, gerbers/, bom.csv, positions.csv.
 - Differences from Rev E:
     - bom.csv: the J6 line (C160407) is removed;
     - positions.csv: J6 removed, C25 has its new position and rotation.
 - All other LCSC parts are unchanged.
