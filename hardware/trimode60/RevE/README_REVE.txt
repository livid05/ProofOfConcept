TRIMODE60 - REV E  (Rev D with cheaper, fee-free parts; new folder, Rev D untouched)
================================================================================

WHAT CHANGED
 - Parts only. Copper, footprints, nets and placement are identical to Rev D (checked
   segment by segment).
 - Only the board label now reads Rev E.
 - Three parts were swapped to drop-in equivalents that JLC lists as Basic or Preferred.
   JLC charges a loading fee (about $3.07 in Economic PCBA) for every unique Extended part
   in an order; Basic and Preferred parts have no fee.
     - D62 BAT54C: C2135 -> C22466350.
         - Same BAT54C: common cathode, 30 V, 200 mA, SOT-23.
         - Preferred. $0.022 -> $0.014.
     - D63 SMF12A TVS: C207255 -> C19077506.
         - Same SMF12A: 12 V standoff, 19.9 V clamp, 200 W, SOD-123FL.
         - Preferred. $0.151 -> $0.028.
     - FB1 600R@100MHz bead: C107335 -> C1002 (GZ1608D601TF, 0603).
         - Basic.
         - It is rated 200 mA / 0.45 ohm. It only feeds VDDA (a few mA, so about 2 mV of
           drop), so there is no effect on the ADC or the USB PHY.
 - Saving: about $9.20 in loading fees per assembly order, plus about $0.13 per board.
 - Already fee-free, so no change needed: R3 150R (C25082) and R15 150k (C25755) are
   Preferred.

LOOKED AT BUT KEPT (a cheaper version would reduce performance, or none exists)
 - C36 150pF: the only Basic/Preferred 150 pF parts are X7R, and a feed-forward cap should
   stay C0G.
 - C43 47nF bootstrap cap: no Basic/Preferred 47 nF. It stays at the datasheet value.
 - R4/R5 NTC divider 5.23k/30.1k: Basic 5.1k/27k would let the battery charge down to
   -1.3 C instead of 0 C. You chose to keep exact behaviour.
 - R26 4.3M, R33 6.04k, R34 10.5k: no Basic/Preferred values. R33/R34 set the PD
   request and must stay exact.
 - Parts with no Basic or Preferred equivalent at JLC in the same footprint:
     - ICs: STM32U595, BMD-340, BQ25895, MAX17048, HUSB238, TPS61023, RT9080, SN74LV1T125,
       74HC4051;
     - protection: USBLC6-2SC6 ESD, 3 A PTC;
     - inductors: FNR4020S (L1), XAL4020 (L2);
     - connectors and switches: USB-C, JST-PH, MSK12C02, B3U;
     - SK6812MINI-E LEDs, hot-swap sockets, 32.768 kHz crystal.
 - RT9080: every EN-pin LDO alternative draws 35-50 uA instead of 2 uA, which would cost
   battery life.

ORDERING (updated)
 - U1 STM32U595RJT6 (C28563722): JLC had only 10 in stock on 2026-09-27.
     - Buy them into your JLC parts library before placing the PCBA order.
     - JLC Economic PCBA assembles a minimum of 2 boards, so reserve 2 for the keyboard.
       Dev board Rev G now uses the same chip, so reserve 4 in total (5 with a spare).
 - U2 BMD-340 (C5456944): 464 in stock (it was 0 on 09-25). Reserving 2 is cheap insurance.
 - Everything else is as in Rev D:
     - impedance control ON, stackup JLC04161H-3313;
     - Economic PCBA, bottom side;
     - consign the TMR2615D sensors.

CHECKS
 - DRC:
     - 0 errors, 0 unconnected;
     - warnings are the same silkscreen ones as before.
 - ERC 0. Schematic/PCB parity 0.
 - bom.csv differs from Rev D only in the three LCSC numbers above. positions.csv is
   identical.
