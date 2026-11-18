TRIMODE60 - REV D  (Rev C with the USB high-speed pair tuned; new folder, Rev C untouched)
================================================================================

WHY
 - Wired 8 kHz polling needs USB 2.0 high speed (480 Mbps).
 - That needs D+/D- routed as a 90 ohm differential pair, with the two lines the same length.
 - Rev C (and Rev B) did not meet either target.

WHAT CHANGED (only the USB_DP / USB_DM copper, the board label, and a net class)
 1. Impedance: 85 -> 90 ohm
    - The pair runs on the bottom layer over the solid In2 GND plane, 0.0994 mm below it
      (JLC04161H-3313 stackup, 3313 prepreg, Dk 4.1).
    - Rev C geometry: 0.20 mm lines, 0.30 mm gap = 85 ohm.
    - Rev D geometry: 0.18 mm lines, 0.32 mm gap = 90.4 ohm.
    - The centre-to-centre pitch is still 0.5 mm, so the path did not move.
    - Method: a 2D field solve of the real cross-section, using JLC's calculator inputs
      (1.6 mil outer copper with etch taper, solder mask 1.2 mil on laminate and 0.6 mil on
      copper, Dk 3.8).
    - Results:
        - odd-mode impedance 45.2 ohm, so differential = 90.4 ohm;
        - with the GND pour 0.25 mm beside the lines: 89.9 ohm;
        - common mode 24.6 ohm.
    - For reference, 0.20/0.30 solves to 85.1 ohm and 0.15/0.35 to 99 ohm.
 2. D+ now passes straight through the ESD part (U3)
    - Rev C problem: D+ left pad 6, went up to the top layer through a via, went round the +3V3
      via, and came back down through a second via to pad 1. That added about 4.4 mm of extra
      trace and 2 via stubs on only one line of the pair.
    - Rev D: D+ runs straight from pad 6 to pad 1 on the bottom layer (the D- side already did
      this). It sits 0.2 mm off the pad centre line to clear the +3V3 via.
    - Result: the whole pair is now on the bottom layer, with 0 vias from the connector to the MCU.
 3. Length matching
    - Measured from the USB-C contact to the MCU pin, for each plug orientation.
    - Rev C: D+ was longer by +7.1 mm (plug one way up) and +4.5 mm (flipped) = 42 / 27 ps
      of skew.
    - Rev D: D+ minus D- = +1.05 mm one way up, -1.04 mm flipped = +/- 6 ps.
        - The D+ tighter A6-B6 join and a shorter D+ exit from the ESD part are set so the two
          orientations cancel.
        - The remaining +/-1 mm cannot be removed on one layer. It comes from the USB-C
          footprint: each line is duplicated for the flipped plug, and the duplicate pads sit
          1 mm apart, so one orientation always has a short extra loop on D+ and the other on D-.
    - Common USB 2.0 high-speed layout guidance allows 1.25 mm (50 mil) or more, so this passes.
 4. Net class "USB_HS"
    - Added to the project for nets USB_D*: 0.18 mm track width, 0.18/0.32 mm differential-pair
      settings.
    - If you edit the pair in KiCad, it keeps the right geometry.

UNCHANGED AND CHECKED
 - Reference plane:
     - In2 carries no tracks, and there are no vias inside the USB corridor.
     - The closest other-net via antipads stay clear of the traces. The one via near an
       uncoupled stretch is the ESD part's own +3V3 via.
 - Stubs:
     - The only stubs are the unavoidable USB-C duplicate-pad joins (about 1.2-1.4 mm).
     - The ESD part is flow-through on both lines, so it adds no stub.
 - Coupling: 89 % of the pair is coupled at 0.32 mm. The rest is at the connector pads, the ESD
   pads (1.9 mm pad pitch) and the MCU pins.

CHECKS
 - DRC:
     - 0 errors, 0 unconnected;
     - warnings are the same silkscreen ones as Rev C.
 - ERC 0. Schematic/PCB parity 0. Via-in-pad 0 (1,222 vias).
 - BOM and CPL are identical to Rev C.

ORDERING
 - Upload production/TriMode60_RevD_gerbers.zip, bom.csv, positions.csv.
 - NEW: choose impedance control in the order form with stackup JLC04161H-3313. It is the same
   stackup as before, but ask for controlled impedance so JLC holds the 90 ohm (+/-10 %).
 - If you want to confirm the numbers yourself: JLC's online impedance calculator, JLC04161H-3313,
   L4 (bottom), reference L3, differential, 0.18 mm width / 0.32 mm spacing.

OTHER BOARDS (your question: are the diff pairs matched?)
 - TriMode60: USB is the only differential pair (the radio link is SPI). It is now tuned and
   matched, as above.
 - Dev board Rev F (STM32U575):
     - Its USB is full speed only (12 Mbps), because the U575 has no high-speed PHY.
     - The pair is about 12 mm long, and D+/D- differ by about 1-2 mm depending on plug
       orientation.
     - At 12 Mbps (4-20 ns edges) that does not matter, so no change was made.
 - Numpad: no differential pairs (it talks over the 1x31 header).
