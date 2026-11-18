# Keypad U5A5 — 4 × 4 USB keyboard matrix (8K)

Open `Keypad-U5A5.code-workspace` in VS Code.

**Target:** STM32U5A5RJT6, Devboard RevK + Numpad RevE. **Test:** all 15 keys of the 4 × 4 matrix type `a b c d / e f g h / i j k l / m n o` over USB (the encoder position is empty). TMR and the encoder are not implemented. The earlier one-key 8K firmware is archived in `archive/verified-8k-single-key/`.

## First test

1. Wire (devboard pin names): ROW_1 → PC0, ROW_2 → PC1, ROW_3 → PC2, ROW_4 → PC3, COL_1 → PB4, COL_2 → PB5, COL_3 → PB6, COL_4 → PB7, GND → GND, 3.3V → 3V3. Do not add numpad 5V, LED, TMR or encoder wiring.
2. VS Code → Terminal → Run Build Task (`Ctrl+Shift+B`). The entire project builds; you do not build an individual C file.
3. Connect devboard USB-C, hold BOOT, press/release RST, then release BOOT. Close/disconnect CubeProgrammer. Click **Build + Flash USB** in the bottom bar. After success, leave BOOT released and press RST.
4. Click inside Notepad and press each key. Expected, row by row: **a b c d / e f g h / i j k l / m n o**. A held key repeats according to Windows settings; release stops it. Try two or three keys at once.

**Current build targets 8K polling and an 8K full-matrix scan. It builds and passes host logic/image checks; hardware testing of the matrix and of 8K enumeration is pending. Only the earlier single-key 1K build was confirmed on hardware (typing, High Speed).**

## Where things live

| Folder/file | Purpose |
|---|---|
| `firmware/Core/Inc/board_config.h` | USB interval, scan period, debounce time and LED pin |
| `firmware/Core/Inc/matrix_config.h` | Matrix pins, keymap (HID usages), row-settle time |
| `firmware/Core/Src/main.c` | Clock and board initialization |
| `firmware/Core/Src/key_matrix.c` | Per-key debounce, report building, queue (`Keyboard_Task`) |
| `firmware/Core/Src/matrix_hw.c` | Row/column GPIO scan (only file that touches the matrix pins) |
| `firmware/Core/Src/usb_keyboard.c` | Keyboard reports, descriptors and HID requests |
| `firmware/Core/Src/usb_port.c` | STM32 USB PHY and HAL glue |
| `firmware/Drivers`, `firmware/USB` | Vendored ST dependencies; no download needed to build |
| `tools/`, `.vscode/` | Build/flash tasks and optional debugger configuration |
| `build/` | Generated `keypad.elf`, `keypad.bin`, `keypad.hex`, map and compile commands |
| `docs/` | Wiring, architecture, test status and handoff notes |
| `archive/verified-bringup/` | Prior LED/switch test header and original CubeMX configuration |

Detailed comments explain the custom firmware. Start with `board_config.h`, then `key_switch.c`.
Read `docs/BUILD_FLASH.md` for DFU flashing or troubleshooting.

The active firmware uses CMake. The archived `.ioc` describes the old bring-up project; **do not regenerate it over this application**. The original project under `11 Capstone STM32U595` remains available separately.

## 8K settings and check

`board_config.h` contains USB_HS_BINTERVAL=1, KEY_SCAN_PERIOD_US=125 and KEY_EAGER_PRESS=1. Press is accepted on first closure; release needs 10 ms continuously open. Set eager mode to 0 for conservative symmetric filtering if noise causes false presses.

After flashing, unplug/reconnect USB and check USBView: **Device Bus Speed: High**, endpoint **bInterval: 0x01**. This verifies the advertised interval, not measured end-to-end latency. Test taps, hold/release and reconnect again. Known working 1K image: archive/verified-usb-1k/build/keypad.hex.

Optional logic test: tools/test-host.ps1. Timing details: docs/ARCHITECTURE.md. TIM2 is now reserved for the microsecond scan clock.
