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

/* USB high-speed interrupt interval: 2^(bInterval-1) * 125 us.
 * 1 = 8K, 2 = 4K, 3 = 2K, 4 = 1K. Full-speed fallback remains 1K.
 * These are USB service opportunities, not promises of end-to-end latency.
 */
#define USB_HS_BINTERVAL 1U
#define USB_FS_BINTERVAL 1U

/* Foreground switch sampling target: one sample each 125 us (8K).
 * TIM2 provides microseconds; HAL_GetTick remains the ordinary 1 ms HAL tick.
 * Faster internal scanning is possible but does not increase USB polling.
 */
#define KEY_SCAN_PERIOD_US 125U

/* Eager press accepts the FIRST sampled closure without waiting 10 ms.
 * Release still requires 10 ms continuously open; bounce cannot re-trigger
 * while the accepted key is held. This reduces press latency but a noise
 * pulse can count as a press. Set KEY_EAGER_PRESS to 0 for symmetric, stable
 * 10 ms press/release filtering if electrical noise causes false presses.
 * Mechanical release latency is still intentionally longer than USB timing.
 */
#define KEY_EAGER_PRESS 1U
#define KEY_DEBOUNCE_MS 10U
#define KEY_DEBOUNCE_US (KEY_DEBOUNCE_MS * 1000U)

#if USB_HS_BINTERVAL < 1 || USB_HS_BINTERVAL > 16
#error "USB_HS_BINTERVAL must be between 1 and 16"
#endif
#if KEY_SCAN_PERIOD_US < 1 || KEY_DEBOUNCE_MS < 1
#error "Scan period and debounce duration must be positive"
#endif

/* USB HID keyboard usage, NOT an ASCII character.
 * 0x04 = A key (normally lowercase 'a' with no modifiers on a US layout).
 * Examples: 0x05=B, 0x1E=top-row 1, 0x28=Enter, 0x59=keypad 1.
 * Caps Lock, Num Lock, keyboard layout and host repeat settings still apply.
 */
#define KEY_HID_USAGE 0x04U
#endif
