#pragma once
#include <Arduino.h>

// ─── Shared radio config — must match dongle exactly ─────────────────────────
#define RADIO_CHANNEL    80          // 2480 MHz — clear of most WiFi
#define RADIO_BASE_ADDR  0xDEADBEEFUL
#define RADIO_PREFIX     0xAB
#define RADIO_PAYLOAD    3           // bytes per packet: [type][b1][b2]

void radio_init_tx(void);
void radio_send_n(const uint8_t *data, uint8_t count);  // send same packet N times using TXIDLE (no re-ramp)
