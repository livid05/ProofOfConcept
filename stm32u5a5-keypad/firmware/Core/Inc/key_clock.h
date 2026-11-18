/* A microsecond clock for key scanning. TIM2 is reserved by this application;
 * do not also assign it to PWM, capture, or another HAL timer user.
 */
#ifndef KEY_CLOCK_H
#define KEY_CLOCK_H
#include <stdint.h>
void KeyClock_Init(void);
uint32_t KeyClock_NowUs(void);
#endif
