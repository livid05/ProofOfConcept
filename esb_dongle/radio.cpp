#include "radio.h"
#include <nrfx.h>
#include <string.h>

static uint8_t rx_buf[RADIO_PAYLOAD];

static void hfclk_start(void) {
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (!NRF_CLOCK->EVENTS_HFCLKSTARTED);
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
}

static void radio_configure(void) {
    NRF_RADIO->POWER = 1;

    NRF_RADIO->MODE    = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
    NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos;
    NRF_RADIO->FREQUENCY = RADIO_CHANNEL;

    NRF_RADIO->BASE0       = RADIO_BASE_ADDR;
    NRF_RADIO->PREFIX0     = RADIO_PREFIX;
    NRF_RADIO->TXADDRESS   = 0;
    NRF_RADIO->RXADDRESSES = 1;

    NRF_RADIO->PCNF0 = 0;

    NRF_RADIO->PCNF1 = (RADIO_PAYLOAD << RADIO_PCNF1_MAXLEN_Pos)  |
                       (RADIO_PAYLOAD << RADIO_PCNF1_STATLEN_Pos)  |
                       (3UL           << RADIO_PCNF1_BALEN_Pos)    |
                       (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos);

    NRF_RADIO->CRCCNF  = (RADIO_CRCCNF_LEN_Two      << RADIO_CRCCNF_LEN_Pos) |
                         (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
    NRF_RADIO->CRCPOLY = 0x11021;
    NRF_RADIO->CRCINIT = 0xFFFF;
}

void radio_init_rx(void) {
    hfclk_start();
    radio_configure();
    NRF_RADIO->PACKETPTR = (uint32_t)rx_buf;
    // Shortcut: READY→START (start listening as soon as rampup done)
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk;

    // Start listening
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN   = 1;
    while (!NRF_RADIO->EVENTS_READY);
    NRF_RADIO->EVENTS_READY = 0;
    // Shortcut fires START automatically
}

bool radio_recv(uint8_t *data) {
    if (!NRF_RADIO->EVENTS_END) return false;
    NRF_RADIO->EVENTS_END = 0;

    bool ok = (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk);
    if (ok) memcpy(data, rx_buf, RADIO_PAYLOAD);

    // Restart listen immediately
    NRF_RADIO->TASKS_START = 1;

    return ok;
}
