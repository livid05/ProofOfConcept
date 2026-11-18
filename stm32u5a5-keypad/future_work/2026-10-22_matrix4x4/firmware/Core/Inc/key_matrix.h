/* key_matrix.h - public interface of the 4 x 4 matrix scanner.
 * FUTURE WORK, NOT YET PART OF THE BUILD.
 *
 * Two layers, so the logic can be tested on a PC without a board:
 *   matrix_hw.c   talks to GPIO pins (Matrix_GpioInit, Matrix_ReadRaw)
 *   key_matrix.c  debounce, report building, queueing (Keyboard_Task)
 */
#ifndef KEY_MATRIX_H
#define KEY_MATRIX_H

#include <stdint.h>
#include "matrix_config.h"

/* Hardware layer (matrix_hw.c). */

/* Call once at startup AFTER the GPIOB and GPIOC clocks are enabled.
 * Releases all rows (floating) and turns the column pins into pulled-up inputs.
 */
void Matrix_GpioInit(void);

/* Scans every row once and returns a 16-bit mask: bit (row * 4 + col) is 1
 * while that key's contact is CLOSED (column read low). Takes about
 * MATRIX_ROWS * MATRIX_SETTLE_US microseconds.
 */
uint16_t Matrix_ReadRaw(void);

/* Logic layer (key_matrix.c). */

/* HID usage for each matrix position (see matrix_config.h). */
extern const uint8_t matrix_keymap[MATRIX_KEYS];

/* Applies one key press or release to an 8-byte boot-keyboard report:
 *   report[0] = modifier bits, report[1] = reserved, report[2..7] = keys.
 * Returns 1 normally, 0 if a press was dropped because six keys are already
 * held. Keys keep the order in which they were pressed.
 */
uint8_t Matrix_ReportApply(uint8_t report[8], uint8_t usage, uint8_t pressed);

/* Debug counters: watch these in a debugger (all in microseconds or counts). */
extern volatile uint32_t matrix_scan_last_us;     /* duration of the latest full scan */
extern volatile uint32_t matrix_scan_max_us;      /* longest full scan so far */
extern volatile uint32_t matrix_rollover_dropped; /* presses ignored (7th+ key) */
extern volatile uint16_t matrix_pressed_mask;     /* debounced keys currently down */

#endif
