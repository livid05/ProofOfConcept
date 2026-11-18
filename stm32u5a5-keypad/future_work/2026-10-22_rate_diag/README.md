# 8K report-rate test (diagnostic image, NOT the keyboard)

Created 2026-10-22. Isolated: nothing here is part of the project build, and no
project file was changed. The normal firmware (`build\keypad.hex`) is untouched, and
**Build + Flash USB** restores the keyboard at any time.

## Why a separate device

Windows does not let a program read the raw reports of a real keyboard. So this
image presents itself as a vendor-defined HID device ("8K Rate Test", USB ID
0483:5711) that uses the **same USB core, endpoint (interrupt IN 0x81, 8-byte
packets), high-speed settings and `bInterval = 1`** as the keyboard. It sends a
numbered report every time the endpoint is free. A PC script then measures what
actually arrives.

Each 8-byte report = 4-byte counter + 4-byte device microsecond clock (little-endian).

## Run it

1. Put the board in DFU mode (hold BOOT, press and release RST, release BOOT; close CubeProgrammer).
2. Flash: `powershell -File "future_work\2026-10-22_rate_diag\tools\flash-diag-usb.ps1"` (from the project folder).
3. Press RST with BOOT released, unplug and replug USB. Wait for Windows to finish setting up "8K Rate Test".
4. Measure: `powershell -File "future_work\2026-10-22_rate_diag\tools\measure-rate.ps1" -Seconds 10`
5. Restore the keyboard: DFU mode again, then **Build + Flash USB**.

Use a USB port directly on the PC (no hub). Keep other heavy USB traffic off while measuring.

## Reading the result

| Line | Meaning |
|---|---|
| Device send rate | reports/s the device sent, from its own clock. ~8000 = every 125 us microframe used. |
| Received / Dropped by Windows | Windows buffers only a few reports per reader; drops here do not change the device rate. |
| Gap between reports (us) | ideal 125. 250 means one microframe skipped, 1000 means 1K polling. |
| VERDICT | `8K CONFIRMED` needs >= 7500/s and >= 90% of gaps within 100-150 us. |

If the verdict is 1K-class, check USBView for High Speed first (a Full Speed link
polls once per ms). Hub, cable, or a USB 2.0 port that falls back to full speed
are the usual causes. A `PARTIAL` result means the firmware sometimes misses a
microframe; the gap table shows how often.

## What is verified

| Check | Result |
|---|---|
| Streamer logic on PC (`tools\run-host-tests.ps1`): report layout, counter, busy retry, clock wrap | Pass |
| Measurement maths self-test (ideal 8K, 1K, mixed, Windows drops, 32-bit wrap) | Pass |
| ARM build with project flags (-Werror): 21,154 B flash | Pass |
| Image descriptors: vendor HID, 0483:5711, endpoint 0x81, 8-byte report, bInterval 1 | Pass |
| Reading a real device (HID open/read code path) and any actual rate | **NOT TESTED** (needs the board) |

## Limits

* It measures the USB report path (host polling plus the firmware re-arming the endpoint
  in time). It does not measure key-to-USB latency or contact bounce.
* A passing result applies to this same endpoint setup; the keyboard image uses it with
  different report contents but the same descriptor timing.

## Result (measured 2026-10-22, on Tyler's PC, direct port, 10 s run)

Flashed rate-test image (BIN from output\arm-build\rate-test.hex), measure-rate.ps1 -Seconds 10:

| Metric | Value |
|---|---|
| Device send rate (device clock) | 7999 reports/s |
| Received by the PC program | 79,995 reports in 10.00 s (7999/s) |
| Dropped by Windows | 0 |
| Gap between reports | min 117 us, median 125 us, 99th percentile 133 us, max 1125 us |
| Gaps near 125 us | 100.0% (to 0.1%); worst gap 1125 us; under 0.05% of gaps were off the 125/250/375 us grid |
| Verdict | **8K CONFIRMED** |

Scope: this proves the USB path (host polls every 125 us microframe and the firmware re-arms the endpoint in time) for the keyboard's endpoint settings. It is not key-to-USB latency, and the keyboard image itself still only sends a report when a key changes.
