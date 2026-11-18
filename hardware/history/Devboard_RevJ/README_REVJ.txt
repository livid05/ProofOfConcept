U595 DEVBOARD - REV J
================================================================================

WHY
 - JLC's 90-ohm stackup (JLC04161H-3313) cannot be used with Economic PCBA.
 - On JLC's default 4-layer stackup (JLC04161H-7628, 0.21 mm prepreg), Rev I's 0.18 mm USB
   traces come out at about 110-125 ohm differential.
 - Rev J re-routes only the USB D+/D- pair so it is about 90 ohm on the default stackup.
 - This makes the board suitable for USB high speed (480 Mbps).

WHAT CHANGED (from Rev I; no parts moved, no vias added)
 1. Paired section (STM32 -> USBLC6):
     - 0.25 mm traces with a 0.18 mm gap (0.43 mm centres);
     - about 90 ohm differential;
     - 92-93 ohm where no ground fill runs beside the pair.
 2. Split sections (through and around the USBLC6, where D+ and D- are 1.9 mm apart):
     - 0.35 mm each;
     - about 46-49 ohm per line, which is 91-98 ohm as a pair.
 3. Pin escapes:
     - 0.20 mm for the last ~0.5-1 mm into the USB-C pins and the short bridges between the
       two pin rows;
     - the 0.5 mm-pitch connector leaves no room for wider traces there.
 4. USB-C dual-row breakout swapped:
     - D+ now bridges over the top (B6 -> A6);
     - D- now loops underneath (B7 -> A7).
 5. Small length bump on D+ beside the USBLC6.
     - D+/D- length mismatch is now about 0.8 mm (~5 ps) in BOTH plug orientations.
     - In Rev I it was 0.4 mm one way and 2.0 mm the other.
 6. USB_HS net class: track 0.25, diff pair 0.25 / 0.18 gap, clearance 0.18.
     - The 0.18 mm clearance applies only between D+ and D-.
     - Everything else still keeps 0.2 mm from the pair.
 7. Back label reads "U595 DEVBOARD REV J".
 - RevJ_vs_RevI_USB.png shows the routing before and after.

HOW THE NUMBERS WERE CHECKED
 - A 2D field solve of the actual cross-section:
     - F.Cu over the solid In1.Cu GND plane, 0.2104 mm 7628 prepreg (er 4.4);
     - 1 oz copper, solder mask on top;
     - with and without the F.Cu ground fill beside the traces.
 - The same solver gives about 87 ohm for Rev I's geometry on the 3313 stackup it was
   designed for, so the method agrees with the original design.

CHECKS
 - ERC: 0.
 - DRC with schematic parity:
     - 0 errors, 0 unconnected, 0 parity issues;
     - the only warning is the old Y2 footprint-library note.
 - Closest copper from another net to the USB pair: 0.215 mm. The D+ to D- gap is 0.18 mm.
 - No via-in-pad. Board size is unchanged at 25.85 x 101.6 mm.

PRODUCTION (production/)
 - Capstone_Keyboard_Design_RevJ_gerbers.zip and gerbers/.
 - bom.csv: identical to Rev I.
 - positions.csv: already includes the JLC placement corrections found in JLC's preview:
     - U1 135, U2 270, U3 180, U5 270;
     - J1 moved 1.37 mm into the board.
   It is identical to Rev I production_jlc_fixed/positions.csv.

ORDERING AT JLC
 - 4 layers, 1.6 mm, FR4 TG135.
 - Specify Stackup: No (default). Impedance Control: no requirement.
 - Economic PCBA, top side. Upload production/bom.csv and production/positions.csv.
 - In the placement preview, check that:
     - each pin-1 dot is on its pin-1 marker;
     - the USB-C pins sit on their pads.
