/* Hardware-layer test for matrix_hw.c, using an ELECTRICAL MODEL of the matrix.
 *
 * The model behaves like the real board: a column pin reads LOW only if some
 * row is pulled low AND the key at that row/column is closed (the diode lets
 * current flow column -> row only). Otherwise the pull-up makes it read HIGH.
 * That lets us check, without a board, that matrix_hw.c:
 *   - maps ROW1..4 to PC0..PC3 and COL1..4 to PB4..PB7,
 *   - pulls only ONE row low at a time and leaves all rows released afterwards,
 *   - releases the rows BEFORE switching them to outputs (no startup glitch),
 *   - waits at least MATRIX_SETTLE_US before reading the columns,
 *   - reports every one of the 65536 possible key patterns correctly.
 *
 * Exit code 0 = pass; otherwise the failing source line number.
 */
#include "key_matrix.h"
#include "key_clock.h"

GPIO_TypeDef fake_gpio_a, fake_gpio_b, fake_gpio_c;

static uint32_t fake_us;           /* advances by 1 us on every clock read */
static uint16_t keys_closed;       /* the key pattern being simulated */
static uint8_t row_latch[4];       /* 1 = released, 0 = pulled low (open-drain latch) */
static uint8_t rows_ever_low_together; /* set if two rows were ever low at once */
static uint8_t latch_ok_before_init;   /* all rows released when GPIOC was switched to output */
static uint32_t row_init_pins, col_init_pins, row_init_mode, col_init_mode, col_init_pull;
static uint32_t low_since_us[4];   /* when each row was last pulled low */
static uint32_t shortest_settle_us = 0xFFFFFFFFU;
static uint8_t read_before_row_low; /* a column was read with no row low */

uint32_t KeyClock_NowUs(void) { return fake_us++; }

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    if (port == GPIOC)
    {
        row_init_pins = init->Pin;
        row_init_mode = init->Mode;
        latch_ok_before_init = row_latch[0] && row_latch[1] && row_latch[2] && row_latch[3];
    }
    else if (port == GPIOB)
    {
        col_init_pins = init->Pin;
        col_init_mode = init->Mode;
        col_init_pull = init->Pull;
    }
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pins, GPIO_PinState state)
{
    if (port != GPIOC)
        return;
    for (unsigned r = 0; r < 4U; r++)
    {
        if (pins & (1U << r))
        {
            row_latch[r] = state == GPIO_PIN_SET;
            if (state == GPIO_PIN_RESET)
                low_since_us[r] = fake_us;
        }
    }
    unsigned low = 0;
    for (unsigned r = 0; r < 4U; r++)
        low += row_latch[r] ? 0U : 1U;
    if (low > 1U)
        rows_ever_low_together = 1;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    if (port != GPIOB)
        return GPIO_PIN_SET;
    unsigned col = 0;
    while (col < 4U && pin != (uint16_t)(GPIO_PIN_4 << col))
        col++;
    unsigned lows = 0;
    for (unsigned r = 0; r < 4U; r++)
    {
        if (row_latch[r])
            continue;
        lows++;
        uint32_t settled = fake_us - low_since_us[r];
        if (settled < shortest_settle_us)
            shortest_settle_us = settled;
        if (keys_closed & (1U << (r * 4U + col)))
            return GPIO_PIN_RESET; /* closed key + low row pulls the column low */
    }
    if (lows == 0U)
        read_before_row_low = 1;
    return GPIO_PIN_SET; /* internal pull-up */
}

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

int main(void)
{
    /* Before init the rows are floating latches (reset state = released). */
    for (unsigned r = 0; r < 4U; r++)
        row_latch[r] = 1;

    Matrix_GpioInit();
    CHECK(row_init_pins == 0x0FU && row_init_mode == GPIO_MODE_OUTPUT_OD); /* PC0..PC3 open-drain */
    CHECK(col_init_pins == 0xF0U && col_init_mode == GPIO_MODE_INPUT);     /* PB4..PB7 inputs */
    CHECK(col_init_pull == GPIO_PULLUP);
    CHECK(latch_ok_before_init == 1U);
    CHECK(row_latch[0] && row_latch[1] && row_latch[2] && row_latch[3]);

    /* Every possible pattern of the 16 positions must read back exactly. */
    for (uint32_t pattern = 0; pattern < 65536U; pattern++)
    {
        keys_closed = (uint16_t)pattern;
        CHECK(Matrix_ReadRaw() == (uint16_t)pattern);
        CHECK(row_latch[0] && row_latch[1] && row_latch[2] && row_latch[3]);
    }
    CHECK(rows_ever_low_together == 0U);
    CHECK(read_before_row_low == 0U);
    CHECK(shortest_settle_us >= MATRIX_SETTLE_US);

    /* Wiring spot checks against the documented pins:
     * ROW1/COL1 = key 0, ROW2/COL2 = key 5, ROW4/COL3 = key 14. */
    keys_closed = 1U << 0;
    CHECK(Matrix_ReadRaw() == 0x0001U);
    keys_closed = 1U << 5;
    CHECK(Matrix_ReadRaw() == 0x0020U);
    keys_closed = 1U << 14;
    CHECK(Matrix_ReadRaw() == 0x4000U);
    return 0;
}
