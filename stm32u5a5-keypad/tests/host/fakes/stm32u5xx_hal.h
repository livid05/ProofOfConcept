/* Minimal stand-in for ST's HAL, used ONLY by the PC tests.
 * It lets the real key_matrix.c and matrix_hw.c compile on Windows without the
 * STM32 headers. The test programs supply the function bodies.
 */
#ifndef FAKE_STM32U5XX_HAL_H
#define FAKE_STM32U5XX_HAL_H
#include <stdint.h>

typedef struct { uint32_t id; } GPIO_TypeDef;
extern GPIO_TypeDef fake_gpio_a, fake_gpio_b, fake_gpio_c;
#define GPIOA (&fake_gpio_a)
#define GPIOB (&fake_gpio_b)
#define GPIOC (&fake_gpio_c)

#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_1 ((uint16_t)0x0002)
#define GPIO_PIN_2 ((uint16_t)0x0004)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)
#define GPIO_PIN_7 ((uint16_t)0x0080)

typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
#define GPIO_MODE_INPUT 0x00000000U
#define GPIO_MODE_OUTPUT_OD 0x00000011U
#define GPIO_NOPULL 0x00000000U
#define GPIO_PULLUP 0x00000001U
#define GPIO_SPEED_FREQ_LOW 0x00000000U

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pins, GPIO_PinState state);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
uint32_t HAL_GetTick(void);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t mask);
#endif
