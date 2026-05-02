// ProofOfConcept 2.4GHz Bridge
// Runs on: TENSTAR Pro Micro NRF52840 (Adafruit Feather nRF52840 Express board def)
//
// Receives key events from the RP2040 over UART and forwards them to the
// Adafruit nRF52840 USB dongle via 2.4 GHz Nordic proprietary radio.
//
// Wiring (unchanged from BLE bridge):
//   RP2040 GP0 (TX)  →  TENSTAR D5 (P0.24 = Serial1 RX)
//   RP2040 GND       →  TENSTAR GND
//
// Protocol (4 bytes per packet, unchanged from RP2040 side):
//   [0xAA] [type] [byte1] [byte2]

#include "radio.h"
#include <Adafruit_TinyUSB.h>  // required by core even when USB not used directly

// ─── UART packet parser ───────────────────────────────────────────────────────
#define START_BYTE  0xAA
#define PKT_LEN     4

static uint8_t buf[PKT_LEN];
static uint8_t idx    = 0;
static bool    synced = false;

void setup() {
    Serial1.begin(1000000); // UART from RP2040 (PIO @ 1 Mbaud)
    radio_init_tx();
}

void loop() {
    while (Serial1.available()) {
        uint8_t b = Serial1.read();

        if (!synced) {
            if (b == START_BYTE) { synced = true; idx = 0; }
            continue;
        }

        buf[idx++] = b;

        if (idx == PKT_LEN - 1) {
            // 3x via TXIDLE — no re-ramp between copies, ~650 µs total
            radio_send_n(&buf[0], 3);
            synced = false;
            idx    = 0;
        }
    }
}
