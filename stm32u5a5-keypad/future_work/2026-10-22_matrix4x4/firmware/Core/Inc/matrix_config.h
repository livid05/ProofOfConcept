/* matrix_config.h - settings for the 4 x 4 key matrix (Numpad RevE).
 *
 * FUTURE WORK, NOT YET PART OF THE BUILD. See ../../../INTEGRATION.md.
 * Every adjustable matrix setting lives here, next to (not inside) board_config.h.
 *
 * HOW THE MATRIX IS WIRED (verified against the KiCad RevE/RevK files):
 *   - 4 ROW wires go to devboard pins PC0, PC1, PC2, PC3 (ROW1..ROW4).
 *   - 4 COLUMN wires go to devboard pins PB4, PB5, PB6, PB7 (COL1..COL4).
 *   - Each key has a diode in series: column -> switch -> diode -> row.
 *
 * HOW A KEY IS DETECTED:
 *   - Rows are OPEN-DRAIN outputs. "High" means released (floating), "low"
 *     means pulled to ground. Only one row is pulled low at a time.
 *   - Columns are inputs with the MCU's internal PULL-UP, so they read HIGH
 *     when nothing is pressed.
 *   - With row R low, a pressed key in row R connects its column to ground
 *     through the switch and diode, so that column reads LOW. LOW = pressed.
 *   - The diodes stop current sneaking through other keys, so pressing three
 *     keys at once cannot create a phantom fourth key ("ghosting").
 *   - Never drive a row HIGH (push-pull): two rows fighting through pressed
 *     keys could damage the pins. Release only.
 */
#ifndef MATRIX_CONFIG_H
#define MATRIX_CONFIG_H

#include "board_config.h" /* KEY_SCAN_PERIOD_US, KEY_EAGER_PRESS, KEY_DEBOUNCE_US */

#define MATRIX_ROWS 4U
#define MATRIX_COLS 4U
#define MATRIX_KEYS (MATRIX_ROWS * MATRIX_COLS)

/* A key's number is row * MATRIX_COLS + column (row-major, counting from 0).
 * That number is its bit position in the 16-bit "pressed" masks, so the
 * matrix can be at most 16 positions. Larger boards need wider masks.
 */
_Static_assert(MATRIX_KEYS <= 16U, "Key masks are 16 bits wide; widen them for bigger matrices");

/* Row outputs (all on GPIO port C) and column inputs (all on GPIO port B).
 * Entry 0 is ROW1/COL1. Change these tables if the wires are rearranged.
 */
#define MATRIX_ROW_PORT GPIOC
#define MATRIX_ROW_PINS {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3}
#define MATRIX_COL_PORT GPIOB
#define MATRIX_COL_PINS {GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7}

/* Settling time after pulling a row low, before reading the columns.
 * Unit: microseconds. Why it exists: a column that was held low by the
 * previous row must climb back to HIGH through the weak internal pull-up
 * (roughly 40 kohm) and the wire/board capacitance. That takes a few
 * microseconds. Reading too early would make an unpressed key look pressed.
 * 6 us x 4 rows = about 24 us of the 125 us scan period. The measured value
 * is exported in matrix_scan_max_us so this can be checked on real hardware.
 */
#define MATRIX_SETTLE_US 6U

/* Boot keyboards report at most six ordinary keys at once (plus modifiers).
 * A seventh simultaneous key is ignored and counted in matrix_rollover_dropped.
 */
#define MATRIX_REPORT_KEYS 6U

/* KEYMAP: one HID usage code per matrix position, row by row.
 * These are HID keyboard usages, NOT ASCII. 0x04 = A, 0x05 = B, ... 0x1D = Z.
 * 0x00 means "no key at this position": the position is never scanned into
 * reports. The bottom-right position (ROW4/COL4) is empty because the rotary
 * encoder sits there and is not part of the matrix.
 *
 * This bring-up map types a..o in reading order, so while testing you can
 * press each key and see which letter appears. Replace with the final layout
 * (e.g. 0x53 NumLock, 0x54 keypad /, 0x55 *, 0x56 -, 0x5F keypad 7, ...)
 * once every key is confirmed. Each usage must be UNIQUE (the report tracks
 * keys by usage). Modifier usages 0xE0..0xE7 (Ctrl, Shift, Alt, GUI) are
 * supported and set modifier bits instead of using a key slot.
 *
 *            COL1  COL2  COL3  COL4
 *   ROW1      a     b     c     d
 *   ROW2      e     f     g     h
 *   ROW3      i     j     k     l
 *   ROW4      m     n     o    (none = encoder position)
 */
#ifndef MATRIX_KEYMAP_INIT
#define MATRIX_KEYMAP_INIT                                                    \
    {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, \
     0x10, 0x11, 0x12, 0x00}
#endif

#endif
