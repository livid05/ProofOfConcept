#include <stdint.h>
#define GPIOA 1
#define GPIOB 2
#define GPIOC 3
#define GPIO_PIN_0 1
#define GPIO_PIN_4 16
#define GPIO_PIN_5 32
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
uint32_t HAL_GetTick(void);
int HAL_GPIO_ReadPin(int port, int pin);
void HAL_GPIO_WritePin(int port, int pin, int value);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t mask);
