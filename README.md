# Keypad to Keyboard: from an RP2040 proof of concept to a TMR analog keyboard on STM32U5A5

This repository documents one project from start to finish: a wireless macro pad that started as QMK firmware on an RP2040, and became a capstone effort to build a fast, reliable, low-power keyboard with TMR (tunnel magnetoresistance) analog keys on an STM32U5A5, with 8 kHz USB reporting as a goal.

It holds the code, the KiCad hardware for every board revision, the bench results, and the reasoning behind each decision, written so someone new could follow the whole path and so the author can pick it up again after a break.

## Where things stand (2026-11-18)

| Area | State | How it is known |
|---|---|---|
| RP2040 QMK keypad with BLE and 2.4 GHz bridges | Built and working | Original proof of concept, see the RP2040 write-up |
| STM32U5A5 USB HID firmware, 4x4 matrix | Built, bench-tested | Typed on hardware; column 3/4 fix still needs a recheck |
| USB report rate | 7999 reports per second measured | Rate-test image on the author's PC, 0 dropped in 10 s |
| TMR numpad (15 sensors, mux, ADC) | 14 of 15 keys pass the self-test | Observed on the bench; key K4 is open and parked |
| Blocking TMR scan | 730 us for 16 inputs | Measured; too slow for an 8 kHz scan |
| Fast pipelined scan and rapid trigger image | Written and PC-tested only | Not built for the board or run on hardware yet. Results pending |
| Full tri-mode keyboard (TriMode60) | Board designed through Rev E, not built or tested | Design only |

Anything marked pending has not been observed on hardware. The docs keep that distinction on purpose.

## Read in this order

1. [Project story](docs/01-project-story.md): the timeline from the RP2040 pad to today.
2. [Hardware evolution](docs/02-hardware-evolution.md): every dev board, numpad and keyboard revision, what changed and why, with images.
3. [TMR sensing guide](docs/03-tmr-sensing-guide.md): how the sensors, mux and ADC work together, and how this board is wired.
4. [Bench bring-up, 2026-11-18](docs/04-bench-bringup-2026-11-18.md): what was measured, what went wrong, and how each problem was found.
5. [Build, flash and test guide](docs/05-build-flash-test-guide.md): how to build the images, flash them and run the console.
6. [Scanning at 8 kHz and market research](docs/06-8k-scanning-and-market-research.md): the time budget, the tradeoffs, and what shipping TMR keyboards do.
7. [Roadmap](docs/07-roadmap.md): what is next, in order.
8. [Lessons learned](docs/08-lessons-learned.md): mistakes worth not repeating.
9. [KiCad guide](docs/09-kicad-guide.md): how to open the projects, net classes, stackups, pinouts, bills of materials and ordering notes.
10. [Presentations and reports](docs/10-presentations-and-reports.md): what each deck and document in `reports/` is for.

The original proof-of-concept README is kept as [docs/rp2040-proof-of-concept.md](docs/rp2040-proof-of-concept.md).

## Repository map

| Path | Contents |
|---|---|
| `docs/` | The guides above, plus `docs/images/` |
| `hardware/` | KiCad projects, production files and revision notes for the dev board, numpad and TriMode60. See [hardware/README.md](hardware/README.md) |
| `stm32u5a5-keypad/` | STM32U5A5 firmware, host tests, build tools and the `future_work` test images. Its own `docs/STATUS.md` and `docs/HANDOFF.md` are the day-to-day status files |
| `reports/` | Presentations, the Rev H overview, instructor feedback and the capstone deck source. All rights reserved |
| `cubemx/` | STM32CubeMX `.ioc` configuration files |
| `rp2040-reference/` | Raspberry Pi GPIO scripts, Numpad TMR1 reference archives and the VIA configuration from the RP2040 stage |
| `binaries/` | Prebuilt `.bin` and `.hex` images of the verified firmware steps and diagnostics |
| `schematics/` | Earlier KiCad work from the RP2040 stage (capstone design, matrix and mux, numpad TMR1) |
| repo root (`keyboard.json`, `proofofconcept.c`, `mux_analog.c` and others) | The RP2040 QMK keyboard definition |
| `ble_bridge/`, `esb_bridge/`, `esb_dongle/` | nRF52840 wireless bridge and dongle firmware |

## Licensing

Short version, in plain words; the exact terms are in [`LICENSE`](LICENSE).

| Part | Terms |
|---|---|
| My firmware, tools, tests and bridge code | MIT |
| Hardware designs (KiCad projects, libraries, production files) | MIT |
| Documentation, guides and images | CC BY 4.0 |
| Reports, presentations and coursework in `reports/` | All rights reserved |
| ST HAL, CMSIS, USB core | Their own ST and Arm licenses, kept in the same folders |
| QMK-based RP2040 files at the top level | GPL-2.0-or-later |

The USB ID 0483:5710 used for bench work is ST's demonstration ID and must be replaced before any product is shared or sold. See `stm32u5a5-keypad/docs/THIRD_PARTY.md` for where each third-party part came from.
