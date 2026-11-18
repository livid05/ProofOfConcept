# Hardware used for this test

Confirmed part: **STM32U5A5RJT6**, LQFP64, 4 MB flash, TrustZone disabled in the previously working target. Do not substitute U575/U595 target definitions or change option bytes.

| Numpad RevE J1 signal | J1 pin | Devboard RevK signal | Devboard connector |
|---|---:|---|---|
| ROW1 | 5 | PC0 | J2 pin 8 |
| COL1 | 27 | PB4 | J2 pin 18 |
| GND | 1 | GND | J2 pin 26 |
| 3.3V | 25 | 3V3 | J2 pin 1 |

Pin numbers come from the KiCad RevK/RevE files; use signal labels to avoid header orientation mistakes. This row/column intersection is **MX2**, not MX1.

**Update 2026-10-22 — full 4 × 4 matrix wiring.** The firmware now scans all rows and columns. Devboard pins are given by NAME because the board has no numbers printed on it. ROW_1 = PC0, ROW_2 = PC1, ROW_3 = PC2, ROW_4 = PC3 (open-drain, one pulled low at a time); COL_1 = PB4, COL_2 = PB5, COL_3 = PB6, COL_4 = PB7 (inputs with pull-up); plus GND and 3.3V. Numpad J1 pins from KiCad: ROW_1 5, ROW_2 4, ROW_3 3, ROW_4 2, COL_1 27, COL_2 28, COL_3 29, COL_4 30, GND 1, 3.3V 25. The bottom-right position is the rotary encoder (ENC_CLK J1 21, ENC_DT 22, ENC_SW 23), not part of the matrix and not yet used. Not wired: 5V, LED_IN/LED_OUT, TMR outputs. A missing row or column wire disables the keys that use it.

ROW1 is open drain and held low. COL1 has an internal pull-up; the switch and series diode pull it low when pressed. PC1–PC3 are open-drain released outputs. PB5–PB7, encoder, TMR, and RGB outputs are not used. Leave unused numpad signals disconnected for the first test.

Devboard LED D1: PA5, active high. User button PC13 is unused in this application.
USB-C: PA11=D−, PA12=D+, embedded HS PHY, 16 MHz HSE crystal. CPU runs at 160 MHz from HSE PLL. Firmware enables VDDUSB and the internal HS transceiver supply with VOS1; it does not enable SMPS.

SWD: J7 pin 1 reference voltage, pin 2 SWDIO, pin 3 GND, pin 4 SWCLK, pin 10 NRST. Keep the known working ST-Link wiring and board power arrangement. Do not assume a clone programmer's 3.3V output is a voltage-sense input or connect two power outputs together.

BOOT selects ROM bootloader startup while RST restarts the MCU. Normal keyboard operation uses BOOT released. No bootloader replacement or security option-byte changes are part of this project.
