# TMR fast-scan test image (2026-11-18)

Diagnostic image, NOT the keyboard firmware. Copy of `2026-11-18_tmr_diag` (left untouched) with:

- `x` speed sweep: for 9 fast-scan settings prints scan time, settle time, noise (rms and peak-to-peak), shift against a slow reference, and channel-to-channel memory in ppm.
- `f` fast mode: register-level pipelined scan every 125 us (8 kHz). Settings come from `TMR_FAST_*` in `firmware/Core/Src/tmr_config.h`.
- `g` rapid trigger: UP when the reading backs off 60 mV from the press peak, DOWN again after 60 mV deeper than the valley.

## Run
1. `tools\build-tmr-fast.ps1` (or the "Build TMR fast" task). Output: `output\arm-build\tmr-fast.hex`.
2. Enter DFU (BOOT + RST), then `tools\flash-tmr-fast-usb.ps1`.
3. `tools\tmr-console.ps1` in a normal terminal. If single keys are not read, type a letter then Enter.
4. Hands off: `t`, then `x`. Keep a few switches in the sockets so neighbouring inputs differ (needed for the memory figure).
5. Then `f` and press keys; `g` to compare release behaviour.

## Reading the sweep
Pick the quietest row whose average scan time is well under 125 us with a small memory figure. If "memory" shows a small number of steps, the figure is not trustworthy.

## Status (2026-11-18)
- Host unit tests (rapid-trigger logic): passed under gcc.
- Hardware files: syntax-checked with gcc against the project headers, no warnings. NOT built with the ARM toolchain.
- Hardware behaviour of fast scan, sweep numbers, rapid trigger on real keys: PENDING, not yet run on the board.
- K4 (J8 hole 4) still open, so 14 of 15 keys usable.
