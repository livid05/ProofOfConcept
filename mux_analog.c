// HC4067 analog multiplexer driver for QMK (CUSTOM_MATRIX = lite)
//
// Scans 16 channels, compares each ADC reading to a per-channel baseline
// captured at boot, and ORs pressed keys into the existing matrix so normal
// matrix keys continue to work alongside the mux keys.
//
// Pin defaults (GP22/26/27/28 select, GP29 signal) are set in mux_analog.h.
// Override them with #define MUX_Sx_PIN / MUX_SIG_PIN in your config.h.
//
// Channel-to-matrix mapping:
//   Edit mux_channel_map[] below so each MUX channel index lines up with the
//   row/col position in your keymap.  VIA layer changes then apply automatically
//   because only the matrix position is stored here, not the keycode.
//   Set {MUX_UNUSED, MUX_UNUSED} for any channel that has nothing wired to it.

#include QMK_KEYBOARD_H
#include "mux_analog.h"
#include "analog.h"
#include "wait.h"
#include "timer.h"
#include "print.h"

// ─── Channel → matrix position ───────────────────────────────────────────────
// Rows and columns must fit within your keyboard's MATRIX_ROWS / MATRIX_COLS.
// Default: linear mapping CH0→(0,0) … CH15→(3,3) for a 4×4 grid.
// Adjust to match the physical order your sensors are wired to the MUX.
// Map each MUX channel to a matrix [row, col] position.
// Set {MUX_UNUSED, MUX_UNUSED} for any channel without a sensor wired to it.
// Unconnected channels float and will ghost-fire if left enabled.
const mux_channel_map_t mux_channel_map[MUX_CHANNELS] = {
    /* CH  0 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  1 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  2 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  3 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  4 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  5 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  6 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  7 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  8 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH  9 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH 10 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH 11 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH 12 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH 13 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH 14 */ {MUX_UNUSED, MUX_UNUSED},
    /* CH 15 */ {MUX_UNUSED, MUX_UNUSED},
};

// ─── Internal state ──────────────────────────────────────────────────────────
static uint16_t baseline[MUX_CHANNELS];
static bool     key_state[MUX_CHANNELS];
static uint32_t debounce_timer[MUX_CHANNELS];

// ─── Helpers ─────────────────────────────────────────────────────────────────
static void select_channel(uint8_t ch) {
    gpio_write_pin(MUX_S0_PIN, (ch >> 0) & 1);
    gpio_write_pin(MUX_S1_PIN, (ch >> 1) & 1);
    gpio_write_pin(MUX_S2_PIN, (ch >> 2) & 1);
    gpio_write_pin(MUX_S3_PIN, (ch >> 3) & 1);
    wait_us(50); // HC4067 propagation + settling
}

static uint16_t sample_channel(uint8_t ch) {
    select_channel(ch);
    return analogReadPin(MUX_SIG_PIN);
}

// ─── QMK custom-matrix hooks ─────────────────────────────────────────────────

void matrix_init_custom(void) {
    gpio_set_pin_output(MUX_S0_PIN);
    gpio_set_pin_output(MUX_S1_PIN);
    gpio_set_pin_output(MUX_S2_PIN);
    gpio_set_pin_output(MUX_S3_PIN);

    // Capture resting baseline for each channel (keys must be unpressed at boot).
    for (uint8_t ch = 0; ch < MUX_CHANNELS; ch++) {
        uint32_t sum = 0;
        for (uint8_t s = 0; s < MUX_CALIBRATION_SAMPLES; s++) {
            sum += sample_channel(ch);
            wait_us(200);
        }
        baseline[ch]       = (uint16_t)(sum / MUX_CALIBRATION_SAMPLES);
        key_state[ch]      = false;
        debounce_timer[ch] = 0;
        uprintf("MUX CH%02u baseline=%u\n", ch, baseline[ch]);
    }
}

// Called by QMK after its own matrix scan; we OR mux key states in so both
// the normal matrix and the mux keys are active at the same time.
bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool     changed = false;
    uint32_t now     = timer_read32();

    for (uint8_t ch = 0; ch < MUX_CHANNELS; ch++) {
        uint8_t row = mux_channel_map[ch].row;
        uint8_t col = mux_channel_map[ch].col;

        if (row == MUX_UNUSED) continue;               // channel not wired
        if (baseline[ch] < MUX_MIN_BASELINE) continue; // looks unconnected

        uint16_t reading = sample_channel(ch);

        // Pressed when voltage drops ≥ MUX_THRESHOLD_PCT% from resting value.
        uint16_t drop_threshold = baseline[ch] - (uint16_t)((uint32_t)baseline[ch] * MUX_THRESHOLD_PCT / 100);
        bool should_press = (reading < drop_threshold);

        // Debounce: only accept a state change after the window has elapsed.
        if (timer_elapsed32(debounce_timer[ch]) > MUX_DEBOUNCE_MS) {
            if (should_press != key_state[ch]) {
                key_state[ch]      = should_press;
                debounce_timer[ch] = now;
                changed            = true;
                uprintf("MUX CH%02u %s (adc=%u baseline=%u)\n",
                        ch, should_press ? "DOWN" : "UP", reading, baseline[ch]);
            }
        }

        // Inject into matrix — OR so normal matrix keys are not disturbed.
        if (key_state[ch]) {
            current_matrix[row] |= (matrix_row_t)1 << col;
        }
    }

    return changed;
}
