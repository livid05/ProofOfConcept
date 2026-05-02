#include "quantum.h"
#include "pio_uart.h"

// ─── PIO UART TX on GP0 → TENSTAR D5 (P0.24 = Serial1 RX) ───────────────────
// 1 Mbaud = 10 bits × 1 µs = 10 µs/byte → 4-byte packet in 40 µs
// (was 694 µs at 57600 baud — 17× faster)
#define UART_TX_GPIO  0       // GP0, raw GPIO number for Pico SDK PIO calls
#define UART_BAUD     1000000

// ─── Wireless bridge protocol (4 bytes per packet) ───────────────────────────
// [0xAA] [type] [byte1] [byte2]
//   type 0x01 = key down     byte1=HID keycode
//   type 0x00 = key up       byte1=HID keycode
//   type 0x03 = consumer down byte1=usage_low, byte2=usage_high
//   type 0x04 = consumer up

static void tx_key(bool pressed, uint8_t keycode) {
    pio_uart_putc(0xAA);
    pio_uart_putc(pressed ? 0x01 : 0x00);
    pio_uart_putc(keycode);
    pio_uart_putc(0x00);
}

static void tx_consumer(bool pressed, uint16_t usage) {
    pio_uart_putc(0xAA);
    pio_uart_putc(pressed ? 0x03 : 0x04);
    pio_uart_putc((uint8_t)(usage & 0xFF));
    pio_uart_putc((uint8_t)(usage >> 8));
}

// Map QMK consumer keycode → HID Consumer Control usage code
static uint16_t consumer_usage(uint16_t kc) {
    switch (kc) {
        case KC_AUDIO_MUTE:        return 0x00E2;
        case KC_AUDIO_VOL_UP:      return 0x00E9;
        case KC_AUDIO_VOL_DOWN:    return 0x00EA;
        case KC_MEDIA_NEXT_TRACK:  return 0x00B5;
        case KC_MEDIA_PREV_TRACK:  return 0x00B6;
        case KC_MEDIA_PLAY_PAUSE:  return 0x00CD;
        case KC_CALCULATOR:        return 0x0192;
        case KC_BRIGHTNESS_UP:     return 0x006F;
        case KC_BRIGHTNESS_DOWN:   return 0x0070;
        default:                   return 0;
    }
}

// ─── Early init ──────────────────────────────────────────────────────────────
void keyboard_pre_init_kb(void) {
    gpio_set_pin_input_high(ENCODER_SWITCH_PIN);

    // Hand GP0 to PIO1 for hardware UART TX at 1 Mbaud
    pio_uart_init(UART_TX_GPIO, UART_BAUD);

    keyboard_pre_init_user();
}

// ─── Encoder push button ─────────────────────────────────────────────────────
void matrix_scan_kb(void) {
    static bool was_pressed = false;
    bool is_pressed = !gpio_read_pin(ENCODER_SWITCH_PIN);
    if (is_pressed != was_pressed) {
        was_pressed = is_pressed;
        tx_consumer(is_pressed, 0x00E2);   // AUDIO_MUTE — wireless only
    }
    matrix_scan_user();
}

// ─── Key event → wireless bridge ─────────────────────────────────────────────
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (IS_CONSUMER_KEYCODE(keycode)) {
        uint16_t usage = consumer_usage(keycode);
        if (usage) tx_consumer(record->event.pressed, usage);
        // Handled wirelessly — suppress USB HID for this keycode.
        (void)process_record_user(keycode, record);
        return false;
    }

    if (keycode < 0x0100) {
        tx_key(record->event.pressed, (uint8_t)keycode);
        // Handled wirelessly — suppress USB HID for this keycode.
        (void)process_record_user(keycode, record);
        return false;
    }

    // Layer keys (MO, TO, TG), RGB keys (RM_*), QK_BOOT, EE_CLR, etc.
    // These are QMK-internal — they don't generate USB HID output themselves,
    // so we let QMK process them normally (layer switches, RGB effects, etc.).
    return process_record_user(keycode, record);
}
