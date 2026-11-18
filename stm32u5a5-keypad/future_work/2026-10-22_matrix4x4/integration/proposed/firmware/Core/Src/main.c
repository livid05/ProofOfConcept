/* One mechanical switch -> USB boot keyboard. STM32U5A5RJT6, RevK. */
#include "main.h"
#include "board_config.h"
#include "key_clock.h"
#include "keyboard.h"
#include "key_matrix.h"
/* The internal USB HS PHY needs the external 16 MHz crystal as its reference.
 * HSE / PLLM * PLLN / PLLR = 16 MHz / 1 * 20 / 2 = 160 MHz CPU/AHB.
 * VOS1 and four flash wait states support this speed. This is the LDO part;
 * do not copy an SMPS supply configuration from a different U5 board.
 */
void SystemClock_Config(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    RCC_OscInitTypeDef o = {0};
    RCC_ClkInitTypeDef c = {0};
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
        Error_Handler();
    o.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI;
    o.HSEState = RCC_HSE_ON;
    o.HSIState = RCC_HSI_ON;
    o.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    o.PLL.PLLState = RCC_PLL_ON;
    o.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    o.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    o.PLL.PLLM = 1;
    o.PLL.PLLN = 20;
    o.PLL.PLLP = 8;
    o.PLL.PLLQ = 2;
    o.PLL.PLLR = 2;
    o.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    o.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&o) != HAL_OK)
        Error_Handler();
    c.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                  RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    c.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    c.AHBCLKDivider = RCC_SYSCLK_DIV1;
    c.APB1CLKDivider = c.APB2CLKDivider = c.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&c, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();
}
/* LED pin plus the 4 x 4 key matrix pins (Matrix_GpioInit). */
static void Board_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_RESET);
    g.Pin = STATUS_LED_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_PORT, &g);
    /* Rows PC0..PC3 (open-drain, released) and columns PB4..PB7 (pull-up
     * inputs). The scanner pulls one row low at a time; see matrix_config.h.
     */
    Matrix_GpioInit();
}
/* HAL_Init installs the 1 ms SysTick timebase. USB transactions are handled
 * in OTG_HS_IRQHandler; the foreground loop samples and debounces the switch.
 * There is no RTOS, background key macro, or automatic text transmission.
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    Board_Init();
    KeyClock_Init(); /* Independent 1 us timebase for the 8K switch task. */
    if (USBD_Init(&usb_device, &keyboard_descriptors, 0) != USBD_OK ||
        USBD_RegisterClass(&usb_device, &keyboard_class) != USBD_OK ||
        USBD_Start(&usb_device) != USBD_OK)
        Error_Handler();
    while (1)
        Keyboard_Task();
}
/* A failure here occurs before normal keyboard operation. Keep SysTick
 * interrupts enabled so HAL_Delay still works and the error blink is visible.
 */
void Error_Handler(void)
{
    /* Distinct rapid LED flash for clock/USB initialization failure. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = STATUS_LED_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(STATUS_LED_PORT, &g);
    while (1)
    {
        HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
        HAL_Delay(100);
    }
}
