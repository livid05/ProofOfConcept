# Roadmap

Ordered by what unblocks what. "Hardware" items need the bench; "firmware" items can be done at a desk first.

## Now: finish the numpad bring-up

1. **Hardware.** Build and flash the fast-scan image, run `t` then `x`, and record the sweep table in `docs/STATUS.md`. This turns the 8 kHz scan from an estimate into a measured result.
2. **Firmware.** Choose the fast-scan setting from the sweep and set it in `tmr_config.h`.
3. **Hardware.** Fix K4: meter J1 pin 20 to J8 hole 4, and pin 20 to pin 21 (ENC_CLK) to look for a bridge.
4. **Hardware.** Recheck matrix columns 3 and 4 (a to o should type in order), and meter the numpad 3.3 V.

## Next: analog keys in the keyboard firmware

5. Move the TMR scan and the analysis code from the test image into the keyboard firmware, keeping the 8 kHz HID path.
6. Per-key calibration: store each key's rest and travel instead of using fixed thresholds, and re-zero safely.
7. Adjustable actuation and rapid trigger per key, with tuned (not default) distances and a noise check at the chosen scan setting.
8. Key profiles: a deeper, calmer setting for typing and a sensitive one for gaming; rapid trigger off on keys fitted with mechanical switches.
9. Rotary encoder and RGB. Notes from the Rev K review: SK6812 data needs 0.7 x VDD, so with a 5 V supply the 3.3 V MCU pin needs a 5 V buffer (an AHCT part) or a 5 V-tolerant open-drain pin with a pull-up; 15 LEDs at full white draw about 540 mA, over USB's 500 mA, so cap brightness near 60 % white; remove R11 before fitting a coin cell on VBAT.

## Then: low power and wireless

10. Adaptive scan rate: fast only while a key is moving, slow when idle, with a wake on activity. Measure the added wake latency before choosing the idle rate.
11. Power measurements on real hardware (idle, active, wireless).
12. Radio coprocessor (nRF52840 / BMD-340) bridge for BLE and 2.4 GHz, reusing what was learned in the RP2040 proof of concept.

## Hardware

13. Decide whether to order the TriMode60 board (Rev E) once the numpad results are in; its sensing and USB sections should be validated on the dev board first.
14. Keep the dev board stocked with parts: the U5A5 (JLC had very few in stock in early October) and the sensors, which are consigned.

## Housekeeping

15. Add a license for the author's own code, and replace ST's demonstration USB ID before sharing any product.
16. Add real KiCad exports (schematic PDFs and board renders) for Rev K and Numpad Rev E to `hardware/images/`; the current images are layout comparisons from earlier revisions.
17. After each bench session, update `stm32u5a5-keypad/docs/STATUS.md` and `HANDOFF.md` with what was observed and what is still pending.
