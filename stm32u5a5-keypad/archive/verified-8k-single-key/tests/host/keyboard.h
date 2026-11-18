#include <stdint.h>
typedef struct { volatile uint8_t dev_state; } USBD_HandleTypeDef;
#define USBD_STATE_CONFIGURED 3
extern USBD_HandleTypeDef usb_device;
extern volatile uint32_t keyboard_epoch;
uint8_t Keyboard_Send(uint8_t usage);
void Keyboard_Task(void);
