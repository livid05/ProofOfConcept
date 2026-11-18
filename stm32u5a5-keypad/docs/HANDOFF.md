# Shared handoff

Both assistants work in this exact project, opened through the workspace file. There is one active application under firmware/. Do not create assistant-specific copies of the firmware.

Current scope: make MX2 produce a reliable USB A-key press/release. Confirm the pending bench tests in STATUS.md before expanding the matrix. TMR comes later.

Read README.md, HARDWARE.md, STATUS.md and the commented source before editing. Record the concrete change, files touched, build result and pending board test here or in CHANGELOG.md. Distinguish compilation from physical verification. Use one assistant to edit a given file at a time; ask the other to review or work on documentation. Read current file contents before patching another assistant's work.

Keep wiring/key/timing settings in board_config.h. Preserve detailed comments, vendor licenses and the archived baseline. The original project under 11 Capstone STM32U595 is a separate fallback. Never regenerate its old .ioc into this application.

Next action: Tyler flashes build/keypad.hex or uses the flash task, then reports USB enumeration and the Notepad MX2 test. If it fails, collect the LED behavior and USB/debug state instead of assuming the probe is broken.


### 2026-10-20 — USB DFU workflow

Added `tools/flash-usb.ps1`, separate Flash USB and combined Build + Flash USB tasks, and bottom status-bar buttons using the already installed Task Buttons extension. USB CLI syntax checked against installed CubeProgrammer help; script configuration/paths and task wiring validated. Firmware unchanged. BOOT/RST remains manual; no automatic board flashing performed during setup. USB hardware flashing/typing validation remains pending.


### 2026-10-20 — 8K update

Tyler confirmed the previous USB build types A, repeats while held and stops after release. USBView screenshot confirmed High Speed, product Keypad U5A5 - MX2, bInterval=4 (1K). These are real prior-build results, not measurements of the new firmware.

Updated HS interrupt bInterval to 1 (8K); FS fallback remains 1K. Added TIM2 1 MHz free-running microsecond clock, nominal 125 us input sampling and scan timing counters. Eager first-closure press with 10 ms stable-release filtering replaces the old 10 ms delayed press; optional conservative mode remains in board_config.h. Queued USB events are serviced between scans too. Detailed comments added. Prior working image/sources saved under archive/verified-usb-1k.

Arm build uses warnings-as-errors. Native tests compile actual key_switch.c against simulated GPIO/time/USB; image checks inspect the actual linked descriptors. Physical 8K enumeration, tap reliability and latency remain pending. Next: use Build + Flash USB, reset BOOT released, unplug/reconnect, confirm USBView High + bInterval=0x01, then retest MX2. No automatic board programming performed.


### 2026-10-22 — 4 x 4 matrix integrated

Scope is now the 15-key matrix at 8K. Files: new key_matrix.c, matrix_hw.c, key_matrix.h, matrix_config.h; key_switch.c removed (archived); usb_keyboard.c gains Keyboard_SendReport (full 8-byte report) and Keyboard_Idle repeats the whole report; main.c calls Matrix_GpioInit(); keyboard.h declares Keyboard_SendReport; tools/test-host.ps1 and tests/host now test the matrix. Previous sources, tests, build and the validated 8K hex are in archive/verified-8k-single-key/. Design record and integration notes: future_work/2026-10-22_matrix4x4/.

Compiled and PC-tested only. Next action: Tyler wires all 8 matrix signals plus GND/3V3 (devboard pins PC0-PC3 and PB4-PB7), uses Build + Flash USB, then follows the acceptance list in STATUS.md. Encoder (bottom right), TMR, RGB remain untouched. Always name devboard pins (PC0, PB4...), never header numbers.


### 2026-10-22 — column 3/4 swap fixed in software

First bench run showed columns 3 and 4 reading swapped; matrix_config.h MATRIX_COL_PINS is now PB4, PB5, PB7, PB6 and tests/host/harness_hw.c expects it. Pending: Tyler rebuilds/flashes and confirms `abcd efgh ijkl mno`, then the rest of the acceptance list in STATUS.md.


### 2026-10-22 — 8K measured

Rate-test result is recorded in STATUS.md (7999 reports/s, 8K confirmed). The board currently runs the rate-test image until Tyler re-flashes the keyboard (Build + Flash USB). The matrix keymap fix (columns 3/4) still needs its on-hardware recheck.

### 2026-11-18 — TMR sensor test image

New isolated diagnostic in future_work/2026-11-18_tmr_diag (README there). No firmware/ or .vscode/ file changed; Keypad-U5A5.code-workspace gained four TMR tasks and two buttons (TMR Build + Flash, TMR Console). Next: Tyler enters DFU, clicks TMR Build + Flash, presses RST, clicks TMR Console, reports the self-test table and the key press results. If K labels are wrong, use the console's 'm' learn mode and paste the printed line into tmr_config.h. Restore the keyboard with Build + Flash USB.


### 2026-11-18 — TMR fast-scan image ready to build

Status: 14 of 15 TMR keys PASS (observed); K4 open (parked). Supply fix: numpad sensors now use the mux header 3V3/GND. Blocking scan measured 730 us, so 8K needs the new pipelined scan.
Next for Tyler: run `tools\build-tmr-fast.ps1` (or Build TMR fast), enter DFU, flash with Flash TMR fast, open TMR fast console, press `t` (hands off), then `x`, paste the sweep table. Then `f` and `g` and test keys. Folder: future_work/2026-11-18_tmr_fast (README there). Restore keyboard with Build + Flash USB.
Open: K4 meter check, matrix cols 3/4 recheck. GitHub not updated (this folder is not a git repo; needs Tyler's decision and explicit yes before any push).
