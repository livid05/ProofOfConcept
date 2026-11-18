TRIMODE60 - REV A
================================================================================

!!! NOT READY TO ORDER YET !!!
 26 connections on 20 nets are still unrouted (all around the STM32 / charger / gauge area).
 They are listed with pin names and coordinates in OPEN_CONNECTIONS.txt and drawn in red in
 OPEN_CONNECTIONS.png. Route them in KiCad, run DRC until it says 0 unconnected, refill zones (B),
 then re-plot the gerbers. The gerber zip is named ..._DO_NOT_ORDER_... until that is done.

WHAT IT IS
 A GH60-compatible 60% (61-key ANSI) keyboard PCB, 285.1 x 94.7 mm, 4 layers, with GH60 mounting
 holes/slots and PCB-mount stabilizer holes (2u Backspace, Enter and both Shifts; 6.25u Space).
 Three modes: USB wired, Bluetooth LE, and 2.4 GHz proprietary (ESB/Gazell, which needs a USB
 receiver dongle). Every part is on the BOTTOM side; the top side only takes the switches.

 - MCU: STM32U595RJT6 (LQFP-64, U1). USB high-speed with the internal PHY on PA11/PA12, for
   8 kHz polling. 16 MHz HSE crystal (Y1). ADC1/ADC2 read the 8 mux outputs.
 - Radio: u-blox BMD-340 (nRF52840 module, U2) under the space bar at the front edge. It talks to
   the MCU over SPI3 plus IRQ and reset lines, and has a 32.768 kHz crystal (Y2).
 - USB-C (J1, HRO TYPE-C-31-M-12): 5.1k Rd on CC1/CC2, so it is a 5 V sink with no USB PD. The
   MCU ADC reads the CC voltages to budget LED current. USBLC6-2SC6 ESD (U3), 2 A PTC fuse (F1).
 - Charger: BQ24073 power-path charger (U4). Charge current about 0.49 A (R3 1.8k), input limit
   about 1.3 A (R4 1.2k) when the MCU selects it; default USB500 (EN1 pulled up to +3V3 by R6, so
   no MCU pin ever sees 5 V). On USB its output VSYS sits at about 4.4 V. The charger's exposed
   pad is tied to its three GND pins with short traces (no vias in the pad).
 - 3.3 V: RT9080-33 LDO (U6) from VSYS, turned on by the power switch (SW62) OR by USB through
   the BAT54C (D62). So the board runs and charges on USB with the switch off.
 - Fuel gauge: MAX17048 (U5) on I2C (7-bit address 0x36), ALRT to the MCU.
 - LED rail: TPS61023 boost (U7) from VSYS, set to about 4.6 V by R14/R15 = 1M/150k
   (0.595 V x (1 + 1M/150k) = 4.56 V), with C36 150 pF feed-forward across R14. The TPS2116 power mux (U8) feeds LED_5V from VBUS on USB and from the
   boost on battery. LED_PWR_EN low = LED rail fully off. In1 is the LED_5V plane.
 - LEDs: 61 x SK6812MINI-E reverse-mount (D1-D61), serpentine chain. Data path: SPI2 MOSI
   (PB15) -> SN74LV1T125 level shifter (U9, powered from LED_5V) -> 33R -> D1.
 - Sensors: 61 x MDT TMR2615D magnetic sensors (U101-U161), one per key, on SENS_3V3. That rail
   is switched by Q1 (AO3401A), so firmware can duty-cycle the sensors (about 15 mA total) on battery.
   Q1 turns on softly (R24 10k in the gate + C37 100 nF gate-drain, about 1.5 ms ramp, about 30 mA
   inrush), so switching the 61 sensor caps on does not pull +3V3 down.
 - Muxes: sensor VOUT -> 3.3k -> KEYn node -> 8 x CD74HC4051PWR, TSSOP-16 0.65 mm (U201-U208, S0-S2 shared)
   -> MUX_OUT1-8 -> MCU ADC.
 - Hot-swap: one MX socket per key (SW1-SW61). A mechanical switch pulls KEYn to GND when
   pressed (reads about 0 V). A magnetic switch leaves the socket empty and the node follows
   the sensor (analog travel). Firmware auto-detects this per key. Switch pins face south and
   each LED is north of its switch (north-facing), so the LED window points to the back edge.
 - Controls: power slide switch SW62 (MSK12C02) at the back edge; BOOT (SW63) and RESET (SW64)
   under the Caps Lock key.
 - Battery: 1S LiPo WITH a protection PCM, on JST-PH 2.0 (J2, pin 1 = +).
 - Stackup JLC04161H-3313 (1.6 mm). In1 = LED_5V plane, In2 = GND plane. USB HS pair: 0.2 mm
   wide, 0.3 mm gap, no vias, over In2.

SENSOR FOOTPRINT (TMR2615D, K60:TMR2615D_DFN3L_2x2mm)
 - DFN3L 2 x 2 x 0.55 mm, land from datasheet Figure 7.
 - Pin 1 = VOUT and pin 2 = VDD: 0.30 mm terminals at 1.30 mm pitch. Pads are 0.35 x 0.80 mm
   and reach 0.4 mm past the body, so you can touch them up with an iron.
 - Pin 3 = GND: the large pad (1.50 x 0.95 mm) plus a neck pad that also reaches past the body.
 - The pin-1 (VOUT) silkscreen dot is next to pad 1.
 - The sensing element is 0.51 mm from the package centre toward pin 3, and senses in-plane,
   along the pin 1-2 edge (not Z). Each sensor sits so that the element is about 3.1 mm from
   the switch centre, with its sensing axis pointing at the switch centre (radial).

ORDERING AT JLC
 Run prod2.py after the zones are filled and DRC is clean:  python3 prod2.py <projdir>
 It writes production/: TriMode60_RevA_gerbers.zip, bom.csv, positions.csv, JLC_ORDER_NOTES.txt.
 1. PCB: 4 layers, 1.6 mm, impedance control with stackup JLC04161H-3313. ENIG is recommended
    for the LGA module and the 0.5 mm QFNs.
 2. Consign the sensors first. The 61 x TMR2615D-ACC-1.000-500 are not stocked, so buy them and
    send them to JLC through parts consignment (plus a few spares). JLC has to create the
    footprint: give them the datasheet (pin 1 VOUT, pin 2 VDD, pin 3 = large GND pad). The BOM
    lists them with an empty LCSC column on purpose.
 3. Global Sourcing: BMD-340 (C5456944) had 0 stock and STM32U595RJT6 (C28563722) had very low
    stock on 2026-09-25. Order both through JLC Global Sourcing early. They add lead time and
    cost, and the board is useless without them.
 4. PCBA: Economic is fine (4 layers, board size within limits). Assembly side = BOTTOM. Every
    placement in positions.csv is "bottom", with rotation = 180 - KiCad rotation + correction.
 5. Upload bom.csv and positions.csv. Then go through the CHECK list in JLC_ORDER_NOTES.txt in
    the 2D/3D preview. The riskiest parts:
      - the SK6812MINI-E LEDs (180 degrees off = VDD on GND);
      - the consigned TMR2615D;
      - the CD74HC4051PWR TSSOP-16 muxes (pin 1 dot), and the BQ24073 and MAX17048 QFNs;
      - the SOT-563/SOT-583 TI parts;
      - the BMD-340;
      - the hot-swap sockets.
    rotations.md has the table and sources. Fix a wrong one by rotating it in the preview.
    JLC engineers also check polarity against the B.SilkS pin-1 marks.
 6. Choose "Remove order number" (or place it yourself), or JLC prints it on the board.

BRING-UP AND TEST POINTS
 The test pads are 1.5 mm round pads on the bottom, at 2.6 mm pitch, for pogo pins or soldered
 wires. Positions below are as seen from the TOP; looking at the bottom, left and right swap.
 The silkscreen shows the labels below; TP4 and TP12 are both printed "NRST".
   MCU (U1), 2 x 4 block under LAlt / the left end of the space bar, about 4 mm from the front
   edge:
     TP1 3V3     TP2 SWDIO   TP3 SWCLK   TP4 NRST
     TP5 GND     TP6 SWO     TP7 TX      TP8 RX       (TX/RX = MCU debug UART)
   Radio (U2), 2 x 3 block between the "," key and the right end of the space bar:
     TP9 3V3     TP10 NSWDIO  TP11 NSWCLK
     TP12 NRST (nRF reset, NRF_RST_N)   TP13 GND
 Handy measuring points (capacitors, bottom side):
   VBUS C1 (4.7 uF, keeps total VBUS capacitance under USB's 10 uF), VSYS C3, VBAT C4, LED_5V C13/C14, boost out C10/C11, SENS_3V3 C15, +3V3 TP1/C8.
 1. Before power: check the preview-check parts on the real board. Then check VBUS, VSYS, VBAT,
    +3V3 and LED_5V to GND for shorts.
 2. USB only, no battery, power switch OFF, ideally through a USB current meter. Expect
    VBUS 5 V, VSYS about 4.4 V and 3.3 V on TP1. LED_5V should be 0 V (LED_PWR_EN is pulled low).
 3. STM32: connect an ST-LINK (V3 preferred) or another SWD probe with STM32U5 support to
    TP1-TP5 (TP1 is the voltage reference only). Flash a USB test and check that it enumerates.
    Holding BOOT and tapping RESET should start the ST ROM bootloader. If it doesn't show up
    on USB, use SWD.
 4. nRF52840: connect a J-Link, an nRF DK's debug-out or a CMSIS-DAP probe to TP9-TP13. The
    MCU also drives NRF_RST_N, so flash the radio while the STM32 is blank or held in reset
    (TP4 to GND).
 5. LEDs: set LED_PWR_EN high. LED_5V should read about 5 V on USB and about 4.6 V on battery.
    Start at low brightness: 61 LEDs at full white can draw about 2 A, far over USB500.
 6. Sensors: drive SENS_EN_N low (SENS_3V3 = 3.3 V). With no magnet, every sensor reads about
    1.65 V (half of VDD). Scan all 61 keys through the muxes.
      - about 0 V: a pressed mechanical switch, or a sensor short / wrong orientation;
      - 3.3 V or floating: an open joint.
 7. Battery: check the lead's polarity against J2 (pin 1 = +) BEFORE plugging it in, because the
    board has no reverse-battery protection. On USB it charges at up to about 0.49 A (less in
    USB500 mode). CHG_N goes low while charging. Read the gauge at I2C 0x36.
 8. Battery only: SW62 ON powers the board; OFF leaves only the charger and gauge quiescent draw.

CHECKS
 - PCB DRC (KiCad 9, JLC04161H-3313 rules): 0 errors. Warnings: 2 silk_over_copper and
   2 silk_edge_clearance (silkscreen only, JLC clips it), plus "library footprint not found"
   notices when the lib tables are not loaded (harmless). 26 UNCONNECTED items remain; see
   OPEN_CONNECTIONS.txt.
 - Via-in-pad: 0. Every via is off its pad and joined to it by a short trace.
 - Schematic ERC: 0 errors, 0 warnings.
 - Schematic/PCB parity: 0 differences (every footprint, value and net matches the schematic).

ROUTING
 - 4 layers: F.Cu and B.Cu signals, In1.Cu = LED_5V plane, In2.Cu = GND plane (solid).
 - Signal tracks 0.15 mm (JLC 4-layer minimum is 0.09 mm), power 0.2/0.3/0.5 mm; vias
   0.5 mm pad / 0.3 mm hole (JLC standard, no extra cost). About 1,090 vias.
 - Fine-pitch parts (STM32 LQFP-64, charger QFN, gauge DFN, muxes) fan out with a short trace to
   a via just outside the pad. No via is placed in any pad.
 - The 8 analog muxes are now CD74HC4051PWR in TSSOP-16 (0.65 mm pitch, LCSC C352826) instead of
   the 0.5 mm DHVQFN, so their pins can be escaped without via-in-pad. Same pinout and function.
 - USB D+/D- run on B.Cu at 0.2 mm right above the In2 GND plane, with one short F.Cu hop on D+.
   They are not tuned to 90 ohm differential. That is fine for USB full speed; for 480 Mbps high
   speed, check the pair in JLC's impedance calculator and re-route it as a tuned pair if needed.
 - Open connections: 26 (see the warning at the top). Tips for finishing them in KiCad:
     Route > Interactive Router Settings > mode "Shove"; use 0.15 mm width; press V to drop a
     via when a trace needs to change layer; stay out of pads with the via.

PRODUCTION (production/)
 - TriMode60_RevA_gerbers_DO_NOT_ORDER_26_unrouted.zip (re-plot after routing): 4 copper layers
   (F, In1, In2, B), both masks, pastes and silkscreens, Edge.Cuts, PTH and NPTH Excellon drill files (mm, oval holes as G85 slots).
 - bom.csv: JLC format, 44 lines, 460 parts. The TMR2615D line has an empty LCSC column
   (consigned). No mounting holes, stabilizers or test pads.
 - positions.csv: JLC CPL format, 460 bottom-side placements, corrections from rotations.md.
 - JLC_ORDER_NOTES.txt: consignment, Global Sourcing and the check-the-preview list.

KNOWN LIMITATIONS
 - Rotation corrections for the QFN/DFN parts, SOT-563/583, the custom footprints (LED, socket,
   sensor, BMD-340) and the MSK12C02 are not in any public JLC database. Check them in the
   preview; that step is not optional.
 - The TMR2615D is consigned and its land comes from the datasheet drawing, not a vendor land
   pattern. Check the first boards (X-ray or a continuity check on VOUT/VDD) before trusting all 61.
 - The sensors are placed from the datasheet geometry. How far each switch's magnet sits from
   the sensor varies between switch brands, so expect to calibrate per key in firmware and to
   test which magnetic switches work.
 - The hot-swap socket in the BOM is the HanElectricity CPG151101S11-16 clone (C41430893),
   because the genuine Kailh CPG151101S11 (C2803348) had 0 stock. It uses the same footprint.
 - The LED rail is about 4.6 V on battery and about 5 V on USB. That is within the SK6812MINI-E's
   3.7-5.5 V, but colour and brightness can shift slightly between the two.
 - LED power budget: 61 LEDs at full white is about 2.3 A at 4.6 V. On battery the boost (2.7 A min
   switch limit) and L1 (FNR4020S1R0MT, 3.2 A rated) can't supply that from a 3.5 V cell, so cap
   brightness at about 50% on battery in firmware. On USB keep it inside the budget read from CC.
 - Firmware must-dos:
     - STM32U5: call HAL_PWREx_DisableUCPDDeadBattery() early. PA15 (NRF_CS) and PB15 (LED_DATA)
       double as the UCPD CC pins, which have pull-downs at reset.
     - nRF52840: P0.18 only acts as a reset input after the UICR PSELRESET registers are programmed
       (0x7FFFFFD2); do that when you first flash the radio.
     - Mux channels with no key (U207 ch 4 input, U208 ch 2/4 inputs) float: never select them.
 - USB-C is a 5 V sink only (no PD). The firmware must keep LED current inside the USB budget
   it reads from CC1/CC2.
 - There is no reverse-polarity protection on the battery input. JST-PH polarity is not
   standard between battery sellers.
 - Radio range depends on the case. Keep metal (case, plate, weights) away from the BMD-340
   antenna end at the front edge under the space bar. The 2.4 GHz mode needs a separate
   receiver dongle and firmware.
