#pragma once
#include <Arduino.h>

// ─── Shared radio config — must match esb_bridge exactly ─────────────────────
#define RADIO_CHANNEL    80
#define RADIO_BASE_ADDR  0xDEADBEEFUL
#define RADIO_PREFIX     0xAB
#define RADIO_PAYLOAD    3           // bytes per packet: [type][b1][b2]

void radio_init_rx(void);
bool radio_recv(uint8_t *data);     // returns true when a valid packet arrives
