#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "usbd_core.h"
extern USBD_HandleTypeDef usb_device;
extern USBD_ClassTypeDef keyboard_class;
extern USBD_DescriptorsTypeDef keyboard_descriptors;
extern volatile uint8_t keyboard_busy;
extern volatile uint32_t keyboard_epoch;
extern volatile uint32_t key_press_count, usb_report_count, key_queue_overflow;
extern volatile uint8_t key_pressed;
uint8_t Keyboard_Send(uint8_t usage);
void Keyboard_Task(void);
#endif
