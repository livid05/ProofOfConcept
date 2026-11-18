U5A5 DEVBOARD - REV K
================================================================================
Rev K is Rev J with three BOM value changes. Copper, drills, mask, paste and placement are
identical to Rev J; only the silkscreen label changed.

WHAT CHANGED
 1. U1: STM32U595RJT6 (C28563722) -> STM32U5A5RJT6 (C22454506), the part you chose.
     - ST lists the two as 100% pin-to-pin compatible.
     - The U5A5 is the same chip plus an encryption engine. It keeps the 480 Mbps USB PHY.
     - It costs $23.08 at JLC instead of $29.83.
     - JLC had only 3 in stock on Oct 4 2026, and this order needs 2.
 2. R13-R28 (mux input resistors): 1k (C11702) -> 100 ohm (C25076, 0402 1%, Basic).
 3. C26-C41 (mux input capacitors): 10 nF (C15195) -> 4.7 nF (C1538, 0402 X7R 50 V, Basic).
 4. Board label: "U595 DEVBOARD REV J" -> "U5A5 DEVBOARD REV K".
     - The schematic note on the RC values was updated to match.

WHY THE RC FILTER CHANGED
 - The CD74HC4067 common pin has 50 pF (TI datasheet).
 - Each time the mux changes channel, that charge is shared with the newly selected filter
   capacitor. That leaves an error of about 50/(C+50) of the voltage difference between the
   two channels, which then decays through R.
     - Rev J, 1k / 10 nF: about 0.5% error, tau 10 us, about 50 us per channel to settle to
       14 bits. Full 16-key scan about 1.2 kHz.
     - Rev K, 100 R / 4.7 nF: about 1% error, tau 0.47 us, about 2.5 us per channel. Full
       16-key scan above 15 kHz, so 8 kHz has plenty of margin.
 - TMR2615 datasheet:
     - maximum load capacitance on VOUT is 10 nF. Rev J's 10 nF sat at that limit;
       4.7 nF plus the cable leaves margin;
     - minimum load resistance is 10 k (DC). The RC draws no DC current, so this is met;
     - sensor bandwidth is 30 kHz, so the faster RC does not let in extra noise.
 - Filter corner is now about 340 kHz. Use ADC oversampling if you want more smoothing.

CHECKED AND UNCHANGED (full review, Oct 4 2026)
 - Y2 crystal: X322516MLB4SI is 16 MHz, 9 pF load, 50 ohm ESR.
     - C5/C6 at 12 pF are correct (12/2 + ~3 pF stray = 9 pF).
     - 16 MHz is a supported reference for the U5 USB HS PHY.
 - STM32 power pins:
     - VDD pins 19, 32 and 64 each have 100 nF within 2-4 mm;
     - VDDUSB has 100 nF;
     - VDDA has 100 nF + 1 uF;
     - VCAP (pin 30) has 4.7 uF X5R;
     - VBAT is tied to 3V3 through R11 (0 ohm) with 100 nF.
 - Reset and boot:
     - NRST: 10k pull-up, 100 nF, RST button, SWD pin 10, J10;
     - BOOT0 (PH3): 10k pull-down, BOOT button to 3V3.
 - USB:
     - CC1 and CC2 have 5.1k pull-downs;
     - USBLC6 is wired flow-through;
     - polyfuse is 500 mA hold / 1 A trip;
     - SBU pins are open;
     - D+/D- are routed for 90 ohm on the default stackup (Rev J).
 - AMS1117-3.3: pinout correct, 10 uF in, 22 uF out.
 - Mux (CD74HC4067):
     - COM -> PC4 (ADC1_IN13, 14-bit);
     - S0-S3 = PA7/PB0/PB1/PB2;
     - E tied to GND;
     - VCC 3V3 with 100 nF.
 - Connectors:
     - J7 SWD is the standard ARM 10-pin Cortex debug pinout (SWO = PB3);
     - J5 UART: pin 1 GND, pin 2 3V3, pin 3 PA2 (TX), pin 4 PA3 (RX).

CHECKS RUN
 - ERC: 0.
 - DRC with schematic parity:
     - 0 errors, 0 unconnected, 0 parity issues;
     - the only warning is the old Y2 footprint-library note.
 - BOM differs from Rev J only in the 3 lines above.
 - positions.csv is identical to Rev J. It already includes the JLC placement corrections:
     - U1 135, U2 270, U3 180, U5 270;
     - J1 moved +1.37 mm.
 - Gerbers: every copper, mask, paste, drill and edge layer is identical to Rev J.

FIRMWARE / WIRING NOTES (no hardware change needed)
 - Target: STM32U5A5RJTx in CubeMX.
 - Clock and USB:
     - HSE = 16 MHz crystal (not 25 MHz);
     - USB_OTG_HS "Device only" with the embedded PHY;
     - PHY reference clock = 16 MHz from HSE;
     - enable the VDDUSB and USB HS transceiver supplies in PWR before starting USB;
     - HSI48 + CRS is only for full-speed USB.
 - UART on J5 is USART2 (PA2 TX / PA3 RX), not USART1.
 - User button SW3 is on PC13 to GND. Enable the internal pull-up.
 - ADC sequencing per mux step:
     - switch S0-S3;
     - wait at least 3 us;
     - convert PC4.
   16 steps fit well inside the 125 us of an 8 kHz frame.
 - SK6812 LED data (numpad LED_IN). The LEDs need 0.7 x VDD = 3.5 V on a 5 V supply; the
   STM32 drives 3.3 V. Pick one:
     a. 74AHCT1G125 / 74AHCT125 buffer powered from 5 V, between the MCU pin and LED_IN
        (cleanest);
     b. a 5 V-tolerant (FT) pin in open-drain mode with a 1k pull-up to 5 V (VBUS).
        Check in the datasheet pin table that the pin is FT.
 - LED current: 15 LEDs at full white is about 540 mA, which is over USB's 500 mA and the
   polyfuse rating. Cap brightness in firmware to about 60% white. Each LED also draws about
   1 mA when off.
 - VBAT: R11 ties VBAT to 3V3. Remove R11 before fitting a coin cell on J2-12.

ORDERING AT JLC
 - Delete the Rev J dev board from the cart and add Rev K. The numpad items stay as they are.
 - 4 layers, 1.6 mm, FR4 TG135, Specify Stackup = No.
 - Economic PCBA, top side, 2 boards.
 - Upload production/Capstone_Keyboard_Design_RevK_gerbers.zip, production/bom.csv and
   production/positions.csv.
 - In the placement preview, check that pin-1 dots sit on the pin-1 markers and the USB-C
   pins are on their pads.
