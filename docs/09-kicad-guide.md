# KiCad guide

Everything needed to open, understand, check and order the three boards. All projects were made with KiCad 9.0 (file version 20241229). Open the `.kicad_pro` file of a board to load its schematic, layout, rules and library tables together.

There are no exported schematic PDFs or fresh board renders in this repository: the KiCad command-line exporter was not available when it was assembled. The images in `hardware/images/` are layout comparisons and renders saved during each revision. To produce schematic PDFs, open a project and use File, Plot (schematic) and File, Plot (board); do this before a release and add the output to `hardware/images/`.

## The three projects

| Board | Folder | Project file | Layers | Size | KiCad content |
|---|---|---|---|---|---|
| Dev board Rev K | `hardware/devboard-u5a5/RevK/` | `Capstone Keyboard Design.kicad_pro` | 4 | 25.85 x 101.6 mm | 73 footprints, 953 track segments, 169 vias |
| Numpad TMR1 Rev E | `hardware/numpad-tmr/RevE/` | `Numpad_TMR1.kicad_pro` | 2 | 81.7 x 86.8 mm | 98 footprints, 781 track segments, 211 vias |
| TriMode60 Rev E | `hardware/trimode60/RevE/` | `TriMode60.kicad_pro` | 4 | 285.1 x 94.7 mm | 492 footprints, 4057 track segments, 1225 vias |

Board sizes come from each revision's notes. Counts come from the PCB files.

### File types in each folder

| File | What it is |
|---|---|
| `.kicad_pro` | Project settings, net classes and design rules |
| `.kicad_sch` | Schematic. TriMode60 is hierarchical: `TriMode60.kicad_sch` is the top sheet and `mcu`, `power`, `keys` and `radio` are its sub-sheets |
| `.kicad_pcb` | Board layout |
| `.kicad_dru` | Custom design rule (dev board: keeps In1 a solid ground plane for the USB pair) |
| `sym-lib-table`, `fp-lib-table`, `TMR2615D.kicad_sym`, `TMR2615D.pretty/`, `lib/` | Project-local libraries, so the custom sensor symbol and footprints load without extra setup |
| `production/` | Gerber zip, `bom.csv` and `positions.csv` for JLCPCB, plus order notes where present |

The `.kicad_prl` files (per-user view state), backup folders and the unpacked `gerbers/` folders were left out; the gerber zips contain the same data.

## Net classes and rules

| Board | Class | Track | Clearance | Via (pad / drill) | Diff pair (width / gap) |
|---|---|---|---|---|---|
| Dev board | Default | 0.2 mm | 0.2 mm | 0.6 / 0.3 mm | 0.2 / 0.25 mm |
| Dev board | USB_HS | 0.25 mm | 0.18 mm | 0.6 / 0.3 mm | 0.25 / 0.18 mm |
| Numpad | Default | 0.2 mm | 0.2 mm | 0.6 / 0.3 mm | 0.2 / 0.25 mm |
| TriMode60 | Default | 0.2 mm | 0.15 mm | 0.5 / 0.3 mm | 0.2 / 0.25 mm |
| TriMode60 | USB_HS | 0.18 mm | 0.15 mm | 0.5 / 0.3 mm | 0.18 / 0.32 mm |

Minimum copper-to-edge clearance is 0.5 mm on the dev board and 0.2 mm on the numpad and TriMode60. The 0.18 mm clearance on the dev board USB class applies only between D+ and D-.

## Stackups and USB impedance

- **Dev board:** 4 layers, 1.6 mm. F.Cu carries parts, signals and the USB pair; In1 is a solid GND plane that is the reference for the pair; In2 is the +3V3 plane; B.Cu carries signals and the breadboard headers. Rev K routes the pair for about 90 ohm differential on JLC's default 4-layer stackup (JLC04161H-7628, 0.21 mm prepreg): 0.25 mm lines with a 0.18 mm gap in the paired section.
- **TriMode60:** 4 layers on the JLC04161H-3313 stackup with impedance control on. The USB pair is on the bottom layer over a ground plane at 0.18 mm width and 0.32 mm gap, which a 2D field solve gave as 90.4 ohm. Order notes say to select that stackup and impedance control in the order form.
- **Numpad:** 2 layers, 1.6 mm, FR4 TG135. Cheap and simple; ordered on its own because mixed-design panels cost extra.

The impedance figures come from the revision notes (a 2D field solve of the real cross-section). They were not measured on built boards.

## Design checks recorded in the revision notes

| Board | Last recorded result |
|---|---|
| Dev board Rev K | ERC 0; DRC with schematic parity 0 errors, 0 unconnected, 0 parity issues; only warning is an old Y2 footprint-library note; 0 vias in pads (169 vias) |
| Numpad Rev E | DRC 0 errors, 0 unconnected; warnings only for footprint-library notes and one silkscreen-at-edge note. The schematic lacks CL2 to CL15 and LC16, which exist only on the PCB |
| TriMode60 Rev C | ERC 0, parity 0, DRC 0 errors and 0 unconnected, 0 vias in pads (1,224 vias). Rev D and E changed only the USB pair copper and parts, with copper otherwise identical |

Re-run ERC and DRC in KiCad after any change; these are records of past runs, not guarantees about the files as they are today.

## Pin and connector reference (dev board)

These tables are from the Rev H overview document in `reports/`. Revs I to K did not change the connector pinouts, but Rev K changed the mux RC values (see below).

### J8: sensor header (2 x 10, 2.54 mm)

Pin 1 (square pad) is 3.3 V. The numbers are TMR_OUT channel numbers. In ribbon-cable order: pin 1 3V3, pin 2 GND, pins 3 to 10 TMR_OUT1 to 8, pin 11 GND, pins 12 to 19 TMR_OUT9 to 16, pin 20 GND. TMR_OUTn arrives on mux channel n minus 1.

| Row | Pos 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Even pins | GND | 2 | 4 | 6 | 8 | 9 | 11 | 13 | 15 | GND |
| Odd pins (pin 1 end) | 3V3 | 1 | 3 | 5 | 7 | GND | 10 | 12 | 14 | 16 |

### Mux and ADC

| Signal | Pin |
|---|---|
| Mux common (COM) to ADC | PC4 (ADC1_IN13) |
| Select S0, S1, S2, S3 | PA7, PB0, PB1, PB2 |
| Enable | tied low (always on) |
| Input filter, Rev K | 100 ohm series + 4.7 nF to GND per input (R13 to R28, C26 to C41); Rev H and earlier were 1 k and 10 nF |

### J7: SWD (10-pin, 1.27 mm Cortex debug)

| Pin | Signal | MCU |
|---|---|---|
| 1 | VTref (3.3 V sense) | +3V3 |
| 2 | SWDIO | PA13 |
| 3, 5, 9 | GND | GND |
| 4 | SWCLK | PA14 |
| 6 | SWO | PB3 |
| 10 | nRESET | NRST |

Keep PA13, PA14 and PB3 out of firmware GPIO use, or debugging stops working once the code runs.

### J5: UART (4-pin JST-SH, USART2)

| Pin | Signal | MCU |
|---|---|---|
| 1 | GND | GND |
| 2 | 3.3 V output | +3V3 |
| 3 | TX (board to adapter) | PA2 |
| 4 | RX (adapter to board) | PA3 |

### Buttons and LEDs

SW1 is reset (NRST), SW2 is boot (BOOT0 on PH3, high while pressed; hold SW2, tap SW1, release SW2 to enter the ROM bootloader), SW3 is a user button on PC13 to GND (enable the internal pull-up). D1 is a red LED on PA5 (also TIM2_CH1), D2 is a green power LED.

### Side headers J2 and J3 (1 x 26, 2.54 mm, bottom side)

| Pin | J3 (left) | J2 (right) |
|---|---|---|
| 1 | +3V3 | +3V3 |
| 2 | PA3 (UART RX) | PA2 (UART TX) |
| 3 | PA4 | PA1 |
| 4 | PA5 (LED D1) | PA0 |
| 5 | PA6 | PC3 |
| 6 | PA7 (mux S0) | PC2 |
| 7 | PC4 (mux COM / ADC) | PC1 |
| 8 | PC5 | PC0 |
| 9 | PB0 (mux S1) | PC15 |
| 10 | PB1 (mux S2) | PC14 |
| 11 | PB2 (mux S3) | PC13 (USER button) |
| 12 | PB10 | VBAT |
| 13 | PB12 | PB9 |
| 14 | PB13 | PB8 |
| 15 | PB14 | PB7 |
| 16 | PB15 | PB6 |
| 17 | PC6 | PB5 |
| 18 | PC7 | PB4 |
| 19 | PC8 | PB3 (SWO) |
| 20 | PC9 | PD2 |
| 21 | PA8 | PC12 |
| 22 | PA9 | PC11 |
| 23 | PA10 | PC10 |
| 24 | SWDIO (PA13) | PA15 |
| 25 | VBUS (5 V) | SWCLK (PA14) |
| 26 | GND | GND |

Orientation: looking at the component side with USB-C at the bottom, J3 is on the left and J2 on the right; pin 1 is at the end nearest J8. J9 (GND) and J10 (NRST) are stabilizer pins that sit 27.94 mm above pin 1; the board spans 37 breadboard rows, so use a full-size breadboard. Not brought out: PA11 and PA12 (USB), PH0 and PH1 (crystal) and PH3 (BOOT0).

### Power

USB-C 5 V passes a 500 mA polyfuse (F1) to an AMS1117-3.3 (U2), which makes the board's single 3.3 V rail on the In2 plane. CC1 and CC2 have 5.1 k pull-downs, so the board is a plain 5 V sink with no USB PD. VBAT is tied to 3V3 through R11 (0 ohm); remove R11 and feed a coin cell into J2 pin 12 to keep the RTC running without USB. The board has no 32 kHz crystal; the RTC runs from the internal LSI.

### Numpad connector J1 and the wiring used on the bench

J1 is a 1 x 31 header. Sensor outputs TMR_OUT1, 5, 9, 13, 2, 6, 10, 14, 15, 11, 7, 3, 12, 8, 4 are on J1 pads 6 to 20 and were wired to dev board J8 holes 16, 14, 12, 10, 7, 5, 3, 1, 15, 13, 11, 9, 8, 6, 4. J8 hole 2 is intentionally unconnected. See [03-tmr-sensing-guide.md](03-tmr-sensing-guide.md).

## Bills of materials (from the production BOMs)

### Dev board Rev K (25 lines)

| Designators | Part | Footprint | LCSC |
|---|---|---|---|
| C1 | 10uF 25V | 0603 | C96446 |
| C2 | 22uF 6.3V | 0603 | C59461 |
| C3 | 4.7uF 16V | 0603 | C19666 |
| C4, C7, C8, C9, C10, C22, C23, C25 | 100nF | 0402 | C1525 |
| C5, C6 | 12pF | 0402 | C1547 |
| C24 | 1uF | 0402 | C52923 |
| C26, C27, C28, C29, C30, C31, C32, C33, C34, C35, C36, C37, ... | 4.7nF | 0402 | C1538 |
| D1 | LED_RED | 0603 | C2286 |
| D2 | PWR green | 0603 | C12624 |
| F1 | Polyfuse 500mA | 1206 | C151162 |
| J1 | USB_C_Receptacle_USB2.0_16P | TYPE-C-31-M-12 | C165948 |
| J5 | UART JST-SH | SM04B-SRSS-TB | C160404 |
| J7 | SWD 10-pin 1.27 | 2x5 1.27mm SMD header | C41376037 |
| R1, R2 | 5.1k | 0603 | C23186 |
| R5, R12 | 1k | 0603 | C21190 |
| R6, R7 | 10k | 0603 | C25804 |
| R8, R11 | 0 | 0402 | C17168 |
| R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, R24, ... | 100 | 0402 | C25076 |
| SW1, SW2 | SW_Push | B3U-1000P | C231329 |
| SW3 | USER | B3U-1000P | C231329 |
| U1 | STM32U5A5RJT6 | LQFP-64 | C22454506 |
| U2 | AMS1117-3.3 | SOT-223 | C6186 |
| U3 | USBLC6-2SC6 | SOT-23-6 | C7519 |
| U5 | CD74HC4067SM96 | SSOP-24 | C98457 |
| Y2 | 16MHz | SMD3225-4P | C13738 |

Hand-solder: J2, J3, J8, J9, J10. Rev K changed U1 to the STM32U5A5RJT6 (C22454506), R13 to R28 to 100 ohm and C26 to C41 to 4.7 nF; the table above is the file as saved and is worth comparing with the Rev K notes.

### Numpad Rev E (8 lines)

| Designators | Part | Footprint | LCSC |
|---|---|---|---|
| CL2, CL3, CL4, CL5, CL6, CL7, CL8, CL9, CL10, CL11, CL12, ... | 0.1uF | C_0402 | C1525 |
| D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, ... | D_Small | D_SOD-323 | C2128 |
| ENC1 | RotaryEncoder_Switch_MP | RotaryEncoder_MX_EC11_CircularMountingHoles | C255515 |
| LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8, LED9, LED10, ... | SK6812MINI-E | LED_6028R | C5149201 |
| MX1, MX2, MX3, MX4, MX5, MX6, MX7, MX8, MX9, MX10, MX11, ... | MX_SW_HS | SW_MX_HS_CPG151101S11_1u | C41430893 |
| R1 | 330 | R_0603_1608Metric_Pad0.98x0.95mm_HandSolder | C23138 |
| R2, R3, R4 | 10k | R_0603 | C25804 |
| U1, U2, U3, U4, U5, U6, U7, U8, U9, U10, U11, U12, U13, U14, ... | TMR2615D-ACC-1.000-500 | SW_MX_TMR2615D_1u | consigned / none |

Hand-solder: J1 (1 x 31 header) and J2, J3 (stabilizer pins). The sensors U1 to U15 have no LCSC number and are consigned or fitted by hand. The `production_economic` variant also removes the LEDs and the encoder from the BOM to be hand-soldered (JLC lists the SK6812MINI-E as Standard PCBA only, which costs a $25 setup fee per side and adds edge rails), and uses JLC consignment entry C9900189162 for the sensors. The schematic value "TMR2615D-ACC-1.000-500" is noted there as a typo for AAC.

### TriMode60 Rev E (51 lines)

| Designators | Part | Footprint | LCSC |
|---|---|---|---|
| C1,C2,C5,C6,C7,C25,C38,C44 | 1uF | C_0402_1005Metric | C52923 |
| C3,C4,C8,C9,C15,C32,C41 | 10uF | C_0603_1608Metric | C19702 |
| C10,C11,C13,C14 | 22uF | C_0805_2012Metric | C45783 |
| C20,C21,C22,C23,C26,C28,C31,C33,C37,C101,C102,C103,C104, ... | 100nF | C_0402_1005Metric | C1525 |
| C24,C27,C42 | 4.7uF | C_0603_1608Metric | C19666 |
| C29,C30 | 12pF | C_0402_1005Metric | C1547 |
| C34,C35 | 20pF | C_0402_1005Metric | C1554 |
| C36 | 150pF | C_0603_1608Metric | C37298 |
| C39,C40 | 10uF 25V | C_0805_2012Metric | C15850 |
| C43 | 47nF | C_0402_1005Metric | C285062 |
| D1,D2,D3,D4,D5,D6,D7,D8,D9,D10,D11,D12,D13,D14,D15,D16,D17, ... | SK6812MINI-E | LED_SK6812MINI-E_Reverse | C5149201 |
| D62 | BAT54C | SOT-23 | C22466350 |
| D63 | SMF12A | D_SMF | C19077506 |
| F1 | PTC 3A hold | Fuse_1812_4532Metric | C883162 |
| FB1 | 600R@100MHz | L_0603_1608Metric | C1002 |
| J1 | USB-C 16P | USB_C_Receptacle_HRO_TYPE-C-31-M-12 | C165948 |
| J2 | Battery JST-PH 3P | JST_PH_S3B-PH-SM4-TB_1x03-1MP_P2.00mm_Horizontal | C265101 |
| L1 | 1uH 3.2A | L_Changjiang_FNR4020S | C167822 |
| L2 | 1uH 8.7A | L_Coilcraft_XAL4020-XXX | C4555951 |
| Q1 | AO3401A | SOT-23 | C15127 |
| R3 | 150R 1% | R_0402_1005Metric | C25082 |
| R4 | 5.23k 1% | R_0402_1005Metric | C2933105 |
| R5 | 30.1k 1% | R_0402_1005Metric | C29402 |
| R6,R18,R19,R28 | 100k | R_0402_1005Metric | C25741 |
| R7,R22,R24,R27,R32 | 10k | R_0402_1005Metric | C25744 |
| R10,R11 | 4.7k | R_0402_1005Metric | C25900 |
| R13,R14,R25 | 1M 1% | R_0402_1005Metric | C26083 |
| R15 | 150k 1% | R_0402_1005Metric | C25755 |
| R23 | 33R | R_0402_1005Metric | C25105 |
| R26 | 4.3M 1% | R_0402_1005Metric | C2998052 |
| R29 | 47k | R_0402_1005Metric | C25792 |
| R33 | 6.04k 1% | R_0402_1005Metric | C25913 |
| R34 | 10.5k 1% | R_0402_1005Metric | C25850 |
| R101,R102,R103,R104,R105,R106,R107,R108,R109,R110,R111,R112, ... | 3.3k | R_0402_1005Metric | C25890 |
| SW1,SW2,SW3,SW4,SW5,SW6,SW7,SW8,SW9,SW10,SW11,SW12,SW13, ... | CPG151101S11 | SW_MX_Hotswap_Kailh_Magnetic_1u | C41430893 |
| SW62 | Power MSK12C02 | SW_SPDT_Shouhan_MSK12C02 | C431540 |
| SW63 | BOOT | SW_SPST_B3U-1000P | C231329 |
| SW64 | RESET | SW_SPST_B3U-1000P | C231329 |
| U1 | STM32U595RJT6 | LQFP-64_10x10mm_P0.5mm | C28563722 |
| U2 | BMD-340-A-R-10 | u-blox_BMD-340 | C5456944 |
| U3 | USBLC6-2SC6 | SOT-23-6 | C7519 |
| U4 | BQ25895RTWR | Texas_RTW_WQFN-24-1EP_4x4mm_P0.5mm_EP2.7x2.7mm | C80200 |
| U5 | MAX17048G+T10 | TDFN-8-1EP_2x2mm_P0.5mm_EP0.8x1.2mm | C2682616 |
| U6 | RT9080-33GJ5 | TSOT-23-5 | C841192 |
| U7 | TPS61023DRLR | SOT-563 | C919459 |
| U9 | SN74LV1T125DBVR | SOT-23-5 | C473338 |
| U10 | HUSB238_002DD | WDFN-10-1EP_3x3mm_P0.5mm_EP1.8x2.5mm | C7471904 |
| U101,U102,U103,U104,U105,U106,U107,U108,U109,U110,U111,U112, ... | TMR2615D-ACC-1.000-500 | TMR2615D_DFN3L_2x2mm | consigned / none |
| U201,U202,U203,U204,U205,U206,U207,U208 | CD74HC4051PWR | TSSOP-16_4.4x5mm_P0.65mm | C352826 |
| Y1 | 16MHz | Crystal_SMD_3225-4Pin_3.2x2.5mm | C13738 |
| Y2 | 32.768kHz | Crystal_SMD_2012-2Pin_2.0x1.2mm | C97607 |

61 TMR2615D sensors (U101 to U161, MDT TMR2615D, DFN3L 2x2) are consigned. Two parts are low stock at JLC: U1 (STM32U595RJT6, 10 in stock on 2026-09-27) and U2 (u-blox BMD-340, nRF52840 module). Order details are in `hardware/trimode60/RevE/production/JLC_ORDER_NOTES.txt`.

## Ordering at JLCPCB (summary of the notes)

| Board | PCB | Assembly |
|---|---|---|
| Dev board Rev K | 4 layers, 1.6 mm, FR4 TG135, specify stackup = No; impedance control not needed on the default stackup | Economic PCBA, top side, 2 boards (JLC minimum); upload `production/` gerbers zip, `bom.csv`, `positions.csv`; check pin-1 and USB-C orientation in the placement preview |
| Numpad Rev E | 2 layers, 1.6 mm, FR4 TG135, single PCB, 5 pieces; quoted about $4 | Economic PCBA, bottom side; check LED, SOD-323 diode and hot-swap socket rotations in the preview |
| TriMode60 Rev E | 4 layers, JLC04161H-3313, impedance control on, 285.1 x 94.7 mm | Economic PCBA, bottom side (all 467 placements); consign the 61 sensors first |

Prices and stock numbers in the notes are from the dates stated there (late September to early October 2026) and will have changed.

## Things that went wrong, from the KiCad side

- Footprint size mismatch (numpad Rev A): see [08-lessons-learned.md](08-lessons-learned.md).
- Vias in pads and edge clearance: Rev C and Rev E of the numpad.
- Duplicate drill holes: the sensor footprint carried its own copy of the switch holes, doubling 45 NPTH holes (fixed in numpad Rev D).
- Schematic and PCB mismatch: the numpad schematic lacks CL2 to CL15 and LC16, which exist only on the PCB. Add them to the schematic before relying on ERC or parity checks for that board.
- TriMode60 Rev A shipped with 26 unrouted connections; it was marked not for ordering until routed.
