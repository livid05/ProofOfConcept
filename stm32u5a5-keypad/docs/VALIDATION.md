# Validation — 8K firmware, 2026-10-20

## Passed in software

- Arm GNU GCC 14.3.1 compilation/link, -Wall -Wextra -Werror, no warnings or errors.
- Flash image: 21,640 bytes. Linker RAM usage: 5,120 bytes including reserved heap/stack area.
- BIN SHA-256: `872dcef3cd7b29430e2c811d7f40221ba4ab534d050287752d4289b616044ba3`.
- Native MSVC host tests compile the actual key_switch.c with simulated GPIO/time/USB: eager press, bounce suppression, stable release, USB-busy event order, reconnect-held suppression, queue overflow recovery, uint32 counter rollover, and 8000 scans in a nominal simulated second.
- Linked ARM ELF/vector table and HID descriptors validated; endpoint interval initialization=1, 8-byte IN reports and one-byte LED output report. Workspace/task JSON parses.

Repeat: tools/build.ps1, tools/test-host.ps1, and python tools/verify_image.py. Normal build needs no Python or MSVC; those are optional validation tools.

## Hardware evidence and remaining tests

Tyler confirmed typing A and stopping repeat on release with the previous 1K image. USBView confirmed High Speed and bInterval=4 for that previous build. It is saved under archive/verified-usb-1k.

The new 8K image has not been flashed or bench-tested by the agent. Confirm USBView High Speed and bInterval=0x01 after flashing/reset/replug. Retest taps, hold/release and reconnect. Eager debounce must be checked for false presses on real hardware. Actual scan jitter, USB timing and end-to-end latency have not been measured. An advertised 125 us interval is not a 125 us latency guarantee.
