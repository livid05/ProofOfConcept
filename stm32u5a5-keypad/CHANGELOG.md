# Changelog

## 2026-10-20 — Initial one-key USB project

- Created shared VS Code workspace under 13 Keypad U5A5.
- Preserved earlier bring-up configuration/header as an archive.
- Added bare-metal USB boot keyboard, one-key debounce, queued press/release, reconnect synchronization and status LED.
- Added build/flash scripts, wiring docs, detailed source comments, handoff instructions and hardware acceptance checklist.
- Compiled successfully; physical USB typing verification pending.


### 2026-10-20 — USB DFU workflow

Added `tools/flash-usb.ps1`, separate Flash USB and combined Build + Flash USB tasks, and bottom status-bar buttons using the already installed Task Buttons extension. USB CLI syntax checked against installed CubeProgrammer help; script configuration/paths and task wiring validated. Firmware unchanged. BOOT/RST remains manual; no automatic board flashing performed during setup. USB hardware flashing/typing validation remains pending.


### 2026-10-20 — 8K update

Tyler confirmed the previous USB build types A, repeats while held and stops after release. USBView screenshot confirmed High Speed, product Keypad U5A5 - MX2, bInterval=4 (1K). These are real prior-build results, not measurements of the new firmware.

Updated HS interrupt bInterval to 1 (8K); FS fallback remains 1K. Added TIM2 1 MHz free-running microsecond clock, nominal 125 us input sampling and scan timing counters. Eager first-closure press with 10 ms stable-release filtering replaces the old 10 ms delayed press; optional conservative mode remains in board_config.h. Queued USB events are serviced between scans too. Detailed comments added. Prior working image/sources saved under archive/verified-usb-1k.

Arm build uses warnings-as-errors. Native tests compile actual key_switch.c against simulated GPIO/time/USB; image checks inspect the actual linked descriptors. Physical 8K enumeration, tap reliability and latency remain pending. Next: use Build + Flash USB, reset BOOT released, unplug/reconnect, confirm USBView High + bInterval=0x01, then retest MX2. No automatic board programming performed.


## 2026-10-22 — 4 x 4 matrix, 8K

- Added key_matrix.c / matrix_hw.c / key_matrix.h / matrix_config.h: full 15-key scan, per-key debounce, six-key reports, ordered queue, reconnect and overflow handling.
- usb_keyboard.c: Keyboard_SendReport (full report); idle repeat sends the whole report. main.c: Matrix_GpioInit(). keyboard.h: new prototype.
- Replaced single-key host tests with matrix tests (logic, electrical model, conservative mode); tools/test-host.ps1 updated.
- Archived the verified one-key 8K firmware, sources, tests and build under archive/verified-8k-single-key.
- Build, host tests and image check pass; hardware verification pending.

- Bench fix: matrix columns 3 and 4 were read swapped; MATRIX_COL_PINS is PB4, PB5, PB7, PB6. Tests updated.

- Added future_work/2026-10-22_rate_diag (8K USB rate-test image and PC measurement script). Measured 7999 reports/s, 8K confirmed.

## 2026-11-18 — TMR sensor test image

- Added future_work/2026-11-18_tmr_diag: TMR2615 self-test and press monitor over a USB COM port, built from the project's own main.c/usb_port.c/USB core (diagnostic image, keyboard untouched).
- Workspace file: added TMR build/flash/console tasks and buttons (backup in the diag folder).
- Sandbox ARM build and host tests pass; hardware test pending.

## 2026-11-18 — TMR fast-scan test image

- Added future_work/2026-11-18_tmr_fast: pipelined register-level scan (125 us target), `x` sweep, `f` fast mode, `g` rapid-trigger release (new TmrA_KeyUpdateRT plus host tests). tmr_diag folder unchanged.
- Workspace file: four TMR fast tasks and two buttons added (backup in the new folder's vscode_backup).
- Host tests and gcc syntax check pass; ARM build and hardware test pending.
