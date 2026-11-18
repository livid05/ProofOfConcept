/* matrix_hw.c - GPIO side of the 4 x 4 key matrix.
 * FUTURE WORK, NOT YET PART OF THE BUILD.
 *
 * Only this file touches the row and column pins. Everything else works on
 * the 16-bit mask that Matrix_ReadRaw() returns, which is what lets the
 * debounce and report logic be tested on a PC.
 */
#include "key_matrix.h"
#include "key_clock.h"

static GPIO_TypeDef *const row_port = MATRIX_ROW_PORT;
static GPIO_TypeDef *const col_port = MATRIX_COL_PORT;
static const uint16_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const uint16_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

void Matrix_GpioInit(void)
{
    GPIO_InitTypeDef g = {0};
    uint32_t row_mask = 0;
    uint32_t col_mask = 0;
    for (uint32_t r = 0; r < MATRIX_ROWS; r++)
        row_mask |= row_pins[r];
    for (uint32_t c = 0; c < MATRIX_COLS; c++)
        col_mask |= col_pins[c];

    /* Write "released" (1) to the output register BEFORE switching the pins to
     * outputs, so no row flickers low for an instant at startup.
     * Open-drain output: writing 1 releases the pin (floating), writing 0
     * pulls it to ground. The pin never drives itself high.
     */
    HAL_GPIO_WritePin(row_port, (uint16_t)row_mask, GPIO_PIN_SET);
    g.Pin = row_mask;
    g.Mode = GPIO_MODE_OUTPUT_OD;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(row_port, &g);

    /* Columns: plain inputs with the internal pull-up, so an open key reads 1. */
    g.Pin = col_mask;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(col_port, &g);
}

uint16_t Matrix_ReadRaw(void)
{
    uint16_t closed = 0;
    for (uint32_t r = 0; r < MATRIX_ROWS; r++)
    {
        /* Pull exactly this one row to ground; all other rows stay released. */
        HAL_GPIO_WritePin(row_port, row_pins[r], GPIO_PIN_RESET);

        /* Wait for the columns to settle (see MATRIX_SETTLE_US). The unsigned
         * subtraction stays correct when the 32-bit microsecond clock wraps.
         */
        uint32_t start_us = KeyClock_NowUs();
        while ((uint32_t)(KeyClock_NowUs() - start_us) < MATRIX_SETTLE_US)
        {
        }

        /* A column reading LOW means a closed key connects it to this row. */
        for (uint32_t c = 0; c < MATRIX_COLS; c++)
        {
            if (HAL_GPIO_ReadPin(col_port, col_pins[c]) == GPIO_PIN_RESET)
                closed |= (uint16_t)(1U << (r * MATRIX_COLS + c));
        }

        /* Release the row again before selecting the next one. */
        HAL_GPIO_WritePin(row_port, row_pins[r], GPIO_PIN_SET);
    }
    return closed;
}
