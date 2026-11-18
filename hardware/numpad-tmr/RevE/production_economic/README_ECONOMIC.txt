NUMPAD REV E - ECONOMIC PCBA FILES  (the files in ../production are unchanged)
================================================================================

WHY THIS FOLDER
 - JLC lists the SK6812MINI-E LEDs (C5149201) as Standard PCBA only.
     - Standard PCBA has a $25 setup fee per side, versus $8 for Economic.
     - It also adds 5 mm edge rails to the board.
 - ENC1 (EC11 encoder) is through-hole. Economic PCBA only places SMD parts.

WHAT CHANGED
 - LED1-LED15 and ENC1 are removed from bom.csv and positions.csv. Hand-solder them after
   the boards arrive.
     - The SK6812MINI-E has long legs made for hand soldering.
     - ENC1 is 5 pins plus 2 mounting lugs.
 - U1-U15 now carry JLC number C9900189162. That is JLC's consignment entry for
   TMR2615D-AAC, rated for both Economic and Standard PCBA.
     - The schematic value "TMR2615D-ACC-1.000-500" is a typo for AAC; the BOM comment is
       corrected here.
 - positions.csv: 79 placements, all bottom side.

STILL HAND-SOLDERED (not in the BOM)
 - J1 (1x31 header), J2/J3 (stabilizer pins), ENC1, LED1-LED15.

OPTION: LEDs PLACED BY JLC ON ECONOMIC
 - SK6812MINI-E-012 (C22466970) is in the same LED6028 package and is rated for Economic.
 - It was 0 in stock (pre-order only) on Oct 3 2026.
 - Check its datasheet pinout against the SK6812MINI-E before using it:
     the board footprint (LED_6028R) is wired pad 1 = +5V, 2 = DOUT, 3 = DIN, 4 = GND.
 - If it matches, add the LED line back with C22466970 and restore the LED rows from
   ../production/positions.csv.
