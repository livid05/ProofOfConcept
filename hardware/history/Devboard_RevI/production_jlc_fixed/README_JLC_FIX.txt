DEV BOARD REV I - PLACEMENT FILE CORRECTED FOR JLC  (../production is unchanged)
================================================================================
Found in JLC's Part Placement preview on Oct 3 2026:
 - Several JLC part models are drawn in a different orientation from the KiCad footprints.
 - With the original positions.csv, JLC would have placed these parts wrong.

Changes in positions.csv. Nothing else changed, and the board and BOM are the same.
  U1  STM32U595 (LQFP-64)   -135 -> 135   pin 1 was at the wrong corner (90 deg off)
  U2  AMS1117 (SOT-223)       90 -> 270   tab was over the 3 small pads (180 deg off)
  U3  USBLC6 (SOT-23-6)      -90 -> 180   leads ran across the pads (90 deg off)
  U5  CD74HC4067 (SSOP-24)     0 -> 270   leads ran across the pads (90 deg off)
  J1  USB-C                   Y moved 1.37 mm into the board. The model sat toward the edge,
                              with its pins off the pads.

Checked and correct as-is:
 - D1 and D2 (LEDs, polarity matches);
 - Y2 crystal, J5, J7, SW1-SW3, F1;
 - all resistors and capacitors.

After uploading, check JLC's placement preview again before paying:
 - every pin-1 dot should sit on the pin-1 marker;
 - the USB-C pins should be on their pads.

Stackup: leave Specify Stackup at "No".
 - JLC04161H-3313 is a special stackup that does not support Economic PCBA, so the option
   is greyed out once Economic assembly is selected.
 - On the default stackup the USB pair comes out around 120 ohm instead of 90.
 - The traces are only 12.7 mm long. That is harmless for USB full speed (12 Mbps), which
   a keyboard uses, and minor even at high speed.
