// ProofOfConcept BLE HID Bridge
// Runs on: TENSTAR Pro Micro NRF52840 (Adafruit Feather nRF52840 Express board def)
//
// Receives key events from the RP2040 over UART and forwards them to the
// connected PC as a BLE HID keyboard.
//
// Wiring:
//   RP2040 GP0 (TX)  →  TENSTAR D5 (P0.24 = Serial1 RX on Feather board def)
//   RP2040 GND       →  TENSTAR GND
//
// Protocol (4 bytes per packet):
//   [0xAA] [type] [byte1] [byte2]
//   type 0x01 = keyboard key down   byte1=HID keycode,   byte2=0
//   type 0x00 = keyboard key up     byte1=HID keycode,   byte2=0
//   type 0x03 = consumer key down   byte1=usage_low,     byte2=usage_high
//   type 0x04 = consumer key up     (release all)        byte1=0, byte2=0

#include <bluefruit.h>

BLEDis         bledis;
BLEHidAdafruit blehid;

// ─── Keyboard key state ──────────────────────────────────────────────────────
static uint8_t modifier  = 0;
static uint8_t keys[6]   = {0};
static uint8_t key_count = 0;

static bool is_modifier(uint8_t kc) { return kc >= 0xE0 && kc <= 0xE7; }
static uint8_t modifier_bit(uint8_t kc) { return 1 << (kc - 0xE0); }

static void key_down(uint8_t kc) {
    if (is_modifier(kc)) {
        modifier |= modifier_bit(kc);
    } else {
        for (uint8_t i = 0; i < key_count; i++) {
            if (keys[i] == kc) return;
        }
        if (key_count < 6) keys[key_count++] = kc;
    }
    if (Bluefruit.connected()) blehid.keyboardReport(modifier, keys);
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
    if (Bluefruit.connected()) blehid.keyboardReport(modifier, keys);
}

// ─── BLE advertising ─────────────────────────────────────────────────────────
void startAdv(void) {
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.setFastTimeout(30);
    Bluefruit.Advertising.start(0);
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);    // USB serial for debug
    Serial1.begin(57600);    // UART from RP2040 (GP0 → D5/P0.24)

    Bluefruit.begin();
    Bluefruit.setTxPower(4);
    Bluefruit.setName("ProofOfConcept");
    // Request the fastest BLE connection interval the spec allows for HID (7.5 ms).
    // Units are 1.25 ms: 6 = 7.5 ms, 12 = 15 ms.
    Bluefruit.Periph.setConnInterval(6, 12);

    bledis.setManufacturer("TylerJudkins");
    bledis.setModel("ProofOfConcept");
    bledis.begin();

    blehid.begin();

    startAdv();
}

// ─── Main loop ───────────────────────────────────────────────────────────────
#define START_BYTE 0xAA
#define PKT_LEN    4   // start + type + byte1 + byte2

static uint8_t buf[PKT_LEN];
static uint8_t idx    = 0;
static bool    synced = false;

void loop() {
    while (Serial1.available()) {
        uint8_t b = Serial1.read();

        if (!synced) {
            if (b == START_BYTE) { synced = true; idx = 0; }
            continue;
        }

        buf[idx++] = b;

        if (idx == PKT_LEN - 1) {  // received type + byte1 + byte2
            uint8_t  type  = buf[0];
            uint8_t  b1    = buf[1];
            uint8_t  b2    = buf[2];
            uint16_t usage = (uint16_t)b1 | ((uint16_t)b2 << 8);

            if      (type == 0x01) key_down(b1);
            else if (type == 0x00) key_up(b1);
            else if (type == 0x03 && Bluefruit.connected()) blehid.consumerKeyPress(usage);
            else if (type == 0x04 && Bluefruit.connected()) blehid.consumerKeyPress(0);

            synced = false;
            idx    = 0;
        }
    }
}
