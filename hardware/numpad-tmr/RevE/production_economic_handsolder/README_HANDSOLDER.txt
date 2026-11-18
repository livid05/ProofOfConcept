NUMPAD REV E - ECONOMIC PCBA, SENSORS PLACED BY HAND
================================================================================
These files replace ../production_economic. That folder and ../production are unchanged.

WHAT JLC PLACES (64 placements, all bottom side, Economic PCBA)
 - CL2-CL15 and LC1-LC16 (0.1uF);
 - D1-D15 (SOD-323);
 - MX1-MX15 (hot-swap sockets);
 - R1-R4.

WHAT YOU SOLDER
 - U1-U15: TMR2615D-AAC-1.000-500 (DFN3L 2x2).
 - LED1-LED15: SK6812MINI-E. JLC only places these on Standard PCBA.
 - ENC1: EC11 encoder, through-hole.
 - J1 header and J2/J3 stabilizer pins (these were already hand-soldered).

The unplaced pads will probably still get solder paste from the stencil and come back tinned.
That helps with hand soldering.

SOLDER ORDER
 1. U1-U15 first.
 2. Test them (see TEST below).
 3. LED1-LED15 last. The LEDs are heat-sensitive and sit about 1.8 mm from the sensor pads,
    so keep hot air off them.
 4. ENC1 and the headers.

SENSOR ORIENTATION (important)
 - The package has two small pins on one edge and a wide GND tab on the other.
 - The two small pins go on pads 1 and 2: the edge away from the switch centre.
 - The wide tab goes on the pin-3 pad that faces the switch centre.
 - If the part is rotated 180 degrees, VDD is shorted to GND.

FOOTPRINT
 - All three pins have pads that extend 0.4 mm past the package edge, so an iron tip can reach
   them.
 - The pin-3 tab connects to the large GND pad under the body.
 - A good joint on the tab is enough electrically. The large pad is only for mechanical
   strength.

HOT AIR (easiest)
 - Flux the pads, place the part with tweezers, then heat at about 300-330 C with low airflow
   until it settles and self-aligns.
 - Cover the LED pads and the hot-swap socket nearby with Kapton tape.

IRON ONLY
 1. Use a fine conical or small knife tip and gel flux.
 2. Tack pin 1, then solder pin 2.
 3. Hold the iron on the pin-3 tab with a little solder for 3-5 seconds so it wicks under the
    body.

TEST (power on, no magnet nearby)
 - VOUT on each sensor (pad 1) should sit at about half of VDD. This part's offset is 500 mV/V.
 - 0 V or a full VDD reading means an open joint.
 - Then bring a switch magnet close. The voltage should move.

Buy a few spare sensors. DFN 2x2 parts are easy to lose.
