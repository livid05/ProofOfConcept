#ifndef BOARD_TEST_H
#define BOARD_TEST_H
#include "main.h"

/* RevK devboard + RevE numpad. Include from main.c only.
 * Replaces the previous board_test.h. Keep calling BoardTest_Task()
 * from while(1), after the generated MX_GPIO_Init(). No CubeMX change needed.
 */
volatile uint16_t numpad_keys = 0;       /* bit = row*4 + column, both zero-based */
volatile uint32_t key_press_count = 0;  /* counts newly pressed contacts */
volatile int32_t encoder_ticks = 0;     /* signed quadrature transitions */
volatile uint8_t encoder_button = 0;    /* 1 = encoder push switch held */
volatile uint32_t encoder_invalid = 0;  /* simultaneous A/B transitions */

static inline uint8_t Numpad_EncoderAB(void)
{
    uint32_t pins = GPIOA->IDR;
    return (uint8_t)(((pins & GPIO_PIN_0) ? 2U : 0U) |
                     ((pins & GPIO_PIN_1) ? 1U : 0U));
}

static inline void BoardTest_Task(void)
{
    static uint8_t initialized = 0, row = 0, previous_ab = 0;
    static uint8_t stable_frames = 0, switch_ms = 0;
    static uint16_t frame = 0, candidate = 0;
    static uint32_t last_ms = 0, activity_ms = 0;
    static uint8_t activity = 0;
    static const int8_t transition[16] = {
         0, -1,  1,  0,  1,  0,  0, -1,
        -1,  0,  0,  1,  0,  1, -1,  0
    };
    uint32_t now = HAL_GetTick();
    if (!initialized) {
        GPIO_InitTypeDef gpio = {0};
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /* Open-drain inactive rows release the bus instead of driving high. */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3,
                          GPIO_PIN_SET);
        gpio.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
        gpio.Mode = GPIO_MODE_OUTPUT_OD;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOC, &gpio);
        gpio.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOB, &gpio);
        gpio.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4;
        HAL_GPIO_Init(GPIOA, &gpio);
        previous_ab = Numpad_EncoderAB();
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        last_ms = now;
        initialized = 1;
        return;
    }
    if ((uint32_t)(now - last_ms) < 1U) return;
    /* A missed polling interval must not count as several debounce samples. */
    last_ms = now;

    uint8_t ab = Numpad_EncoderAB();
    if (ab != previous_ab) {
        int8_t step = transition[(previous_ab << 2) | ab];
        if ((previous_ab ^ ab) == 3U) ++encoder_invalid;
        encoder_ticks += step;
        previous_ab = ab;
        if (step) { activity_ms = now; activity = 1; }
    }
    uint8_t pressed = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET;
    if (pressed != encoder_button) {
        if (++switch_ms >= 20U) { encoder_button = pressed; switch_ms = 0; }
    } else switch_ms = 0;

    uint16_t columns = (uint16_t)((~GPIOB->IDR >> 4) & 0xFU);
    uint16_t row_mask = (uint16_t)(0xFU << (row * 4U));
    frame = (uint16_t)((frame & (uint16_t)~row_mask) | (columns << (row * 4U)));
    HAL_GPIO_WritePin(GPIOC, (uint16_t)(GPIO_PIN_0 << row), GPIO_PIN_SET);
    row = (uint8_t)((row + 1U) & 3U);
    HAL_GPIO_WritePin(GPIOC, (uint16_t)(GPIO_PIN_0 << row), GPIO_PIN_RESET);
    if (row == 0U) {
        frame &= 0x7FFFU; /* row 4 / column 4 is the encoder position */
        if (frame != candidate) { candidate = frame; stable_frames = 0; }
        else if (stable_frames < 3U && ++stable_frames == 3U) {
            uint16_t rising = (uint16_t)(candidate & (uint16_t)~numpad_keys);
            while (rising) { key_press_count += rising & 1U; rising >>= 1; }
            numpad_keys = candidate;
        }
    }
    if (activity && (uint32_t)(now - activity_ms) >= 250U) activity = 0;
    uint8_t user = HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin)
                   == GPIO_PIN_RESET;
    uint8_t on = numpad_keys || encoder_button || activity || user ||
                 ((now / 500U) & 1U);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
#endif
