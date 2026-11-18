# Status — 2026-10-20

## Previously confirmed by Tyler

- Original RevK firmware builds, flashes and runs through ST-Link/OpenOCD at 100 kHz.
- LED operation, standalone USB power boot, and ROM USB DFU flashing work.
- MX2 mechanical input works through ROW1/PC0 and COL1/PB4 in the earlier LED test.

These are earlier bring-up results, not verification of this new USB firmware.

## New project

- One-switch USB HID implementation prepared; detailed source comments and shared assistant instructions added.
- Full Arm GCC 14.3.1 build and link completed for STM32U5A5xx, 4 MB flash, non-secure configuration.
- No target programming or host typing test performed by the agent.

## Required hardware acceptance

| Check | Expected | Result |
|---|---|---|
| USB connection | Keyboard recognized without custom driver, slow LED blink | Pending |
| 20 short MX2 taps | Exactly 20 A-key characters, no extra characters | Pending |
| Hold/release MX2 | OS repeat while held; stops after release | Pending |
| Plug in while held | No key until released and pressed again | Pending |
| Unplug/replug | Re-enumerates and accepts a fresh press | Pending |
| Reset without BOOT | Returns to keyboard firmware | Pending |
| Idle 5 minutes | No unsolicited characters | Pending |
| Queue overflow counter | Remains zero in ordinary operation | Pending |

Record Windows layout, USB cable/port, firmware hash, date and observed results here. TMR, encoder, RGB, full matrix, rollover, latency characterization and production readiness are out of scope for this first test.


### 2026-10-20 — USB DFU workflow

Added `tools/flash-usb.ps1`, separate Flash USB and combined Build + Flash USB tasks, and bottom status-bar buttons using the already installed Task Buttons extension. USB CLI syntax checked against installed CubeProgrammer help; script configuration/paths and task wiring validated. Firmware unchanged. BOOT/RST remains manual; no automatic board flashing performed during setup. USB hardware flashing/typing validation remains pending.


### 2026-10-20 — 8K update

Tyler confirmed the previous USB build types A, repeats while held and stops after release. USBView screenshot confirmed High Speed, product Keypad U5A5 - MX2, bInterval=4 (1K). These are real prior-build results, not measurements of the new firmware.

Updated HS interrupt bInterval to 1 (8K); FS fallback remains 1K. Added TIM2 1 MHz free-running microsecond clock, nominal 125 us input sampling and scan timing counters. Eager first-closure press with 10 ms stable-release filtering replaces the old 10 ms delayed press; optional conservative mode remains in board_config.h. Queued USB events are serviced between scans too. Detailed comments added. Prior working image/sources saved under archive/verified-usb-1k.

Arm build uses warnings-as-errors. Native tests compile actual key_switch.c against simulated GPIO/time/USB; image checks inspect the actual linked descriptors. Physical 8K enumeration, tap reliability and latency remain pending. Next: use Build + Flash USB, reset BOOT released, unplug/reconnect, confirm USBView High + bInterval=0x01, then retest MX2. No automatic board programming performed.


### 2026-10-22 — 4 x 4 matrix integrated (compiled and PC-tested only)

Replaced the single-switch scanner with a 4 x 4 matrix scanner at the same 8K cadence: all 15 keys, up to six at once, press order kept, ordered queue, held-at-plug-in suppression, overflow recovery. Bring-up keymap types a..o row by row; the encoder position (bottom right) is empty. Wiring: ROW_1..4 = PC0..PC3, COL_1..4 = PB4..PB7 (see HARDWARE.md). The Tyler-requested integration was made from `future_work/2026-10-22_matrix4x4/`, which keeps the full design record, mutation checks and rollback notes.

Software evidence: Arm GCC build with -Wall -Wextra -Werror passes (22,428 B flash, 5,320 B RAM); tools/test-host.ps1 passes (logic, electrical-model and conservative-mode tests, including deliberately broken variants being caught); tools/verify_image.py passes on the linked image (8K endpoint interval, 8-byte report). BIN SHA-256 of this build: 11E9EF64F31E7116B8E2C0831A79C4B51B1EDFB5DED931CC5A4C515CBE644217.

NOT done: no flashing by the agent; no bench test of the matrix or of 8K enumeration; row-settle time (MATRIX_SETTLE_US = 6) and full-scan duration (matrix_scan_max_us) unmeasured; encoder, TMR, RGB not implemented. Rollback: archive/verified-8k-single-key/ (validated one-key 8K image and sources).

Hardware acceptance (all Pending): USBView High Speed and bInterval 0x01; each of the 15 keys types its letter; two and three keys together; hold repeats and release stops; unplug/replug; 5 minutes idle with no stray letters; key_queue_overflow stays 0. If a key types a neighbour's letter, raise MATRIX_SETTLE_US.


### 2026-10-22 — first bench result and column fix

Tyler flashed the 4 x 4 build and pressed the keys left to right, top to bottom. Typed `abdcefhgijlkmn` instead of `abcdefghijklmno`: the third and fourth columns were swapped (rows 1-3 flipped their last two letters; the third key of row 4 typed nothing because it read as the empty encoder slot). Columns 1-2 and all four rows were correct. Fix: MATRIX_COL_PINS now reads PB4, PB5, PB7, PB6 (matrix_config.h); the electrical-model test expects that order. Rebuilt; host tests and image check pass (BIN SHA-256 A42CAAD2CDC6A758A2CBE… see build). NOT yet re-tested on hardware. If the COL_3 and COL_4 wires are later swapped by hand, restore PB4, PB5, PB6, PB7.
Observed so far: matrix keys type; 8K enumeration (USBView bInterval 0x01), chords, idle and replug still unconfirmed.


### 2026-10-22 — 8K USB report rate MEASURED

A separate rate-test image (future_work/2026-10-22_rate_diag; same USB core, endpoint 0x81, High Speed, bInterval 1) streamed numbered reports to a PC reader for 10 s: 7999 reports/s by the device clock, 79,995 received, 0 dropped, gaps median 125 us / 99th percentile 133 us / max 1125 us, 100.0% of gaps near 125 us (under 0.05% off-grid). Verdict: 8K confirmed for the USB report path on Tyler's PC (direct port). Not a key-to-USB latency or scan-jitter measurement. The keyboard image's own descriptor (USBView bInterval 0x01) is still worth a screenshot for the report. After the test the board holds the rate-test image; restore the keyboard with Build + Flash USB.

### 2026-11-18 — TMR sensor test image (compiled and PC-tested only)

Tyler soldered the 15 TMR2615 sensors on Numpad RevE (no supply shorts; diode test normal) and wired J1 TMR_OUT pins to devboard J8. Added `future_work/2026-11-18_tmr_diag/`: an isolated test image (like the rate test) that reuses main.c, clocks, usb_port.c and the ST USB core, replaces key_matrix.c/usb_keyboard.c with a TMR self-test and a USB CDC COM port (0483:5712). Per input: open-input probe (PC4 pull-up/pull-down), rest voltage, noise; then 1 kHz scan with press/release events, summary, wiring-order learn mode. Tyler's J8 wiring order is in tmr_config.h.

Software evidence: ARM build with project flags (-Werror) passes on a Linux build host (GCC 13.2, same HAL v1.6.2 / CMSIS 1.4.2 files): 39,879 B flash, 16,424 B RAM; linked descriptors checked (IAD + CDC ACM, bulk 512/64); host tests of tmr_analysis.c pass, deliberately broken variants caught. firmware/ unchanged; .vscode/ unchanged; TMR tasks/buttons added to Keypad-U5A5.code-workspace (original copied to the diag folder's vscode_backup/).

NOT done: no build with CubeIDE GCC 14.3 on Tyler's PC yet, no flashing by the agent, nothing observed on hardware (enumeration, ADC readings, sensor statuses all pending). Restore keyboard: Build + Flash USB.


### 2026-11-18 — TMR bench results and fast-scan image

Observed by Tyler on the bench: after moving the numpad sensor supply to the mux header's 3V3/GND, all rests settle together (about 49.3 to 50.1% of supply for no-switch keys, +0 mV deltas live). Open sensors K5, K10, K15 were resoldered. Self-test: 14 of 15 keys PASS, swings about -405 to -566 mV. K4 (J8 hole 4, J1 pin 20) is still OPEN and parked. Hole 2 is deliberately empty and always reads OPEN. Scan measured 730 us for 16 inputs (max 788 us), 279902 scans, 0 USB bytes dropped. Long "held" events in the diag were the release rule (needs a return within 100 mV of rest), not a hardware fault; spam taps without full release give one long hold.

New `future_work/2026-11-18_tmr_fast/` (diag folder untouched): pipelined register-level scan aimed at 125 us (8K), `x` speed sweep (noise, settle, channel memory vs a slow reference), `f` fast mode, `g` rapid-trigger release. Evidence: host tests for the rapid-trigger logic pass; hardware files syntax-checked with gcc against project headers, no warnings.

NOT done: no ARM build of this image, no flashing, nothing run on hardware. Fast-scan time, noise and memory figures are PENDING. Also pending: K4 meter check (J1 pin 20 to J8 hole 4), matrix columns 3/4 recheck, numpad 3.3 V meter check.
