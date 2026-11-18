# Optional host logic tests

Run `powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-host.ps1` from the project. Uses the MSVC compiler already installed on this PC (not required for the normal firmware build).

The actual firmware/Core/Src/key_switch.c is compiled as Windows C against fake time, GPIO and USB functions. There is no second implementation of the algorithm. Tests cover eager press, contact bounce, deferred release, preserving press/release order when USB is busy, suppressing a key held during reconnect, overflow recovery, timer rollover, and 8000 samples in a nominal simulated second.

These tests do not validate TIM2 hardware, real switch noise, USB link timing or host input latency. Assertion failure numbers are in harness.c. Generated test binaries stay under build/host-tests/.
