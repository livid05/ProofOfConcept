
/* Run the actual key_switch.c with fake GPIO/USB/time, not a copied algorithm.
 * No CRT calls are used, allowing this tiny host executable to link standalone.
 * Return codes identify failing assertions. This does not test real USB or TIM2.
 */
#include "keyboard.h"
#include "board_config.h"
USBD_HandleTypeDef usb_device;
volatile uint32_t keyboard_epoch;
extern volatile uint32_t key_scan_count, key_queue_overflow;
static uint32_t now_us;
static uint8_t pressed, blocked, events[4096];
static unsigned event_count;
uint32_t KeyClock_NowUs(void){return now_us;}
uint32_t HAL_GetTick(void){return now_us/1000;}
int HAL_GPIO_ReadPin(int port,int pin){(void)port;(void)pin;return !pressed;}
void HAL_GPIO_WritePin(int port,int pin,int value){(void)port;(void)pin;(void)value;}
uint32_t __get_PRIMASK(void){return 0;}
void __disable_irq(void){}
void __set_PRIMASK(uint32_t mask){(void)mask;}
void Keyboard_Idle(void){}
uint8_t Keyboard_Send(uint8_t usage){if(blocked)return 0;if(event_count<4096)events[event_count++]=usage;return 1;}
static void ticks(unsigned n){while(n--){now_us+=125;Keyboard_Task();}}
#define CHECK(expr,code) do{if(!(expr))return code;}while(0)
int main(void){
 usb_device.dev_state=USBD_STATE_CONFIGURED;Keyboard_Task();ticks(100);
 CHECK(event_count==1 && events[0]==0,1);
 /* Eager first closure, then alternating contact bounce: just one press. */
 pressed=1;ticks(1);CHECK(event_count==2 && events[1]==KEY_HID_USAGE,2);
 for(unsigned n=0;n<12;n++){pressed=0;ticks(1);pressed=1;ticks(1);}
 CHECK(event_count==2,3);
 /* Release waits the full 10 ms quiet interval. */
 pressed=0;ticks(80);CHECK(event_count==2,4);ticks(1);
 CHECK(event_count==3 && events[2]==0,5);
 /* A busy endpoint must retain BOTH press and release, in order. */
 blocked=1;pressed=1;ticks(1);pressed=0;ticks(81);
 unsigned before=event_count;blocked=0;Keyboard_Task();Keyboard_Task();
 CHECK(event_count==before+2 && events[before]==KEY_HID_USAGE && events[before+1]==0,6);
 /* Held during reconnect: remain silent until release, then a fresh press. */
 usb_device.dev_state=0;pressed=1;ticks(2);
 usb_device.dev_state=USBD_STATE_CONFIGURED;keyboard_epoch++;ticks(100);
 CHECK(events[event_count-1]==0,7);before=event_count;pressed=0;ticks(81);
 for(unsigned n=before;n<event_count;n++)CHECK(events[n]==0,8);
 pressed=1;ticks(1);CHECK(events[event_count-1]==KEY_HID_USAGE,9);
 pressed=0;ticks(81);
 /* Overflow must recover with all keys up, rather than a stuck key. */
 blocked=1;for(unsigned n=0;n<20;n++){pressed=1;ticks(1);pressed=0;ticks(81);}
 CHECK(key_queue_overflow>0,10);blocked=0;ticks(100);
 CHECK(events[event_count-1]==0,11);
 /* Real uint32_t wrap: release debounce spans the counter rollover. */
 usb_device.dev_state=0;now_us=0xffffe000U;Keyboard_Task();ticks(100);
 usb_device.dev_state=USBD_STATE_CONFIGURED;keyboard_epoch++;ticks(100);
 pressed=1;ticks(1);CHECK(events[event_count-1]==KEY_HID_USAGE,12);
 now_us=0xffffff00U;Keyboard_Task();pressed=0;ticks(81);
 CHECK(events[event_count-1]==0,13);
 /* A nominal second produces 8000 real input samples; subperiod calls do not. */
 uint32_t scans=key_scan_count;ticks(8000);CHECK(key_scan_count-scans==8000,14);
 scans=key_scan_count;Keyboard_Task();CHECK(key_scan_count==scans,15);
 return 0;
}
