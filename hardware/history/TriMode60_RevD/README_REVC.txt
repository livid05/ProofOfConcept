TRIMODE60 - REV C  (new folder; Rev A and Rev B are untouched)
================================================================================

STATUS
 - Fully routed.
 - KiCad DRC: 0 errors, 0 unconnected. The only warnings are silkscreen ones on the USB-C / L2
   outlines.
 - ERC: 0. Schematic/PCB parity: 0.
 - Via-in-pad: 0 (1,224 vias, none in or touching a pad).
 - Production files are in production/ (see ORDERING AT JLC).

WHAT CHANGED FROM REV B
 1. USB-C PD fast charging (about 18-20 W)
    - U10 HUSB238_002DD PD sink (LCSC C7471904).
        - It asks the charger for 9 V / 2 A: R33 6.04k sets VSET = 9 V, R34 10.5k sets
          ISET = 2 A.
        - If the charger has no 9 V profile, it takes the next lower one (normally 5 V).
        - It presents the 5.1k Rd itself, so R1/R2 are gone. Its I2C is not used.
    - F1 is now a 3 A PTC (1812), D63 an SMF12A TVS on VBUS, and C1 is 1 uF.
    - The USBLC6 (U3) now takes its reference from +3V3, because VBUS can be 9 V.
 2. New charger: U4 BQ25895RTWR, a 5 A switch-mode charger with power path (I2C 0x6A, LCSC C80200)
    - It replaces the 0.49 A linear BQ24073.
    - L2 1 uH 8.7 A (XAL4020 class), PMID 2 x 10 uF 25 V, SYS/BAT 10 uF each, REGN 4.7 uF,
      BTST 47 nF.
    - R3 150R sets the hardware input-current ceiling to about 2.37 A (ILIM pin).
    - Default charge current is 2.0 A (ICHG register default 2048 mA).
    - 4000 mAh from empty:
        - about 1.5 h to 80 %;
        - about 2.3 h to full.
 3. Battery (4000 mAh, 1S LiPo, 60 x 90 x 6 mm, "606090"). It is now in KiCad:
    - BT1: schematic symbol only (not on the board and not in the BOM).
    - BAT_OUTLINE: a board-only outline plus a 3D box model. It sits under the right half with
      its lead end toward J2 and floats 4.5 mm below the PCB in the 3D view. It is there only
      to check the fit and has no copper.
    - J2: 3-pin JST-PH (pin 1 VBAT, pin 2 NTC, pin 3 GND). The connector is rated 2 A.
    - J3: three solder pads (VBAT / NTC / GND) for a battery soldered directly. Use them if you
      raise the charge current above 2 A. They are hand-soldered and not in the BOM.
    - NTC: R4 5.23k / R5 30.1k are TI's values for a 10k 103AT thermistor.
        - If your battery has no NTC wire, solder-bridge JP1: R32 (10k) then looks like a
          25 C thermistor.
        - With no NTC and JP1 open, the charger sees "too cold" and will not charge.
 4. LEDs always run from the boost (U7 TPS61023); the TPS2116 LED power mux (U8) is gone.
    The LED rail voltage is set in firmware:
    - PB9 PWM -> R27 10k / C38 1 uF filter -> R26 4.3M into the boost feedback node.
    - PWM 0 % = 4.70 V, 100 % = 3.93 V, pin floating/reset = 4.56 V.
    - Lower it to save battery (the SK6812MINI-E runs from 3.7 V).
 5. Optional MCU lines dropped in this revision to free routing room. Everything they did is still
    available over I2C:
    - Charger STAT: pin 4 not connected, R8 removed. Read REG0B CHRG_STAT instead.
    - Charger CE: held low by R6 100k, no MCU pin. Use REG03 CHG_CONFIG to stop and start charging.
    - CC1/CC2 ADC taps: R30/R31 removed. PD is handled by U10. For a non-PD 5 V charger, let the
      charger's input current optimizer (ICO, on by default) find the limit.
    - VBUS_DET (PA9) is also gone. USB presence = charger INT plus REG0B PG_STAT/VBUS_STAT.
    - Freed pins (set them to analog): PC4, PC5, PC6, PB8, PA9. The Rev B list of unconnected
      pins still applies.
 6. Layout
    - The charger, PD sink and battery connectors sit under the right half, next to the battery.
    - VBUS runs from the USB corner:
        - route: F1 -> C1 -> between rows 1 and 2 -> down to the PD sink and charger;
        - length about 130 mm;
        - width: 0.8-1.0 mm on its main path; the narrowest point is a 0.6 mm neck for 1 mm
          near the charger;
        - vias: the layer changes use 2-3 vias in parallel where there was room.
    - B.Cu power pours thicken VBAT, VSYS and VBUS around the charger.
    - The LED_5V In1 plane has a new tongue under the boost output, because the old
      boost -> mux path is gone.
    - Parts moved or turned to untangle the charger area:
        - C44 takes R30's old spot;
        - R4 and R26 are turned 180 degrees;
        - the boost switch node BST_SW was re-routed at 0.6 mm (Rev B had 0.3 mm).

FIRMWARE MUST-DOS (new for Rev C)
 - BQ25895 (I2C1 on PB6/PB7, address 0x6A; INT on PB5, active low, R7 pull-up)
    1. Watchdog: the chip has a 40 s I2C watchdog that puts it back in default mode when it runs
       out. Either write REG07 WATCHDOG = 00 at boot, or set REG03 WD_RST more often than
       every 40 s.
    2. Input current limit: D+/D- are not wired, so after plug-in the charger picks a safe
       default input limit (about 500 mA).
         - After the PD contract settles (VBUS about 9 V in the ADC), write REG00 IINLIM = 2.0 A
           to match U10's 9 V / 2 A request.
         - Leave ICO enabled (REG02 ICO_EN).
         - The ILIM pin caps everything at about 2.37 A anyway.
    3. Charge current: REG04 ICHG. Default 2048 mA; that is the recommended value (see the
       thermal note).
    4. Start the ADC (REG02 CONV_START / CONV_RATE). It then reports VBUS, VBAT, VSYS, charge
       current and TS, so the MCU can see which PD voltage it got.
    5. Enable/disable charging: REG03 CHG_CONFIG. Status: REG0B (CHRG_STAT, VBUS_STAT, PG_STAT)
       and REG0C (faults).
 - Fuel gauge MAX17048: I2C 0x36, as in Rev B.
 - LED rail: TIM4_CH4 (or TIM17_CH1) PWM on PB9, at 20 kHz or faster. Duty 0 % = brightest
   headroom (4.70 V), 100 % = 3.93 V.
 - The Rev B must-dos still apply:
     - UCPD dead-battery disable;
     - nRF PSELRESET;
     - never select the empty mux channels.

THERMAL AND CURRENT NOTES (read before raising the charge current)
 - No via-in-pad, as you asked. So the BQ25895's exposed pad has no thermal vias; it is tied to
   GND only through pins 8/17/18 and the copper around it.
 - At 2 A charging it runs warm but within limits.
 - Above about 2.5 A it will reach its 120 C thermal regulation and fold the current back by
   itself (REG0E THERM_STAT shows it). That is safe, just slower.
 - If you ever allow vias in that one exposed pad, 3 A+ becomes realistic.
 - J2 (JST-PH) is rated 2 A. For more than 2 A, solder the battery to J3.
 - LED budget (unchanged): 61 LEDs at full white is about 2.3 A at 4.6 V. Cap brightness at
   about 50 % on battery.

ORDERING AT JLC
 - production/TriMode60_RevC_gerbers.zip:
     - 4 layers: F, In1, In2, B, plus masks, paste, silkscreen, outline;
     - PTH/NPTH drill files.
     - Stackup JLC04161H-3313, 1.6 mm, ENIG recommended.
 - bom.csv: 51 lines, 467 parts. All new Rev C parts have LCSC numbers and were in stock when
   chosen.
     - The TMR2615D sensors are still consigned (empty LCSC column).
     - BMD-340 and STM32U595 still need Global Sourcing.
 - positions.csv: 467 bottom-side placements.
 - JLC_ORDER_NOTES.txt: the preview check list. New entries are the BQ25895 WQFN-24 and the
   HUSB238 WDFN-10; check their pin-1 corners in the preview.
 - J3 (solder pads), JP1 (solder jumper), BT1 and BAT_OUTLINE are not in the BOM or CPL.

BRING-UP DELTAS (the rest of the Rev B procedure still applies)
 1. Measuring points:
      - VBUS at C1 (78.5, 74.1), near the USB corner;
      - PMID at C39;
      - VSYS at C3;
      - VBAT at C4;
      - LED_5V at C10/C13;
      - QON at TP14.
 2. Power it first with no battery, from a 5 V-only USB port. You should see VBUS 5 V and VSYS
    about 3.6-3.7 V (the charger's minimum system voltage), and +3V3 up.
 3. Then use a PD charger: VBUS should go to about 9 V within about 1 s. If it stays at 5 V, the
    charger has no 9 V profile or U10 is not placed right.
 4. Battery: check the lead polarity against J2 first (pin 1 = +). There is no reverse-battery
    protection. Check the NTC wire, or bridge JP1.

KNOWN LIMITATIONS
 - The HUSB238's I2C is not wired. Firmware cannot read the PD contract directly; use the
   charger's VBUS ADC reading.
 - Charging above 2 A is limited by heat (no exposed-pad vias) and by J2's 2 A rating (use J3).
 - The Rev B notes on the USB HS pair (not impedance-tuned) and on radio placement still apply.
