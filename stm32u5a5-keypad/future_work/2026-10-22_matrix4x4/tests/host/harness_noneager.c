/* Conservative-mode test: KEY_EAGER_PRESS = 0 (both press and release need
 * a full 10 ms of steady contact).
 *
 * The run script builds this against a COPY of board_config.h with eager press
 * switched off (the real file is never edited). It checks the two behaviours
 * that only matter in this mode:
 *   1. a press is delayed by the 10 ms filter, and a short noise pulse is ignored;
 *   2. a key that is still being debounced when USB connects does NOT type a
 *      stray character afterwards (the held-at-connect suppression).
 * Exit code 0 = pass; otherwise the failing source line number.
 */
#include "key_matrix.h"
#include "key_clock.h"
#include "keyboard.h"

GPIO_TypeDef fake_gpio_a, fake_gpio_b, fake_gpio_c;
USBD_HandleTypeDef usb_device;
volatile uint32_t keyboard_epoch;

static uint32_t now_us;
static uint16_t closed;
static uint8_t sent[256][8];
static unsigned sent_count;

uint32_t KeyClock_NowUs(void) { return now_us; }
uint32_t HAL_GetTick(void) { return now_us / 1000U; }
void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *i) { (void)p; (void)i; }
void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint16_t n, GPIO_PinState s) { (void)p; (void)n; (void)s; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *p, uint16_t n) { (void)p; (void)n; return GPIO_PIN_SET; }
uint32_t __get_PRIMASK(void) { return 0; }
void __disable_irq(void) {}
void __set_PRIMASK(uint32_t m) { (void)m; }
void Keyboard_Idle(void) {}
uint16_t Matrix_ReadRaw(void) { return closed; }
void Matrix_GpioInit(void) {}
uint8_t Keyboard_SendReport(const uint8_t r[8])
{
    if (sent_count >= 256U)
        return 0;
    for (unsigned i = 0; i < 8U; i++)
        sent[sent_count][i] = r[i];
    sent_count++;
    return 1;
}

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
static void ticks(unsigned n)
{
    while (n--)
    {
        now_us += 125U;
        Keyboard_Task();
    }
}

int main(void)
{
    usb_device.dev_state = USBD_STATE_CONFIGURED;
    Keyboard_Task();
    ticks(100);
    CHECK(sent_count == 1U); /* startup all-up report */

    /* A 5 ms noise pulse never becomes a key. */
    closed = 1U << 0;
    ticks(40);
    closed = 0;
    ticks(100);
    CHECK(sent_count == 1U);

    /* A steady press is reported only after 10 ms. */
    closed = 1U << 0;
    ticks(80);
    CHECK(sent_count == 1U);
    ticks(1);
    CHECK(sent_count == 2U && sent[1][2] == 0x04U);
    closed = 0;
    ticks(100);
    CHECK(sent_count == 3U && sent[2][2] == 0U);

    /* Key goes down while USB is disconnected, then USB connects before the
     * 10 ms filter finishes. When the filter completes the key must stay silent.
     */
    usb_device.dev_state = 0;
    closed = 1U << 1;
    ticks(2);
    usb_device.dev_state = USBD_STATE_CONFIGURED;
    keyboard_epoch++;
    ticks(200);
    {
        unsigned after_sync = sent_count;
        CHECK(sent[after_sync - 1U][2] == 0U); /* no 'b' ever sent */
        for (unsigned i = 0; i < sent_count; i++)
            CHECK(sent[i][2] != 0x05U);
    }
    closed = 0;
    ticks(100);
    closed = 1U << 1;
    ticks(81);
    CHECK(sent[sent_count - 1U][2] == 0x05U); /* a fresh press works */
    return 0;
}
