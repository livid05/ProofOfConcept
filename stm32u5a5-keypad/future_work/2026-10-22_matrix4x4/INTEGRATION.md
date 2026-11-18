# Integration, bench test and rollback

Nothing here has been applied. Do these steps only when Tyler says so.

## 1. Wiring for the bench test

Already connected: ROW1 -> **PC0**, COL1 -> **PB4**, GND, 3.3V.
Add six wires (devboard pin names; Numpad net names from the KiCad RevE file,
with the J1 pin numbers in brackets in case the numpad silkscreen shows them):

| Numpad net | Devboard pin |
|---|---|
| ROW_2 (J1 pin 4) | **PC1** |
| ROW_3 (J1 pin 3) | **PC2** |
| ROW_4 (J1 pin 2) | **PC3** |
| COL_2 (J1 pin 28) | **PB5** |
| COL_3 (J1 pin 29) | **PB6** |
| COL_4 (J1 pin 30) | **PB7** |

Still not needed: 5V, RGB (LED_IN/LED_OUT), TMR outputs, encoder (ENC_CLK/ENC_DT/ENC_SW).
Any row/column pairing works if you tell the firmware (edit `MATRIX_ROW_PINS` /
`MATRIX_COL_PINS`), but the keymap letters then follow the pins, not the board.

## 2. Try it WITHOUT touching the project (recommended first)

1. `tools\run-host-tests.ps1` and `tools\build-arm-isolated.ps1` (already pass; rerun if anything changes).
2. Put the devboard in DFU mode (hold BOOT, press and release RST, release BOOT), close CubeProgrammer.
3. `tools\flash-matrix-usb.ps1`, then press RST with BOOT released.
4. In Notepad press each key. Expected, row by row: `a b c d / e f g h / i j k l / m n o`.
5. Check: two keys together both appear, holding repeats, release stops, no stray letters at idle, unplug/replug works.
6. If something is wrong, the known-good image is untouched: DFU again and use the normal **Flash USB** task (`build\keypad.hex`, the 8K single-key firmware).

## 3. Adopt it in the real project (needs Tyler's OK)

The build GLOBs `firmware/Core/Src/*.c`, so duplicate symbols are the main hazard.

1. Archive first: copy `firmware/Core/Src/{key_switch.c,usb_keyboard.c,main.c}` and `firmware/Core/Inc/keyboard.h` to `archive/verified-8k-single-key/`.
2. Add `key_matrix.c` and `matrix_hw.c` to `firmware/Core/Src/`, and `key_matrix.h` and `matrix_config.h` to `firmware/Core/Inc/`.
3. **Remove `key_switch.c` from `firmware/Core/Src/`** (it defines the same `Keyboard_Task` and counters).
4. Replace `usb_keyboard.c`, `main.c`, `keyboard.h` with the files in `integration/proposed/` (or apply `integration/patches/*.diff`).
5. Delete `build/` (stale CMake cache after the folder move), then run `tools/build.ps1`.
6. Update `tools/test-host.ps1`/`tests/host` (they test `key_switch.c`) or retire them in favour of this folder's tests, and update `docs/STATUS.md`, `docs/HANDOFF.md`, `CHANGELOG.md`.

## 4. Rollback

Restore the archived four files, delete `key_matrix.c`, `matrix_hw.c`, `key_matrix.h`, `matrix_config.h`, rebuild. Or simply flash `archive/verified-usb-1k/build/keypad.hex` (1K) / rebuild the 8K sources.

## 5. Known limits and next work

* Encoder (CLK, DT, SW) not implemented; it needs a quadrature decoder and likely a second HID report (volume).
* Six-key limit (boot protocol). More simultaneous keys needs an N-key-rollover report descriptor.
* Keymap is compile-time; a runtime/VIA-style keymap is later work.
* The USB product string still reads "Keypad U5A5 - MX2" (change in `usb_keyboard.c` if wanted).
* TMR sensing is separate work (ADC + the CD74HC4067 mux already on the devboard).
