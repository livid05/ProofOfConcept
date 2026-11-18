/* Change board wiring, key assignment and debounce timing here.
 * Hardware: Devboard RevK + Numpad RevE, STM32U5A5RJT6.
 * GPIO port clocks are enabled by Board_Init() in main.c.
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include "stm32u5xx_hal.h"

/* ROW1 connects to PC0; COL1 connects to PB4. This intersection is MX2.
 * Row is an OPEN-DRAIN output held LOW. Column is an input with a PULL-UP.
 * Pressing MX2 completes the column -> switch -> diode -> row -> GND path.
 * The column therefore reads LOW when pressed. Never drive a column high.
 */
#define KEY_ROW_PORT GPIOC
#define KEY_ROW_PIN GPIO_PIN_0
#define KEY_COL_PORT GPIOB
#define KEY_COL_PIN GPIO_PIN_4
#define STATUS_LED_PORT GPIOA
#define STATUS_LED_PIN GPIO_PIN_5

/* A raw input must stay unchanged for this long before it becomes a press
 * or release. Increase if one physical tap produces several characters.
 */
#define KEY_DEBOUNCE_MS 10U

/* USB HID keyboard usage, NOT an ASCII character.
 * 0x04 = A key (normally lowercase 'a' with no modifiers on a US layout).
 * Examples: 0x05=B, 0x1E=top-row 1, 0x28=Enter, 0x59=keypad 1.
 * Caps Lock, Num Lock, keyboard layout and host repeat settings still apply.
 */
#define KEY_HID_USAGE 0x04U
#endif
