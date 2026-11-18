/* TIM2 is a 32-bit counter. Its 1 MHz count gives one tick per microsecond.
 * This avoids raising SysTick from 1K to 8K (HAL timeouts still use ms),
 * avoids depending on debugger access to DWT, and requires no timer IRQ.
 */
#include "key_clock.h"
#include "main.h"

void KeyClock_Init(void)
{
    uint32_t peripheral_hz = HAL_RCC_GetPCLK1Freq();
    /* Current clock setup has APB1=/1, so TIM2 clock equals PCLK1 (160 MHz).
     * Check that assumption instead of silently mis-timing the scanner if
     * someone later changes the APB divider/timer clock configuration.
     */
    if (peripheral_hz != HAL_RCC_GetHCLKFreq() || peripheral_hz < 1000000U ||
        peripheral_hz % 1000000U != 0U)
        Error_Handler();

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();
    TIM2->PSC = peripheral_hz / 1000000U - 1U; /* 160 MHz / 160 = 1 MHz. */
    TIM2->ARR = UINT32_MAX;                    /* Wraps every ~71.6 minutes. */
    TIM2->EGR = TIM_EGR_UG;                    /* Load the new prescaler now. */
    TIM2->SR = 0;
    TIM2->CNT = 0;
    __HAL_DBGMCU_FREEZE_TIM2(); /* Pause scan time at breakpoints. */
    TIM2->CR1 = TIM_CR1_CEN;
}

uint32_t KeyClock_NowUs(void)
{
    /* Callers use unsigned subtraction, so short timing intervals remain
     * correct when this counter rolls over from 0xFFFFFFFF back to zero.
     */
    return TIM2->CNT;
}
