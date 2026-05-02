# ProofOfConcept — Wireless QMK Keypad

A custom 4×4 keypad with rotary encoder and per-key RGB, built on an RP2040,
that transmits keystrokes wirelessly to a PC via either BLE (Bluetooth Low Energy)
or a 2.4 GHz proprietary radio link using a Nordic nRF52840 bridge board.

**Hardware path:**
`RP2040 keypad → UART (GP0) → TENSTAR nRF52840 → BLE or 2.4 GHz → PC`

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Hardware](#hardware)
3. [Repository Structure](#repository-structure)
4. [System Architecture](#system-architecture)
5. [Wireless Protocol](#wireless-protocol)
6. [QMK Firmware (RP2040)](#qmk-firmware-rp2040)
7. [BLE Bridge (ble_bridge)](#ble-bridge-ble_bridge)
8. [2.4 GHz ESB Bridge + Dongle (esb_bridge / esb_dongle)](#24-ghz-esb-bridge--dongle)
9. [Keymap & Layers](#keymap--layers)
10. [RGB Matrix](#rgb-matrix)
11. [Rotary Encoder](#rotary-encoder)
12. [HC4067 Analog MUX Expansion (Future)](#hc4067-analog-mux-expansion-future)
13. [Latency Testing](#latency-testing)
14. [Pin Reference](#pin-reference)
15. [Build & Flash Instructions](#build--flash-instructions)
16. [Schematics](#schematics)
17. [Errors & Troubleshooting Log](#errors--troubleshooting-log)

---

## Project Overview

The goal was to build a proof-of-concept wireless macro pad that:

- Runs full **QMK firmware** on an RP2040 microcontroller (handling the key matrix,
  encoder, RGB, VIA support, and layer logic)
- Offloads wireless communication to a second MCU — a **TENSTAR Pro Micro nRF52840**
  — connected via UART
- Supports two wireless modes: **BLE HID** (Bluetooth) and **2.4 GHz proprietary
  radio** (lower latency, uses a USB dongle)
- Has **per-key RGB** via WS2812 LEDs (15 LEDs total)
- Has a **rotary encoder** with a push button
- Has a **VIA-compatible** keymap for live remapping without reflashing

---

## Hardware

| Part | Role |
|------|------|
| RP2040 dev board (e.g. Raspberry Pi Pico) | Main keyboard MCU — runs QMK |
| TENSTAR Pro Micro nRF52840 | Wireless bridge — runs BLE or ESB firmware |
| Adafruit nRF52840 USB Key dongle | 2.4 GHz USB HID receiver (ESB mode only) |
| 4×4 key switch matrix | 16 keys, COL2ROW diodes |
| Rotary encoder (with push button) | Volume / navigation |
| WS2812 addressable RGB LEDs (×15) | Per-key lighting |
| HC4067 16-channel analog MUX | Reserved for analog/hall-effect key expansion |

**Arduino board definitions used:**
- TENSTAR bridge: **Adafruit Feather nRF52840 Express** board definition
- USB dongle: **Raytac nRF52840 Dongle** (or Adafruit nRF52840 USB Key)

---

## Repository Structure

```
proofofconcept/
├── keyboard.json          # QMK keyboard definition (layout, matrix pins, RGB, USB)
├── config.h               # Pin definitions, UART config, encoder pins, RGB defaults
├── mcuconf.h              # ChibiOS override: enables RP2040 UART0 SIO driver
├── rules.mk               # QMK build rules: encoder, UART, (MUX disabled by default)
├── proofofconcept.c       # Keyboard-level C: UART init, key→packet TX, encoder button
├── pio_uart.c/.h          # Thin wrapper: delegates to QMK's uart_sio driver
├── mux_analog.c/.h        # HC4067 custom matrix driver (disabled, ready to enable)
├── proofofconcept-via.json # VIA layout definition for live remapping
├── latency_test.py        # Python script to measure BLE/radio connection latency
├── keymaps/
│   └── default/
│       └── keymap.c       # 3-layer keymap + RGB indicators
├── ble_bridge/
│   ├── ble_bridge.ino     # TENSTAR Arduino sketch: UART → BLE HID
│   ├── ble_bridge.uf2     # Pre-compiled BLE bridge firmware
│   ├── uf2conv.py         # UF2 conversion utility
│   └── uf2families.json   # UF2 chip family definitions
├── esb_bridge/
│   ├── esb_bridge.ino     # TENSTAR Arduino sketch: UART → 2.4 GHz radio TX
│   ├── esb_bridge.uf2     # Pre-compiled ESB bridge firmware
│   ├── radio.cpp/.h       # nRF52840 bare-metal radio driver (TX side)
├── esb_dongle/
│   ├── esb_dongle.ino     # USB dongle Arduino sketch: radio RX → USB HID
│   ├── esb_dongle.uf2     # Pre-compiled ESB dongle firmware
│   └── radio.cpp/.h       # nRF52840 bare-metal radio driver (RX side)
└── .gitignore             # Excludes .claude/ (Claude Code config)
```

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      RP2040 (QMK)                           │
│                                                             │
│  Key matrix (4×4) ──→ process_record_kb()                  │
│  Encoder A/B (GP14/15) ──→ encoder_map[]                   │
│  Encoder button (GP12) ──→ matrix_scan_kb()                 │
│                              │                              │
│                    Build 4-byte packet                      │
│                    [0xAA][type][b1][b2]                     │
│                              │                              │
│                    UART0 TX (GP0) @ 1 Mbaud                 │
└─────────────────────────────┬───────────────────────────────┘
                              │ wire: GP0 → D5 (P0.24)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              TENSTAR nRF52840 (Bridge)                      │
│                                                             │
│  Serial1.begin(1000000) ← receives packets                  │
│                                                             │
│  ── BLE mode (ble_bridge.ino) ──────────────────────────   │
│     Bluefruit HID → BLE HID keyboard to PC                 │
│                                                             │
│  ── ESB mode (esb_bridge.ino) ──────────────────────────   │
│     Bare-metal NRF_RADIO → 2.4 GHz 2 Mbps                  │
│     Sends each packet 3× via TXIDLE (no re-ramp)           │
└─────────────────────────────────────────────────────────────┘
                              │  (ESB mode only)
                              │  2.4 GHz @ 2480 MHz
                              ▼
┌─────────────────────────────────────────────────────────────┐
│           Adafruit nRF52840 USB Dongle                      │
│                                                             │
│  NRF_RADIO RX → dedup filter → USB HID (TinyUSB)           │
│  Reports: keyboard (ID 1) + consumer (ID 2)                 │
│  Stuck-key safety: releases all keys after 500ms silence    │
└─────────────────────────────────────────────────────────────┘
                              │ USB
                              ▼
                             PC
```

---

## Wireless Protocol

Every key event is encoded as a **4-byte packet**:

```
[0xAA] [type] [byte1] [byte2]
```

| type | Meaning | byte1 | byte2 |
|------|---------|-------|-------|
| `0x01` | Key down | HID keycode | `0x00` |
| `0x00` | Key up | HID keycode | `0x00` |
| `0x03` | Consumer key down | usage_low | usage_high |
| `0x04` | Consumer key up | `0x00` | `0x00` |

- `0xAA` is the sync/start byte — the bridge re-syncs on every packet boundary
- Consumer keycodes (volume, media, brightness) are mapped to HID Consumer Control
  usage codes inside `proofofconcept.c` before transmission
- Layer keys, RGB keys, `QK_BOOT`, `EE_CLR` are **not** transmitted — they are
  handled locally by QMK and never generate USB HID output themselves

In **ESB mode**, the packet is stripped of the start byte before radio transmission
(the 3-byte payload is `[type][b1][b2]`), and the same packet is sent 3 times via
the TXIDLE trick to avoid paying the ~130 µs radio ramp-up cost each time.

---

## QMK Firmware (RP2040)

### Files: `keyboard.json`, `config.h`, `mcuconf.h`, `rules.mk`, `proofofconcept.c`

**Key points:**

- Processor: `RP2040` | Bootloader: `rp2040` (UF2 drag-and-drop)
- Matrix: 4 rows × 4 cols, `COL2ROW` diode direction
  - Rows: `GP2, GP3, GP4, GP5`
  - Cols: `GP9, GP8, GP7, GP6`
- Encoder: A=`GP15`, B=`GP14`, resolution=4, push button=`GP12` (direct GPIO, not in matrix)
- RGB: WS2812 on `GP17`, vendor driver, 15 LEDs

**UART configuration:**

The RP2040 uses its hardware UART0 peripheral via QMK's ChibiOS SIO driver:

- `config.h`: sets `UART_DRIVER SIOD0`, `UART_TX_PIN GP0`, `UART_TX_PAL_MODE 2`
- `mcuconf.h`: overrides the default with `RP_SIO_USE_UART0 TRUE`
- `rules.mk`: `UART_DRIVER_REQUIRED = yes`
- Baud rate: **1,000,000 bps (1 Mbaud)**
  - A 4-byte packet takes ~40 µs at 1 Mbaud
  - Previous baud of 57,600 took ~694 µs per packet (17× slower)

**Key event handling (`proofofconcept.c`):**

- `keyboard_pre_init_kb()`: sets encoder button pin high-impedance, calls `pio_uart_init()`
- `matrix_scan_kb()`: polls the encoder push button pin directly (it's not in the matrix)
  and sends a mute consumer packet wirelessly when state changes
- `process_record_kb()`: intercepts every key event:
  - Regular keycodes (`< 0x0100`): sends key down/up packet via UART, suppresses USB HID
  - Consumer keycodes: maps to HID usage code, sends packet, suppresses USB HID
  - QMK-internal keys (layer switches, RGB, bootloader, etc.): passed through to normal
    QMK processing — these never output USB HID by themselves so wireless suppression
    is not needed

### `pio_uart.c` / `pio_uart.h`

Despite the "PIO" name (a remnant of an earlier implementation), this module now
delegates entirely to QMK's `uart_sio` driver (`uart_init()` / `uart_write()`).
The `gpio_pin` parameter to `pio_uart_init()` is ignored — pin assignment happens
through `config.h`. The file was renamed from a prior PIO-based approach and kept
for API compatibility.

---

## BLE Bridge (`ble_bridge`)

**Target board:** TENSTAR Pro Micro nRF52840 — use the
**Adafruit Feather nRF52840 Express** board definition in Arduino IDE.

**Firmware:** `ble_bridge.ino`

- Initializes `Serial1` at **1,000,000 baud** on D5 (P0.24) to receive RP2040 packets
- Initializes `BLEHidAdafruit` (keyboard + consumer reports)
- Advertises as `"ProofOfConcept"`, requests fastest BLE connection interval (6–12 units
  = 7.5–15 ms, the HID minimum per spec)
- Parses the 4-byte packet stream with a simple sync-byte state machine
- Maintains a 6-key rollover keyboard report (`modifier` + `keys[6]`) and sends it on
  every key event when BLE is connected
- Consumer keycodes are forwarded as BLE Consumer Control reports

**Pre-compiled firmware:** `ble_bridge.uf2` — drag and drop onto the TENSTAR's UF2
bootloader drive.

---

## 2.4 GHz ESB Bridge + Dongle

### Bridge — `esb_bridge/esb_bridge.ino`

**Target board:** TENSTAR Pro Micro nRF52840 (same as BLE bridge, different firmware)

- Receives the same 4-byte UART packets from the RP2040
- Strips the `0xAA` start byte and passes the 3-byte payload to `radio_send_n()`
- Sends each packet **3 times** using the TXIDLE trick (radio stays energized between
  sends — ~130 µs ramp-up paid once, subsequent packets cost ~174 µs each)
  - Total for 3 copies: ~652 µs vs. ~900 µs if ramp-up were paid each time

### Radio driver — `esb_bridge/radio.cpp` + `radio.h`

Bare-metal Nordic Semiconductor peripheral programming (no SoftDevice, no ESB library):

- Mode: Nordic proprietary 2 Mbps
- Channel: 80 (2480 MHz — chosen to avoid 2.4 GHz WiFi channels 1–11 which run 2401–2483 MHz)
- Address: 4-byte base `0xDEADBEEF` + 1-byte prefix `0xAB`
- Payload: **fixed 3 bytes** (no length field)
- CRC: 2-byte CCITT-16, polynomial `0x11021`, init `0xFFFF`
- TX shortcut: `READY→START` (radio fires immediately after ramp-up)

### Dongle — `esb_dongle/esb_dongle.ino`

**Target board:** Adafruit nRF52840 USB Key — use **Raytac nRF52840 Dongle** board definition

- Enumerates as a **USB HID device** with two report descriptors:
  - Report ID 1: standard keyboard (6KRO)
  - Report ID 2: consumer control
- Receives radio packets and dispatches to `key_down()` / `key_up()` / `consumer_pulse()`
- **Deduplication:** ignores identical packets arriving within 50 ms of each other
  (guards against the 3× retransmit appearing as separate events)
- **Stuck-key safety net:** if any key has been held for more than 500 ms with no new
  radio packets received, all keys are forcibly released (protects against a dropped
  key-up packet when the wireless link goes down)
- **Consumer key handling:** `consumer_pulse()` sends a press+release atomically,
  waiting up to 5 ms for USB HID to be ready between each report to avoid dropped
  releases when USB is busy

---

## Keymap & Layers

Three layers defined in `keymaps/default/keymap.c`:

### Layer 0 — BASE

```
┌──────┬──────┬──────┬──────┐
│  1   │  2   │  3   │SHIFT │
├──────┼──────┼──────┼──────┤
│  4   │  5   │  6   │ FN2  │  ← hold for function layer
├──────┼──────┼──────┼──────┤
│  7   │  8   │  9   │ RGB  │  ← hold for RGB layer
├──────┼──────┼──────┼──────┤
│  +   │  -   │  0   │ ENC  │  ← encoder button (wired via matrix scan)
└──────┴──────┴──────┴──────┘
Encoder rotate: Volume Down / Volume Up
```

### Layer 1 — FN2 (hold F2 / row 1 col 3)

```
┌──────┬──────┬──────┬──────┐
│ PREV │ PLAY │ NEXT │      │
├──────┼──────┼──────┼──────┤
│ CALC │NUMLK │ PSCR │      │
├──────┼──────┼──────┼──────┤
│BRDWN │ BRUP │ MUTE │      │
├──────┼──────┼──────┼──────┤
│ BOOT │EECLR │      │      │  ← BOOT and EECLR lit orange as warning
└──────┴──────┴──────┴──────┘
Encoder rotate: Previous Track / Next Track
```

### Layer 2 — RGB (hold F3 / row 2 col 3)

```
┌──────┬──────┬──────┬──────┐
│TOGGL │ NEXT │ PREV │      │
├──────┼──────┼──────┼──────┤
│HUE + │HUE - │SAT + │SAT - │
├──────┼──────┼──────┼──────┤
│VAL + │VAL - │SPD + │ F3   │  ← F3 key lights white (held)
├──────┼──────┼──────┼──────┤
│SPD - │      │      │      │
└──────┴──────┴──────┴──────┘
Encoder rotate: Brightness Down / Brightness Up
```

---

## RGB Matrix

Configured in `keyboard.json`:

- Driver: WS2812, pin GP17, vendor (Pico PIO) driver
- 15 LEDs (16 key positions, no LED on matrix position `[0,0]`)
- Default animation: `cycle_left_right` at brightness 150/200, speed 40
- Enabled animations: solid, breathing, cycle all, cycle left/right, cycle up/down,
  rainbow chevron, dual beacon, wave, solid reactive, solid splash, solid multisplash

**RGB layer indicators** (`keymap.c` — `rgb_matrix_indicators_advanced_user()`):

- All FN2 keys → cyan/blue when FN2 is held
- QK_BOOT and EE_CLR → orange (warning color) on FN2 layer
- All RGB keys → purple when RGB layer is held
- On base layer: F2 key glows dim blue, F3 key glows dim purple (passive hints)
- Indicator brightness scales with the current `RM_VALD`/`RM_VALU` setting by
  manually multiplying `r/g/b × rgb_matrix_get_val() / 255`

---

## Rotary Encoder

- Pins: A=`GP15`, B=`GP14`, resolution=4
- Push button: `GP12` — wired directly to GPIO, **not** in the key matrix
  - The button is polled in `matrix_scan_kb()` using `gpio_read_pin()`
  - State change sends a consumer mute packet wirelessly
  - The button is listed as `XXXXXXX` (no key) at matrix position `[3,3]` in the keymap
    since it is not routed through the matrix
- VIA layout shows the encoder position at `[3,3]` with `e0` encoding

---

## HC4067 Analog MUX Expansion (Future)

Files `mux_analog.c` / `mux_analog.h` implement a **QMK `CUSTOM_MATRIX = lite`**
driver for a Texas Instruments/NXP HC4067 16-channel analog multiplexer, intended
for hall-effect or analog key switches.

**Currently disabled** in `rules.mk` (lines are commented out):

```makefile
# CUSTOM_MATRIX = lite
# SRC += mux_analog.c
# ANALOG_DRIVER_REQUIRED = yes
# CONSOLE_ENABLE = yes
```

To enable: uncomment those lines, then edit `mux_channel_map[]` in `mux_analog.c`
to map each MUX channel index to its `{row, col}` position in the key matrix.

**Default MUX pins:**

| Signal | Default Pin | Override |
|--------|-------------|---------|
| S0 | GP22 | `#define MUX_S0_PIN` in config.h |
| S1 | GP26 | `#define MUX_S1_PIN` |
| S2 | GP27 | `#define MUX_S2_PIN` |
| S3 | GP28 | `#define MUX_S3_PIN` |
| SIG (ADC) | GP29 | `#define MUX_SIG_PIN` |

**How it works:**

1. At boot (`matrix_init_custom`), each channel is sampled 32 times and averaged to
   build a per-channel voltage baseline (keys must be released at power-on).
2. Each scan (`matrix_scan_custom`), channels whose reading drops ≥ 15% below
   baseline are treated as pressed.
3. Channels with a baseline below 100 ADC counts are skipped (assumed unconnected).
4. A 5 ms debounce window is applied per channel.
5. Detected presses are OR'd into the QMK matrix so normal matrix keys and MUX keys
   coexist.
6. Debug output via `uprintf` when `CONSOLE_ENABLE = yes`.

---

## Latency Testing

`latency_test.py` — Python script using `pynput` to measure keyboard round-trip latency.

**Usage:**

```bash
pip install pynput
python latency_test.py
```

Hold any key down. Windows fires repeated `keydown` events while a key is held; the
minimum interval between them reflects the BLE connection interval or radio polling rate.
Press Escape to stop and see the summary.

**Reference numbers:**

| Protocol | Target floor |
|----------|-------------|
| BLE (7.5 ms interval) | ~7.5 ms |
| 2.4 GHz proprietary (ESB) | ~0.125 ms (8K polls/sec) |

---

## Pin Reference

### RP2040

| Pin | Function |
|-----|----------|
| GP0 | UART0 TX → TENSTAR D5 (Serial1 RX) |
| GP1 | UART0 RX (unused, reserved) |
| GP2 | Matrix row 0 |
| GP3 | Matrix row 1 |
| GP4 | Matrix row 2 |
| GP5 | Matrix row 3 |
| GP6 | Matrix col 3 |
| GP7 | Matrix col 2 |
| GP8 | Matrix col 1 |
| GP9 | Matrix col 0 |
| GP12 | Encoder push button (active low, internal pull-up) |
| GP14 | Encoder B |
| GP15 | Encoder A |
| GP17 | WS2812 RGB data |
| GP22 | MUX S0 (future) |
| GP26 | MUX S1 (future) |
| GP27 | MUX S2 (future) |
| GP28 | MUX S3 (future) |
| GP29 | MUX ADC signal (future) |

### TENSTAR nRF52840 (Feather board definition)

| Pin | Function |
|-----|----------|
| D5 / P0.24 | Serial1 RX ← RP2040 GP0 |
| GND | Common ground with RP2040 |

---

## Build & Flash Instructions

### QMK firmware (RP2040)

This keyboard lives inside a QMK firmware tree at:

```
qmk_firmware/keyboards/proofofconcept/
```

**Build:**

```bash
qmk compile -kb proofofconcept -km default
```

**Flash:**

1. Hold BOOTSEL on the RP2040 board while plugging in USB (or press QK_BOOT from FN2 layer)
2. A drive named `RPI-RP2` appears
3. Drag the generated `.uf2` file onto the drive
4. Board reboots automatically

### BLE bridge firmware (TENSTAR)

1. Open `ble_bridge/ble_bridge.ino` in Arduino IDE
2. Board: **Adafruit Feather nRF52840 Express**
3. Upload via USB, or drag `ble_bridge.uf2` onto the TENSTAR's bootloader drive

### ESB bridge firmware (TENSTAR)

Same as BLE bridge but use `esb_bridge/esb_bridge.ino` (or `esb_bridge.uf2`).

### ESB dongle firmware

1. Open `esb_dongle/esb_dongle.ino` in Arduino IDE
2. Board: **Raytac nRF52840 Dongle** (or Adafruit nRF52840 USB Key)
3. Upload, or drag `esb_dongle.uf2` onto the dongle's bootloader drive

### VIA

Load `proofofconcept-via.json` in [VIA](https://usevia.app/) for live keymap editing
without reflashing.

---

## Schematics

All KiCad design files live in the [`schematics/`](schematics/) folder.
Open `.kicad_pro` files in KiCad 7+ to view or edit schematics and PCB layouts.
Gerber files are included for PCB fabrication.

### Capstone Keyboard Design — Main PCB

**Folder:** [`schematics/capstone_keyboard_design/`](schematics/capstone_keyboard_design/)

The primary PCB design for the keypad, integrating the key matrix, RGB LEDs,
encoder, and all MCU connections.

| File | Description |
|------|-------------|
| [`Capstone Keyboard Design.kicad_sch`](schematics/capstone_keyboard_design/Capstone%20Keyboard%20Design.kicad_sch) | Main schematic |
| [`Capstone Keyboard Design.kicad_pcb`](schematics/capstone_keyboard_design/Capstone%20Keyboard%20Design.kicad_pcb) | PCB layout |
| `Capstone Keyboard Design-*.gbr` | Gerber files (F_Cu, B_Cu, F/B_Mask, F/B_Silkscreen, Edge_Cuts, In1/In2_Cu, Paste) |
| [`production_gerbers.zip`](schematics/capstone_keyboard_design/production_gerbers.zip) | Production-ready Gerber archive |
| [`production_bom.csv`](schematics/capstone_keyboard_design/production_bom.csv) | Bill of materials |
| [`production_positions.csv`](schematics/capstone_keyboard_design/production_positions.csv) | Component placement file |

**PCB layers:** 4-layer board (F_Cu, In1_Cu, In2_Cu, B_Cu)

### Matrix + Multiplex — Key Matrix Subproject

**Folder:** [`schematics/matrix_multiplex/`](schematics/matrix_multiplex/)

Standalone KiCad project for the 4×4 switch matrix wiring and the HC4067
analog multiplexer expansion circuit.

| File | Description |
|------|-------------|
| [`Matrix + Multiplex.kicad_sch`](schematics/matrix_multiplex/Matrix%20+%20Multiplex.kicad_sch) | Matrix + MUX schematic |
| [`Matrix + Multiplex.kicad_pcb`](schematics/matrix_multiplex/Matrix%20+%20Multiplex.kicad_pcb) | PCB layout |

The matrix uses standard COL2ROW diode wiring:
- 4 rows (GP2–GP5) driven low one at a time
- 4 columns (GP9–GP6) read with internal pull-ups
- 1N4148 or equivalent diodes, cathode toward the row (COL2ROW)

The HC4067 MUX section shows wiring for the 16-channel analog expansion
(select pins S0–S3 on GP22/26/27/28, signal on GP29/ADC).

### Numpad_TMR1 — Production Numpad PCB

**Folder:** [`schematics/numpad_tmr1/`](schematics/numpad_tmr1/)

Refined production version of the numpad PCB with full fabrication outputs.

| File | Description |
|------|-------------|
| [`Numpad_TMR1.kicad_sch`](schematics/numpad_tmr1/Numpad_TMR1.kicad_sch) | Schematic |
| [`Numpad_TMR1.kicad_pcb`](schematics/numpad_tmr1/Numpad_TMR1.kicad_pcb) | PCB layout |
| [`production/Numpad_TMR1.zip`](schematics/numpad_tmr1/production/Numpad_TMR1.zip) | Production Gerber archive |
| [`production/bom.csv`](schematics/numpad_tmr1/production/bom.csv) | Bill of materials |
| [`production/positions.csv`](schematics/numpad_tmr1/production/positions.csv) | Component placement |

---

## Errors & Troubleshooting Log

This section documents issues encountered during development and their resolutions.

---

### 1. PIO UART vs. QMK SIO driver confusion

**Problem:** Initially implemented UART TX using the RP2040's PIO state machine
(`pio_uart_tx_init` / `pio_sm_put_blocking` via Pico SDK). This conflicted with QMK's
build system — the Pico SDK PIO header `hardware/pio.h` was not available in the QMK
ChibiOS build environment, causing compile errors.

**Fix:** Switched to QMK's built-in ChibiOS SIO UART driver (`uart_sio.c`).
Required:
- `config.h`: `UART_DRIVER SIOD0`, `UART_TX_PIN GP0`, `UART_TX_PAL_MODE 2`
- `mcuconf.h`: `RP_SIO_USE_UART0 TRUE`
- `rules.mk`: `UART_DRIVER_REQUIRED = yes`

The file `pio_uart.c` was kept (renamed rather than deleted) as a thin wrapper that
calls `uart_init()` / `uart_write()`. The PIO name is a historic artifact.

---

### 2. Baud rate mismatch / slow packet delivery

**Problem:** The initial baud rate was **57,600 bps** — matching common Arduino defaults.
Each 4-byte packet took ~694 µs, adding noticeable latency on every keypress.

**Fix:** Raised to **1,000,000 bps (1 Mbaud)** on both sides:
- RP2040: `uart_init(1000000)` in `pio_uart_init()`
- TENSTAR bridge: `Serial1.begin(1000000)` in both `ble_bridge.ino` and `esb_bridge.ino`

At 1 Mbaud a 4-byte packet takes ~40 µs — 17× faster.

> The early BLE bridge `ble_bridge.ino` still shows `Serial1.begin(57600)` in a
> comment block; the active code uses 1 Mbaud. Keep the two sides in sync if you
> modify the baud rate.

---

### 3. Consumer keys not forwarded wirelessly (encoder mute button)

**Problem:** The encoder push button triggered a mute action via `AUDIO_MUTE` in the
keymap, but QMK processed it as a USB HID consumer event — it never reached the
wireless transmission path in `process_record_kb()`.

**Fix:** Moved encoder button handling out of the keymap entirely. The button is
polled directly via `gpio_read_pin(ENCODER_SWITCH_PIN)` in `matrix_scan_kb()`, and a
raw consumer packet (`0x00E2` = AUDIO_MUTE) is sent directly over UART. The matrix
position `[3,3]` is mapped to `XXXXXXX` (blocked) so QMK never sees it as a keycode.

---

### 4. Consumer key "stuck" on dongle (key-up dropped)

**Problem:** On the ESB dongle, a consumer key press (e.g. mute) would stick — the
press report was received but the release was dropped because `usb_hid.ready()` was
false when the type `0x04` (consumer up) packet arrived.

**Fix:** Changed `consumer_pulse()` to send press and release atomically in a single
function, using `hid_wait_ready(5)` (blocks up to 5 ms) between the two USB HID
reports. The `0x04` consumer-up type is now ignored at the dongle level since
`consumer_pulse()` always self-releases.

---

### 5. Ghost keys from floating MUX channels

**Problem:** When `mux_analog.c` was first enabled (before `mux_channel_map[]` was
populated), all 16 channels were mapped to matrix positions. Unconnected channels float
to unpredictable voltages and fire ghost keypresses constantly.

**Fix:** All 16 channels in `mux_channel_map[]` default to `{MUX_UNUSED, MUX_UNUSED}`.
Additionally, `matrix_scan_custom()` skips any channel whose `baseline` ADC reading
is below `MUX_MIN_BASELINE` (100 counts) at boot, treating it as unconnected.
Enable channels by assigning real `{row, col}` values only for physically wired sensors.

---

### 6. RGB brightness indicator bypass

**Problem:** `rgb_matrix_set_color()` writes raw 8-bit RGB values that bypass the
global brightness setting. Layer indicator LEDs stayed at full brightness even after
using `RM_VALD` to dim the keyboard.

**Fix:** Implemented `set_led()` helper in `keymap.c` that scales the requested RGB
values by `rgb_matrix_get_val() / 255` before calling `rgb_matrix_set_color()`.
Indicators now respond to brightness controls alongside the animation.

---

### 7. GitHub repo setup — git initialized inside existing QMK repo

**Context:** The `proofofconcept` folder lives inside the `qmk_firmware` git
repository at `keyboards/proofofconcept/`. When setting up the GitHub remote, `git init`
was run **inside** `proofofconcept/` to create an independent nested repository.

**Result:** This creates a valid inner git repo (git supports nested repos when the
inner `.git` is present). The outer `qmk_firmware` repo treats `proofofconcept/` as
an untracked directory (`??`) since it has its own `.git`. This is intentional —
the keypad firmware is tracked separately from the QMK codebase.

**Remote:** `https://github.com/livid05/ProofOfConcept.git`

**`.gitignore`:** The `.claude/` folder is excluded from the repo since it contains
Claude Code editor configuration and worktree state (not project code).

---

*README generated May 2, 2026 — reflects the state of the project at initial GitHub push.*
