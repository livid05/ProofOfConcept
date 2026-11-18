/* Stand-in for the firmware's keyboard.h, used ONLY by the PC logic test.
 * Declares just what key_matrix.c uses from the USB layer.
 */
#ifndef FAKE_KEYBOARD_H
#define FAKE_KEYBOARD_H
#include <stdint.h>
#include "stm32u5xx_hal.h"
typedef struct { volatile uint8_t dev_state; } USBD_HandleTypeDef;
#define USBD_STATE_CONFIGURED 3
extern USBD_HandleTypeDef usb_device;
extern volatile uint32_t keyboard_epoch;
extern volatile uint32_t key_press_count, usb_report_count, key_queue_overflow;
extern volatile uint8_t key_pressed;
uint8_t Keyboard_SendReport(const uint8_t report[8]);
void Keyboard_Task(void);
#endif
