// ProofOfConcept 2.4GHz USB HID Dongle
// Runs on: Adafruit nRF52840 USB Key — board: "Raytac nRF52840 Dongle"
//
// Receives 2.4 GHz packets from the TENSTAR and outputs them as a
// USB HID keyboard + consumer control device to the PC.
//
// Plug into any USB port. No wiring required.

#include "radio.h"
#include <Adafruit_TinyUSB.h>

// ─── USB HID ─────────────────────────────────────────────────────────────────
static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(2)),
};

Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report),
                           HID_ITF_PROTOCOL_NONE, 1, false);  // 1ms poll interval

uint16_t get_report_cb(uint8_t id, hid_report_type_t type, uint8_t *buf, uint16_t len) {
    (void)id; (void)type; (void)buf; (void)len;
    return 0;
}
void set_report_cb(uint8_t id, hid_report_type_t type, uint8_t const *buf, uint16_t len) {
    (void)id; (void)type; (void)buf; (void)len;
}

// ─── Key state ───────────────────────────────────────────────────────────────
static uint8_t modifier  = 0;
static uint8_t keys[6]   = {0};
static uint8_t key_count = 0;

static bool    is_modifier(uint8_t kc) { return kc >= 0xE0 && kc <= 0xE7; }
static uint8_t modifier_bit(uint8_t kc) { return 1 << (kc - 0xE0); }

static void key_down(uint8_t kc) {
    if (is_modifier(kc)) {
        modifier |= modifier_bit(kc);
    } else {
        for (uint8_t i = 0; i < key_count; i++) if (keys[i] == kc) return;
        if (key_count < 6) keys[key_count++] = kc;
    }
    if (usb_hid.ready()) usb_hid.keyboardReport(1, modifier, keys);
}

static void key_up(uint8_t kc) {
    if (is_modifier(kc)) {
        modifier &= ~modifier_bit(kc);
    } else {
        for (uint8_t i = 0; i < key_count; i++) {
            if (keys[i] == kc) {
                for (uint8_t j = i; j < key_count - 1; j++) keys[j] = keys[j + 1];
                keys[--key_count] = 0;
                break;
            }
        }
    }
    if (usb_hid.ready()) usb_hid.keyboardReport(1, modifier, keys);
}

// Wait up to `ms` milliseconds for USB HID to be ready to send.
static bool hid_wait_ready(uint32_t ms) {
    uint32_t t0 = millis();
    while (!usb_hid.ready()) {
        if (millis() - t0 >= ms) return false;
    }
    return true;
}

// Send a consumer key as a self-contained press+release.
// This avoids the race where the key-up packet arrives while usb_hid is still
// busy sending the key-down report, which would silently drop the release.
static void consumer_pulse(uint16_t usage) {
    if (!hid_wait_ready(5)) return;
    usb_hid.sendReport16(2, usage);
    if (!hid_wait_ready(5)) return;
    usb_hid.sendReport16(2, 0);
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    usb_hid.setReportCallback(get_report_cb, set_report_cb);
    usb_hid.begin();

    // Wait for USB to enumerate before starting the radio
    while (!TinyUSBDevice.mounted()) delay(1);

    radio_init_rx();
}

// ─── Main loop ───────────────────────────────────────────────────────────────
// Dedup: ignore packets identical to the last one within 50ms
static uint8_t  last_pkt[RADIO_PAYLOAD] = {0xFF, 0xFF, 0xFF};
static uint32_t last_pkt_ms = 0;

void loop() {
    // ── Stuck-key safety net ──────────────────────────────────────────────────
    // If the wireless link drops or all copies of a key-up are lost, release
    // everything after 500ms of silence so no key stays held indefinitely.
    if ((key_count > 0 || modifier != 0) && (millis() - last_pkt_ms > 500)) {
        modifier  = 0;
        key_count = 0;
        memset(keys, 0, sizeof(keys));
        if (usb_hid.ready()) usb_hid.keyboardReport(1, 0, keys);
    }

    uint8_t pkt[RADIO_PAYLOAD];
    if (!radio_recv(pkt)) return;

    uint32_t now = millis();
    bool duplicate = (now - last_pkt_ms < 50) &&
                     (pkt[0] == last_pkt[0]) &&
                     (pkt[1] == last_pkt[1]) &&
                     (pkt[2] == last_pkt[2]);

    memcpy(last_pkt, pkt, RADIO_PAYLOAD);
    last_pkt_ms = now;

    if (duplicate) return;

    uint8_t  type  = pkt[0];
    uint8_t  b1    = pkt[1];
    uint8_t  b2    = pkt[2];
    uint16_t usage = (uint16_t)b1 | ((uint16_t)b2 << 8);

    if      (type == 0x01) key_down(b1);
    else if (type == 0x00) key_up(b1);
    else if (type == 0x03) consumer_pulse(usage);  // press+release atomically
    // type == 0x04: release is already handled inside consumer_pulse; ignore
}
