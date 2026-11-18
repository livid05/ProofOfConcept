# 4 x 4 key matrix firmware (design record; INTEGRATED 2026-10-22)

**Status update:** Tyler authorized integration on 2026-10-22 and these sources were
copied into `firmware/` (see `docs/STATUS.md`). The validated one-key 8K firmware and
its build are archived in `archive/verified-8k-single-key/`. This folder remains as the
design record; its sections below describe the state when it was written, before integration.

Created 2026-10-22. This folder is **isolated**: nothing in it is compiled by the
normal project build, and no existing file was changed at that time. The existing
`firmware/`, `docs/`, `build/`, `archive/` and `tools/` were exactly as they were
(`build/keypad.bin` had SHA-256 `872DCEF3...4BA3`, the validated 8K image).

## What it does

Scans all 15 keys of the Numpad RevE (4 rows x 4 columns, diode per key) instead
of the single MX2 switch, at the same 125 us / 8K cadence, and sends a normal
USB boot-keyboard report holding up to six keys at once.

* Rows ROW1..ROW4 = devboard pins **PC0, PC1, PC2, PC3** (open-drain, one pulled low at a time).
* Columns COL1..COL4 = devboard pins **PB4, PB5, PB6, PB7** (inputs with pull-up).
* The bottom-right position (ROW4/COL4) is the rotary encoder, which is not in the
  matrix, so it is empty in the keymap. The encoder is **not** handled yet.
* Bring-up keymap types `a b c d / e f g h / i j k l / m n o` (row by row), so each
  key identifies itself. Change it in `firmware/Core/Inc/matrix_config.h`.
* Same safety rules as the one-key firmware: no lost press or release, ordered
  queue, held-at-plug-in keys stay silent, overflow recovers to "all keys up".

## What is verified (and what is not)

| Check | Result |
|---|---|
| Logic test (`harness_logic.c`): all 15 keys, chords, press order, bounce, 10 ms release, 6-key limit, busy-USB ordering, reconnect with held keys, queue overflow, 32-bit clock wrap, 8000 scans/second, modifiers, keymap | **Pass** |
| Conservative-mode test (`harness_noneager.c`, eager press off) | **Pass** |
| Pin/electrical model test (`harness_hw.c`): PC0-PC3 / PB4-PB7 mapping, one row low at a time, rows released before output mode and after each scan, >= 6 us settle, all 65,536 key patterns read back exactly | **Pass** |
| Deliberately broken copies of the logic (release never sent, no bounce filter, no suppression, queue kept on overflow, wrong key order) | **All caught** by the tests |
| Full isolated ARM build, `-Wall -Wextra -Werror`, real HAL/USB sources | **Pass**: 22,428 B flash (+788 B vs the 21,640 B one-key image), 5,320 B RAM |
| Linked-image check (`tools/verify_arm_image.py`): vectors, USB descriptors, 8K interval, 8-byte report | **Pass** |
| Flash script dry check (`flash-matrix-usb.ps1 -CheckOnly`) | **Pass**, no hardware touched |
| **Any real hardware**: wiring, enumeration, typing, scan duration, ghosting, settle time | **NOT TESTED** |

`MATRIX_SETTLE_US = 6` is an estimate from the pull-up resistance and wiring
capacitance. Check `matrix_scan_max_us` in a debugger (full scan should be near
24 us) and, if a key types a neighbour's letter, raise the settle time.
The eager-press-off suppression path is covered by the conservative-mode test;
with eager press on, held-key silence comes from the report being rebuilt only
from press/release events (the suppression mask is a second safety net).

## Folder map

| Path | Purpose |
|---|---|
| `firmware/Core/Inc/matrix_config.h` | pins, keymap, settle time (all adjustable settings) |
| `firmware/Core/Inc/key_matrix.h` | interface |
| `firmware/Core/Src/matrix_hw.c` | GPIO scan (only file touching the pins) |
| `firmware/Core/Src/key_matrix.c` | debounce, reports, queue, `Keyboard_Task` (replaces `key_switch.c`) |
| `integration/proposed/` | edited COPIES of `usb_keyboard.c`, `keyboard.h`, `main.c` |
| `integration/patches/*.diff` | exact changes versus the originals (111 diff lines total) |
| `tests/host/` | PC tests and fake HAL |
| `tools/run-host-tests.ps1` | build + run all PC tests (needs Visual Studio 2022 `cl.exe`) |
| `tools/build-arm-isolated.ps1` | full ARM build into `output/arm-build/` (does not use `build/`) |
| `tools/verify_arm_image.py` | checks the linked matrix image |
| `tools/flash-matrix-usb.ps1` | flashes the isolated test image over USB DFU (**writes hardware; only run on purpose**) |
| `output/` | generated test programs, mutants and the ARM image |

See `INTEGRATION.md` for wiring, bench test and how to adopt or roll back.
