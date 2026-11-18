U595 DEVBOARD - REV G
================================================================================

WHY
 - You need USB high speed (480 Mbps). The STM32U575 only has full-speed USB (12 Mbps).
 - The STM32U595 has a built-in high-speed USB PHY on the same pins, so this board can
   now run the keyboard's USB-HS firmware.

WHAT CHANGED
 1. U1: STM32U575RGT6 (C5270980) -> STM32U595RJT6 (C28563722)
    - Same LQFP-64 footprint.
    - In KiCad's library, all 64 pin names and positions are identical between the two
      symbols, so no wire, net or track moved.
    - 4 MB flash / 2.5 MB RAM (was 1 MB / 786 KB).
    - USB HS on PA12 (D+) / PA11 (D-), the same pins as before.
    - The HS PHY needs a crystal reference clock. Y2 (16 MHz, on PH0/PH1) is one of the
      supported values, so no new parts are needed.
    - The schematic symbol, value, LCSC, datasheet link and description are updated. The
      board label now reads "U595 DEVBOARD REV G".
 2. USB lines through the ESD part (U3)
    - Before: the connector side (DP_C/DM_C) and the MCU side (DP/DM) were separate nets,
      and the signal relied on the connection inside the USBLC6.
    - Now: each line is one net, with a straight trace under the package from pad 6 to
      pad 1 (D+) and from pad 4 to pad 3 (D-). This is ST's flow-through layout: no stub,
      and the lowest inductance.
    - In the schematic the DP_C/DM_C labels are renamed DP/DM.
 3. 90 ohm pair
    - Same stackup as the keyboard: 3313 prepreg under F.Cu, over the solid In1 GND plane.
    - Lines are now 0.18 mm on the existing 0.5 mm pitch (0.32 mm gap) = 90 ohm, from the
      same 2D field solve used for the keyboard.
 4. Length matching (connector contact to MCU pin, each plug orientation)
    - D+ minus D- = +0.40 mm one way up, -0.02 mm flipped (under 3 ps).
    - To get there, the D+ A6-B6 join was pulled 0.25 mm closer to the pads, and stub
      copper overlapping the connector pads was trimmed.

FIRMWARE NOTES
 - Retarget the project to STM32U595RJTx. It is a superset of the U575, so existing code
   ports directly.
 - For USB HS, use USB_OTG_HS with the embedded PHY:
     - select the 16 MHz PHY reference clock (SYSCFG OTG HS PHY clock selection);
     - enable VDDUSB and the HS transceiver supply in PWR before starting the core.
   CubeMX sets all of this if you pick USB_OTG_HS "Device only" with the internal HS PHY
   and HSE = 16 MHz.

CHECKS
 - ERC 0.
 - DRC with schematic parity: 0 errors, 0 unconnected, 0 parity issues. The only warning is
   the old Y2 footprint-library note.
 - Via-in-pad 0 (169 vias).
 - BOM: only the U1 line changes. CPL is identical to Rev F.

ORDERING
 - STM32U595RJT6 (C28563722) had only 10 at JLC on 2026-09-27. Buy it into your JLC parts
   library first:
     - 2 for this board (JLC's assembly minimum);
     - 2 for the keyboard;
     - one spare if you want it.
 - The chip costs about $24 (the U575 was about $5.70), so each assembled dev board is
   about $18.60 more.
 - Production files are in production/ (gerbers zip, bom.csv, positions.csv), as before.
