# Project story

This is the path the project took, in order, with the reason for each turn. Dates come from file records and the revision notes; where a date is not known it is left out.

## 1. RP2040 proof of concept

The first goal was a wireless 4x4 macro pad. An RP2040 runs QMK (matrix, rotary encoder, 15 WS2812 LEDs, VIA remapping) and sends key events over UART at 1 Mbaud to a Nordic nRF52840 module. That module either speaks BLE HID or a 2.4 GHz proprietary link to a USB dongle. A Python script measured latency for both. A CD74HC4067 analog mux was designed in as a reserved path for analog key sensing later.

What it proved: the wireless chain works end to end, and QMK is a quick way to get a working keypad. The next goal was set by the capstone: take what the QMK version does and build the firmware from scratch, optimized for fast and reliable connection, low power draw and robust features, on an STM32 with a high-speed USB PHY. Full detail is in [rp2040-proof-of-concept.md](rp2040-proof-of-concept.md).

## 2. Choosing TMR sensors and the first numpad

Tunnel magnetoresistance sensors give an analog position signal for each key, which allows adjustable actuation points and rapid trigger. The author designed a 15-key numpad (Numpad_TMR1) around the TMR2615 sensor, with a hot-swap MX socket and one sensor per key, plus SK6812 LEDs and a rotary encoder footprint. The first fabricated board read every sensor as open. The cause, found during the Rev B work, was that the sensor footprint was sized for the 1.6 x 1.6 mm TMR2615F while the parts bought were the 2 x 2 mm TMR2615D, so the small pins missed their pads. Rev B added a correct footprint built from the datasheet package drawing.

## 3. A development board with a real USB high-speed PHY

To write firmware before the full keyboard exists, a development board was designed (U575 Rev A through U5A5 Rev K). Its key decisions, in order:

- Professor review of Rev A led to short, via-free USB pairs, a UART connector instead of QSPI flash, and the MCU placed next to the USB connector.
- Rev C added the on-board 16-channel mux and a 2x10 header (J8) for up to 16 sensor outputs, each with an RC filter.
- Rev G moved from the STM32U575 (full-speed USB only) to the U595, which has an embedded high-speed PHY, because 8 kHz polling needs USB high speed.
- Rev H to J tuned layout, board size (for JLC's low-cost 4-layer pricing) and the USB pair to about 90 ohm on JLC's default stackup.
- Rev K changed the part to the cheaper, pin-compatible STM32U5A5 and changed the mux filter from 1 k / 10 nF to 100 ohm / 4.7 nF so the mux settles in about 2.5 us per channel instead of about 50 us.

See [02-hardware-evolution.md](02-hardware-evolution.md).

## 4. Firmware from scratch, in measured steps

Firmware was built up in small verified stages, each kept in its own folder so the last working state is never lost:

1. A one-key image proving the U5A5 enumerates and types over USB at 1 kHz.
2. The same at 8 kHz (HS interrupt bInterval 1). A separate rate-test image then streamed numbered reports to a PC reader: 7999 reports per second, 79,995 received in 10 s, 0 dropped, gaps median 125 us.
3. A 4x4 digital matrix scanner at the same cadence (six-key rollover, ordered queue). The first bench run typed columns 3 and 4 swapped, which was fixed in software; the recheck is pending.

## 5. TMR bring-up on the numpad (2026-11-18)

A separate diagnostic image reads all 16 mux inputs, runs an open-input self-test (pull-up and pull-down probes), measures rest voltage and noise, then tracks press and release over a USB serial console. The bench session found wiring faults, a shared-supply problem and several misleading readings, and ended with 14 of 15 keys passing. The full account is in [04-bench-bringup-2026-11-18.md](04-bench-bringup-2026-11-18.md).

## 6. The scan-speed problem

The diagnostic's blocking scan took 730 us for 16 inputs. An 8 kHz report needs a new reading every 125 us, so that scan cannot feed 8 kHz. A new image (`future_work/2026-11-18_tmr_fast`) uses a pipelined register-level scan, a sweep command that measures noise and channel-to-channel memory for nine settings, and a rapid-trigger release rule. It is compiled for syntax and unit-tested on a PC, and has not yet run on the board. See [06-8k-scanning-and-market-research.md](06-8k-scanning-and-market-research.md).

## 7. The full keyboard (TriMode60)

In parallel, a 60% tri-mode keyboard board was designed through Rev E: STM32U595 with USB high speed, a u-blox BMD-340 (nRF52840) radio module for BLE and 2.4 GHz, a BQ25895 charger with USB-C PD sink, a fuel gauge, and RGB. It is a design: the repository holds no build or test results for it. The numpad and dev board work is the way of de-risking its sensing and USB before that board is ordered.

## Going forward

See [07-roadmap.md](07-roadmap.md).
