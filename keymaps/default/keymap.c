// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _FN2,
    _RGB
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [_BASE] = LAYOUT_ortho_4x4(
        KC_1,    KC_2,    KC_3,    KC_LSFT,  // F1 = shift
        KC_4,    KC_5,    KC_6,    MO(_FN2), // F2 = function layer
        KC_7,    KC_8,    KC_9,    MO(_RGB), // F3 = RGB layer
        KC_PPLS, KC_PMNS, KC_0,    XXXXXXX   // encoder
    ),

    // F2 held: media, system utilities, bootloader
    [_FN2] = LAYOUT_ortho_4x4(
        KC_MPRV, KC_MPLY, KC_MNXT, _______,  // media prev / play / next
        KC_CALC, KC_NUM,  KC_PSCR, _______,  // calculator / numlock / print screen
        KC_BRID, KC_BRIU, KC_MUTE, _______,  // brightness down / up / mute
        QK_BOOT, EE_CLR,  _______, XXXXXXX   // bootloader / clear EEPROM
    ),

    // F3 held: RGB controls
    [_RGB] = LAYOUT_ortho_4x4(
        RM_TOGG, RM_NEXT, RM_PREV, _______,  // toggle / next mode / prev mode
        RM_HUEU, RM_HUED, RM_SATU, RM_SATD,  // hue / saturation
        RM_VALU, RM_VALD, RM_SPDU, RM_SPDD,  // brightness / speed
        _______, _______, _______, XXXXXXX   // encoder
    )
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) }, // volume
    [_FN2]  = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT) }, // track skip
    [_RGB]  = { ENCODER_CCW_CW(RM_VALD, RM_VALU) }  // brightness
};
#endif

// ─── RGB layer indicators ────────────────────────────────────────────────────
// Lights all active keys on the current layer, plus dim hints on base layer
// showing F2 (blue) and F3 (purple) are available.

// Helper: get LED index for a matrix position, returns NO_LED if none.
static inline uint8_t led_at(uint8_t row, uint8_t col) {
    return g_led_config.matrix_co[row][col];
}

// Helper: set a key's LED, scaled by the current RGB matrix brightness.
// rgb_matrix_set_color() writes raw values that bypass brightness — we scale
// manually so indicator colours respond to RM_VALD / RM_VALU like everything else.
static inline void set_led(uint8_t led, uint8_t r, uint8_t g, uint8_t b,
                            uint8_t led_min, uint8_t led_max) {
    if (led != NO_LED && led >= led_min && led < led_max) {
        uint8_t v = rgb_matrix_get_val();
        rgb_matrix_set_color(led,
            (uint8_t)((uint16_t)r * v / 255),
            (uint8_t)((uint16_t)g * v / 255),
            (uint8_t)((uint16_t)b * v / 255));
    }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);

    switch (layer) {

        case _FN2:
            // All active FN2 keys → cyan blue
            set_led(led_at(0, 0), 0,   180, 255, led_min, led_max); // prev track
            set_led(led_at(0, 1), 0,   180, 255, led_min, led_max); // play
            set_led(led_at(0, 2), 0,   180, 255, led_min, led_max); // next track
            set_led(led_at(1, 0), 0,   180, 255, led_min, led_max); // calculator
            set_led(led_at(1, 1), 0,   180, 255, led_min, led_max); // numlock
            set_led(led_at(1, 2), 0,   180, 255, led_min, led_max); // print screen
            set_led(led_at(2, 0), 0,   180, 255, led_min, led_max); // brightness down
            set_led(led_at(2, 1), 0,   180, 255, led_min, led_max); // brightness up
            set_led(led_at(2, 2), 0,   180, 255, led_min, led_max); // mute
            set_led(led_at(3, 0), 255, 50,  0,   led_min, led_max); // bootloader → orange warning
            set_led(led_at(3, 1), 255, 50,  0,   led_min, led_max); // EE_CLR → orange warning
            set_led(led_at(1, 3), 255, 255, 255, led_min, led_max); // F2 key itself → white (held)
            break;

        case _RGB:
            // All active RGB keys → purple
            set_led(led_at(0, 0), 180, 0,   255, led_min, led_max); // toggle
            set_led(led_at(0, 1), 180, 0,   255, led_min, led_max); // next mode
            set_led(led_at(0, 2), 180, 0,   255, led_min, led_max); // prev mode
            set_led(led_at(1, 0), 180, 0,   255, led_min, led_max); // hue up
            set_led(led_at(1, 1), 180, 0,   255, led_min, led_max); // hue down
            set_led(led_at(1, 2), 180, 0,   255, led_min, led_max); // sat up
            set_led(led_at(1, 3), 180, 0,   255, led_min, led_max); // sat down
            set_led(led_at(2, 0), 180, 0,   255, led_min, led_max); // brightness up
            set_led(led_at(2, 1), 180, 0,   255, led_min, led_max); // brightness down
            set_led(led_at(2, 2), 180, 0,   255, led_min, led_max); // speed up
            set_led(led_at(2, 3), 255, 255, 255, led_min, led_max); // F3 key itself → white (held)
            set_led(led_at(3, 0), 180, 0,   255, led_min, led_max); // speed down
            break;

        default: // _BASE
            // Dim hints so user knows F2 and F3 are available
            set_led(led_at(1, 3), 0,   0,   80,  led_min, led_max); // F2 → dim blue
            set_led(led_at(2, 3), 80,  0,   80,  led_min, led_max); // F3 → dim purple
            break;
    }

    return false;
}
