# Build, flash and test guide

All commands are PowerShell on Windows and run from the firmware folder, `stm32u5a5-keypad/`. Test images live in `stm32u5a5-keypad/future_work/` and never replace the keyboard firmware; to go back, rebuild and flash the normal keyboard image.

## Tools needed

- Arm GNU toolchain (the project was built with the one in STM32CubeIDE, GCC 14.3; a sandbox build with GCC 13.2 also passed).
- STM32CubeProgrammer, for its command-line programmer (USB DFU, no ST-Link needed).
- A normal terminal (not the editor's task panel) for the serial console.
- A USB data cable.

## Keyboard firmware

1. `tools\build.ps1` builds with warnings treated as errors.
2. Put the board in DFU: hold BOOT, press and release RST, release BOOT.
3. `tools\flash-usb.ps1` flashes and verifies.
4. Press RST (BOOT released), replug, and check the USB descriptor in USBView (High Speed, bInterval 0x01).
5. `tools\test-host.ps1` runs the PC-side logic tests.

## TMR test images

Two images exist: the original diagnostic (`future_work/2026-11-18_tmr_diag`) and the fast-scan image (`future_work/2026-11-18_tmr_fast`). Use the fast image for new work; the original is kept as the known reference.

1. `future_work\2026-11-18_tmr_fast\tools\build-tmr-fast.ps1` writes `output\arm-build\tmr-fast.hex` (nothing is flashed).
2. Enter DFU, then `tools\flash-tmr-fast-usb.ps1`.
3. Press RST. The board appears as a serial port (USB 0483:5712).
4. Run `tools\tmr-console.ps1` in a normal terminal. If the window cannot read single keys, type a command letter and press Enter (this fallback is untested).
5. `tools\run-host-tests.ps1` runs the PC-side tests of the analysis code, including rapid trigger.

The workspace file has buttons for these (TMR Fast Build + Flash and TMR Fast Console).

## Console commands

| Key | What it does |
|---|---|
| t | Self-test: open-input probe, rest voltage, noise. Hands off the keys |
| s | Summary of presses, peaks and scan timing |
| l | Live line of per-key millivolt change on or off |
| r | Raw voltages |
| z | Re-zero, hands off. Do this after moving switches |
| c | Clear counts |
| m | Learn wiring order, then paste the printed line into `tmr_config.h` |
| x | Speed sweep (fast image only): about 5 s, hands off, run `t` first |
| f | Fast scan on or off (fast image only), every 125 us |
| g | Rapid-trigger release on or off (fast image only) |
| h | Help |

## Reading the sweep (`x`)

Each row scans all 16 inputs 2000 times with one setting and prints: average and maximum scan time, the shortest settle time from a mux change to the end of the sampling window, noise (rms and worst peak-to-peak, in microvolts), the shift of the average against a slow fully-settled reference row, and a "memory" figure in parts per million. Memory estimates how much of the previous input's voltage leaks into the next reading; 1000 ppm is 0.1 %. It needs some differences between neighbouring inputs (at least 40 mV), so keep a few switches in their sockets. If it reports few steps, do not trust the figure.

Pick the quietest row that finishes well inside 125 us with a small memory figure, then set `TMR_FAST_SAMPLING`, `TMR_FAST_SAMPLES` and `TMR_FAST_PREWAIT_CYCLES` in `tmr_config.h`.

## Status of these instructions

The keyboard build, flash and host-test steps were run during the project. The fast-image steps have not been run on hardware, so treat that part as a first draft and correct this page after the first real run.
