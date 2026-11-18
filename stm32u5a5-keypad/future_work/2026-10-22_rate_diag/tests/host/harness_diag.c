/* PC test of rate_diag.c (the 8K rate-test report streamer).
 * Uses the project's fake HAL/keyboard headers (tests/host/fakes), read-only.
 * Exit code 0 = pass; otherwise the failing source line number.
 */
#include "keyboard.h"
#include "key_clock.h"
#include "rate_diag.h"

GPIO_TypeDef fake_gpio_a, fake_gpio_b, fake_gpio_c;
USBD_HandleTypeDef usb_device;
volatile uint32_t keyboard_epoch;

static uint32_t now_us;
static uint8_t busy;
static uint8_t last_report[8];
static unsigned sent_count;

uint32_t KeyClock_NowUs(void) { return now_us; }
uint32_t HAL_GetTick(void) { return now_us / 1000U; }
void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *i) { (void)p; (void)i; }
void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint16_t n, GPIO_PinState s) { (void)p; (void)n; (void)s; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *p, uint16_t n) { (void)p; (void)n; return GPIO_PIN_SET; }
uint32_t __get_PRIMASK(void) { return 0; }
void __disable_irq(void) {}
void __set_PRIMASK(uint32_t m) { (void)m; }
uint8_t Keyboard_SendReport(const uint8_t r[8])
{
    if (busy)
        return 0;
    for (unsigned i = 0; i < 8U; i++)
        last_report[i] = r[i];
    sent_count++;
    return 1;
}

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
static uint32_t counter_of(const uint8_t *r) { return (uint32_t)r[0] | ((uint32_t)r[1] << 8) | ((uint32_t)r[2] << 16) | ((uint32_t)r[3] << 24); }
static uint32_t time_of(const uint8_t *r) { return (uint32_t)r[4] | ((uint32_t)r[5] << 8) | ((uint32_t)r[6] << 16) | ((uint32_t)r[7] << 24); }

int main(void)
{
    /* Little-endian layout. */
    uint8_t r[8];
    Diag_FillReport(r, 0x04030201U, 0x08070605U);
    CHECK(r[0] == 1 && r[3] == 4 && r[4] == 5 && r[7] == 8);

    /* Nothing is sent before USB is configured. */
    now_us = 1000;
    Keyboard_Task();
    CHECK(sent_count == 0U);

    /* Once configured, every free endpoint gets one report; counter counts 0,1,2... */
    usb_device.dev_state = USBD_STATE_CONFIGURED;
    for (unsigned i = 0; i < 5U; i++)
    {
        now_us = 2000U + 125U * i;
        Keyboard_Task();
        CHECK(sent_count == i + 1U);
        CHECK(counter_of(last_report) == i);
        CHECK(time_of(last_report) == 2000U + 125U * i);
    }

    /* A busy endpoint is retried without skipping a counter value. */
    busy = 1;
    now_us += 125;
    Keyboard_Task();
    Keyboard_Task();
    CHECK(sent_count == 5U && diag_busy_refusals == 2U);
    busy = 0;
    now_us += 125;
    Keyboard_Task();
    CHECK(sent_count == 6U && counter_of(last_report) == 5U);

    /* The 32-bit device clock wrapping is just data in the report. */
    now_us = 0xFFFFFFF0U;
    Keyboard_Task();
    CHECK(time_of(last_report) == 0xFFFFFFF0U && counter_of(last_report) == 6U);
    return 0;
}
