#include "radio.h"
#include <nrfx.h>
#include <string.h>

static uint8_t tx_buf[RADIO_PAYLOAD];

// Start the 16 MHz crystal oscillator — RADIO won't work without it
static void hfclk_start(void) {
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (!NRF_CLOCK->EVENTS_HFCLKSTARTED);
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
}

// Apply settings common to TX and RX
static void radio_configure(void) {
    NRF_RADIO->POWER = 1;

    // Nordic proprietary 2 Mbps — same on-air format as ESB
    NRF_RADIO->MODE    = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
    NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos;
    NRF_RADIO->FREQUENCY = RADIO_CHANNEL;

    // 4-byte base address + 1-byte prefix = 5 bytes on air
    NRF_RADIO->BASE0       = RADIO_BASE_ADDR;
    NRF_RADIO->PREFIX0     = RADIO_PREFIX;
    NRF_RADIO->TXADDRESS   = 0;   // logical address 0 for TX
    NRF_RADIO->RXADDRESSES = 1;   // enable logical address 0 for RX

    // Packet config 0: no S0/S1 length fields, LENGTH field = 0 bits (fixed payload)
    NRF_RADIO->PCNF0 = 0;

    // Packet config 1: fixed 3-byte payload, 4-byte base addr, little-endian, no whitening
    NRF_RADIO->PCNF1 = (RADIO_PAYLOAD << RADIO_PCNF1_MAXLEN_Pos)  |
                       (RADIO_PAYLOAD << RADIO_PCNF1_STATLEN_Pos)  |
                       (3UL           << RADIO_PCNF1_BALEN_Pos)    |
                       (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos);

    // CRC: 2 bytes, skip address, CCITT-16 polynomial, init 0xFFFF
    NRF_RADIO->CRCCNF  = (RADIO_CRCCNF_LEN_Two   << RADIO_CRCCNF_LEN_Pos) |
                         (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
    NRF_RADIO->CRCPOLY = 0x11021;
    NRF_RADIO->CRCINIT = 0xFFFF;
}

void radio_init_tx(void) {
    hfclk_start();
    radio_configure();
    NRF_RADIO->PACKETPTR = (uint32_t)tx_buf;
    // READY→START shortcut only — we manage END manually to stay in TXIDLE
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk;
}

// Send the same packet N times.
// First send: full ramp-up (~130 µs). Subsequent sends: TXIDLE→TX (~0 µs ramp).
// Total for N=3: ~130+174+174+174 µs ≈ 650 µs vs 3×300 µs = 900 µs the old way.
void radio_send_n(const uint8_t *data, uint8_t count) {
    if (count == 0) return;
    memcpy(tx_buf, data, RADIO_PAYLOAD);

    NRF_RADIO->EVENTS_END      = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;

    // First packet — full ramp-up via TXEN (READY→START shortcut fires automatically)
    NRF_RADIO->TASKS_TXEN = 1;
    while (!NRF_RADIO->EVENTS_END);
    NRF_RADIO->EVENTS_END = 0;
    // Radio is now in TXIDLE — no ramp needed for subsequent packets

    for (uint8_t i = 1; i < count; i++) {
        NRF_RADIO->TASKS_START = 1;   // fire immediately from TXIDLE
        while (!NRF_RADIO->EVENTS_END);
        NRF_RADIO->EVENTS_END = 0;
    }

    // Done — disable radio
    NRF_RADIO->TASKS_DISABLE = 1;
    while (!NRF_RADIO->EVENTS_DISABLED);
    NRF_RADIO->EVENTS_DISABLED = 0;
}
