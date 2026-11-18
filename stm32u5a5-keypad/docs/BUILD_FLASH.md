# Build and flash

## Build in VS Code

Open the workspace file. Press Ctrl+Shift+B. Tool scripts discover GCC, CMake, Ninja and OpenOCD inside the existing STM32CubeIDE installation. No new compiler, extension, Python, or network download is needed to build. C/C++ IntelliSense is an optional recommended extension.

Or in the project terminal:

```powershell
.\tools\build.ps1
```

If CubeIDE moves, pass `-CubeIdeRoot 'C:\path\to\STM32CubeIDE'` to the scripts. They currently default to the installed `C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE`.

Outputs: `build/keypad.elf` (debugger + flash), `build/keypad.hex` (addressed flash image), `build/keypad.bin` (raw image at 0x08000000). CMake cache paths are local; do not share or commit `build/`. All source dependencies are vendored.

## Flash with the proven ST-Link/OpenOCD route

Power the target and use the working SWD connection. Close/terminate CubeIDE debugging and CubeProgrammer connections. Run VS Code task **Flash keypad (ST-Link)**, or:

```powershell
.\tools\flash.ps1
```

The VS Code task builds first. Calling the script directly uses the existing ELF. OpenOCD uses SWD at 100 kHz and software reset, programs flash, reads it back to verify, then resets/runs. No mass erase, option-byte edits, or probe firmware updates are requested.

The clone probe previously worked with OpenOCD despite the official ST GDB server failing. Keep using this route. The supplied optional VS Code debug launch uses Cortex-Debug (`marus25.cortex-debug`); install it only if you want F5 debugging. Its tool paths match this PC. Debug halts at main: press Continue to start USB enumeration.

## USB buttons in the VS Code bottom bar

The installed Task Buttons extension provides **Build**, **Flash USB**, and **Build + Flash USB** for this workspace. If they do not appear, run **Developer: Reload Window** from the command palette.

- **Build** compiles only; no board required.
- **Flash USB** programs/verifies the existing `build/keypad.hex` without rebuilding.
- **Build + Flash USB** builds first, then programs/verifies that image. A failed build stops the sequence.

Before flashing: connect devboard USB-C with a data cable, hold BOOT, press/release RST, release BOOT. Close or disconnect CubeProgrammer GUI so the CLI can access the board. After success, leave BOOT released and press RST. These physical steps cannot be performed by a VS Code task.

`tools/flash-usb.ps1` locates the installed CubeProgrammer CLI, enumerates USB DFU devices, selects a single board, programs the HEX and verifies it. No ST-Link is used. If no DFU device is found, enter BOOT/RST mode and retry. If several boards are found, unplug the others or select a port with `-Port USB1`.

Terminal equivalents:

```powershell
.\tools\build.ps1
.\tools\flash-usb.ps1
```

Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\flash-usb.ps1 -CheckOnly` to check CLI/image paths without accessing hardware. Alternatively use Terminal → Run Task → **Build + Flash keypad (USB DFU)**. ST-Link tasks remain optional alternatives.

## Flash using the already tested USB ROM DFU route

1. Hold BOOT, press/release RST, then release BOOT.
2. In STM32CubeProgrammer select **USB**, refresh and connect to the DFU device.
3. Open `build/keypad.hex`, enable verify, and download/program it. HEX carries its flash address. If using `keypad.bin`, set start address **0x08000000**.
4. Release BOOT and press RST to run. Reconnect USB if necessary; the device should enumerate as a keyboard instead of DFU.

## Bench check / troubleshooting

Use a USB **data** cable in the devboard USB-C port. Open Notepad, release MX2, then tap it. Expect one `a` per short tap on a US layout with Caps Lock off. Holding it may repeat, which is normal host behavior. Release must stop repeat.

- Slow LED blink (500 ms on/off): USB configured, idle.
- Fast LED blink (125 ms on/off): USB not configured, idle.
- Solid LED: switch debounced pressed, even if USB is not configured.
- Rapid 100 ms on/off from startup: clock or USB initialization failed; inspect `Error_Handler` and HSE/PHY setup.

If LED responds but no typing: check USB enumeration, data cable, Notepad focus, correct binary and release-after-connect. In debugger inspect `usb_device.dev_state`, `key_pressed`, `key_press_count`, `usb_report_count`, `keyboard_busy`, and `key_queue_overflow`. Merely having an LED response does not prove HID is working.

If Windows reports an unknown USB device, capture its error and the LED behavior before changing clocks or hardware. If ST-Link flash fails, terminate other debug sessions first. The project has not been flashed automatically.
