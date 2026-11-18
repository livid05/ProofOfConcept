# TMR sensor test (diagnostic image, NOT the keyboard)

Created 2026-11-18. Same pattern as `2026-10-22_rate_diag`: an isolated
test image built from the project's own `main.c`, clock setup, `usb_port.c` (the HS PHY
setup that works on this board) and ST USB core. Nothing in `firmware/` was changed, and
**Build + Flash USB** puts the keyboard back at any time.

The board shows up as a USB COM port ("USB Serial Device", USB ID 0483:5712) and prints a
self-test of every TMR sensor, then every key press and release.

## Wiring (as on the bench, 2026-11-18)

- Numpad J1 TMR pins to devboard J8 (the MUX IN header). Your order is recorded in
  `firmware/Core/Src/tmr_config.h` (`TMR_KEY_OF_CHANNEL`): J1 TMR_OUT1, 5, 9, 13, 2, 6, 10,
  14, 15, 11, 7, 3, 12, 8, 4 go to J8 holes 16, 14, 12, 10, 7, 5, 3, 1, then 15, 13, 11,
  9, 8, 6, 4. J8 hole 2 is empty and should test OPEN.
- Numpad J1 **+3.3V** (pin 25) to devboard 3V3, and numpad GND to devboard GND. Check with
  a meter that the numpad's +3.3V pin reads 3.3 V. It must **not** be on +5V: the sensors
  would then swing above the mux and ADC supply.
- No sensor wire may sit on a J8 **G** hole. Those holes are ground.
- The matrix row/column wires can stay connected; this image ignores them.

## Run it (VS Code bottom bar)

Open `Keypad-U5A5.code-workspace` (as usual). The TMR tasks and the two new buttons live in
that workspace file; `.vscode\` was not changed. If the buttons do not show, run
**Developer: Reload Window**. The original config files are copied in `vscode_backup\` as .bak files (do not open them).

1. Hold BOOT, press and release RST, release BOOT (DFU mode, same as the keyboard).
2. Click **TMR Build + Flash**. It builds into `output\arm-build\` and flashes over USB DFU.
3. Press RST with BOOT released.
4. Click **TMR Console**. The self-test report prints as soon as the port opens.
   Keep your hands off the keys during start-up and self-tests.
5. Press each key fully, one at a time. Each press prints `DOWN` and `up` lines.
   Type `s` for the summary.

Terminal equivalents (from the project folder):
`future_work\2026-11-18_tmr_diag\tools\build-tmr-diag.ps1`, then `flash-tmr-diag-usb.ps1`,
then `tmr-console.ps1`. Any serial terminal also works (any baud rate).

Restore the keyboard: DFU mode again, then **Build + Flash USB**.

## Console keys

| Key | Does |
|---|---|
| `t` or Enter (or SW3 button) | Self-test again (hands off) |
| `s` | Summary: presses, peak swing each way, PASS / weak swing / press it / FAULT |
| `l` | Live line every 250 ms: millivolts from rest for each J8 hole (`*` = down) |
| `r` | Raw voltage of all 16 holes |
| `z` | Re-zero rest points (hands off) |
| `c` | Clear press counts and peaks |
| `m` | Learn the wiring order: press K1..K15 in reading order, then paste the printed line into `tmr_config.h` |

LED D1: slow blink = USB connected, fast blink = no USB, solid = a key is down,
100 ms blink from power-up = ADC or clock setup failed.

## Reading the self-test

| Status | Meaning | Usual fix |
|---|---|---|
| `ok` | Sensor drives the input and rests near half the supply (about 1.65 V) | — |
| `OFF-CTR` | Works, but rests outside 25-75 % of supply | Magnet very close at rest, or a weak sensor |
| `OPEN` | Nothing drives the input: it follows PC4's pull-up and pull-down | Jumper missing, wrong J8 hole, sensor pin 1 not soldered |
| `NO POWER?` | Pull-down reaches 0 V but pull-up gets stuck partway, as an unpowered output does | Numpad +3.3V wire, sensor pin 2 joint |
| `WEAK` | Connected but barely driven | Cold or cracked joint, loose jumper |
| `STUCK 0V` | Held at 0 V | No 3.3 V, output shorted to GND, sensor rotated |
| `STUCK 3V3` | Held at 3.3 V | Output shorted to 3.3 V, or a magnet sitting on the sensor |

How the open check works: for each input, PC4 is briefly switched to a digital input with
its ~40 k pull-up, then its pull-down, and read straight away. A floating input (4.7 nF
filter capacitor, nothing else) charges to the pull's rail. A real TMR2615 output barely
moves. Noise is the peak-to-peak and RMS of 256 readings at rest.

In the event lines, "others moved <= N mV" is how far the released sensors moved in the
same scan. A few mV is normal. Tens of mV point to crosstalk, a shared wire, or a magnet
reaching the next sensor.

## Settings

All of them are in `firmware/Core/Src/tmr_config.h`: press/release thresholds (150/100 mV,
either direction), PASS swing (300 mV), mux settle (10 us), samples per read (4), scan
period (1 ms), self-test limits, wiring order, USB ID.

## What is verified

| Check | Result |
|---|---|
| ARM build with the project's flags (-Wall -Wextra -Werror), GCC 13.2 on a Linux build host against identical HAL v1.6.2 / CMSIS v1.4.2 sources | Pass: 39.9 KB flash, 16.4 KB RAM |
| Same sandbox builds the untouched keyboard image cleanly (toolchain sanity check) | Pass |
| Linked image descriptors: IAD + CDC-ACM, 0483:5712, notify 0x81, bulk 0x02/0x82 at 512 B (HS) / 64 B (FS) | Pass |
| PC tests `tests\host\harness_tmr.c` (classification, press/release, both polarities, drift, slow press, statistics), plus deliberately broken versions being caught | Pass (gcc). `tools\run-host-tests.ps1` runs them with MSVC like the rate test |
| Built with CubeIDE's GCC 14.3 on Tyler's PC | **Pending** (TMR Build + Flash does it) |
| Anything on the real board: enumeration, ADC readings, sensor results | **NOT TESTED** |

`output\arm-build\tmr-test.hex` ships pre-built from the sandbox. The build task replaces it.

## Limits

- This is a bench test: one scan per 1 ms with 10 us settling, not the keyboard's 8K timing.
- The TMR2615 senses in the chip's plane (sideways). A sensor reading `ok` that hardly moves
  on a press is a magnet/geometry issue, not a solder fault.
- USB descriptors use the ST demo VID like the other test images. Prototype only.
