/* Logic test for the 4 x 4 matrix firmware (key_matrix.c).
 *
 * The REAL key_matrix.c is compiled and driven with fake time, fake key
 * contacts and a fake USB endpoint. Nothing here talks to hardware.
 * Time advances in exact 125 us steps, the same as the 8K scan period.
 *
 * The program returns 0 when everything passes. On failure the exit code is
 * the source line number of the failing CHECK, so open this file at that line.
 * No C runtime functions are used (the test is linked without the CRT).
 */
#include "key_matrix.h"
#include "key_clock.h"
#include "keyboard.h"

GPIO_TypeDef fake_gpio_a, fake_gpio_b, fake_gpio_c;
USBD_HandleTypeDef usb_device;
volatile uint32_t keyboard_epoch;
extern volatile uint32_t key_scan_count, key_scan_missed_periods;

/* ---- fakes ---- */
static uint32_t now_us;      /* the fake microsecond clock */
static uint16_t closed;      /* which key contacts are closed right now */
static uint32_t read_calls;  /* how many times the matrix was read */
static uint8_t blocked;      /* 1 = USB endpoint busy, refuses reports */
static uint8_t sent[1024][8];
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
uint16_t Matrix_ReadRaw(void) { read_calls++; return closed; }
void Matrix_GpioInit(void) {}
uint8_t Keyboard_SendReport(const uint8_t r[8])
{
    if (blocked || sent_count >= 1024U)
        return 0;
    for (unsigned i = 0; i < 8U; i++)
        sent[sent_count][i] = r[i];
    sent_count++;
    return 1;
}

/* ---- helpers ---- */
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define KEY(row, col) ((uint16_t)(1U << ((row) * 4U + (col))))
static void ticks(unsigned n)
{
    while (n--)
    {
        now_us += 125U;
        Keyboard_Task();
    }
}
static const uint8_t *last(void) { return sent[sent_count - 1U]; }
/* 1 if report r lists exactly the given usages (in order) and no others. */
static int keys_are(const uint8_t *r, unsigned n, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
{
    const uint8_t want[6] = {a, b, c, d, e, f};
    for (unsigned i = 0; i < 6U; i++)
        if ((unsigned)r[2U + i] != (i < n ? (unsigned)want[i] : 0U))
            return 0;
    return r[0] == 0U && r[1] == 0U;
}
static int all_up(const uint8_t *r) { return keys_are(r, 0, 0, 0, 0, 0, 0, 0); }
#define RELEASE_WAIT 82U /* a bit more than 10 ms of 125 us scans */

int main(void)
{
    /* 1. Startup: once USB is configured, the first report is "all keys up". */
    usb_device.dev_state = USBD_STATE_CONFIGURED;
    Keyboard_Task();
    ticks(100);
    CHECK(sent_count == 1U && all_up(last()));

    /* 2. Every populated key, alone: exactly one press report and one release. */
    for (unsigned i = 0; i < 15U; i++)
    {
        unsigned before = sent_count;
        closed = (uint16_t)(1U << i);
        ticks(1);
        CHECK(sent_count == before + 1U);
        CHECK(keys_are(last(), 1, (uint8_t)(0x04U + i), 0, 0, 0, 0, 0));
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(sent_count == before + 2U && all_up(last()));
    }

    /* 3. The empty position (ROW4/COL4, where the encoder sits) is ignored. */
    {
        unsigned before = sent_count;
        closed = KEY(3, 3);
        ticks(100);
        closed = 0;
        ticks(100);
        CHECK(sent_count == before);
    }

    /* 4. Two keys pressed in the same scan give ONE report with both, in
     * matrix order; keys pressed at different times keep PRESS order.
     */
    {
        unsigned before = sent_count;
        closed = KEY(0, 0) | KEY(0, 1) | KEY(0, 2);
        ticks(1);
        CHECK(sent_count == before + 1U && keys_are(last(), 3, 0x04, 0x05, 0x06, 0, 0, 0));
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(sent_count == before + 2U && all_up(last()));

        closed = KEY(0, 2);
        ticks(1);
        closed = KEY(0, 2) | KEY(0, 0);
        ticks(1);
        CHECK(keys_are(last(), 2, 0x06, 0x04, 0, 0, 0, 0)); /* c then a */
        closed = KEY(0, 0); /* release c only: a remains and moves to slot 0 */
        ticks(RELEASE_WAIT);
        CHECK(keys_are(last(), 1, 0x04, 0, 0, 0, 0, 0));
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(all_up(last()));
    }

    /* 5. Contact bounce: one press; release only after 10 ms continuously open. */
    {
        unsigned before;
        closed = KEY(1, 0);
        ticks(1);
        before = sent_count;
        CHECK(keys_are(last(), 1, 0x08, 0, 0, 0, 0, 0));
        for (unsigned n = 0; n < 12U; n++)
        {
            closed = 0;
            ticks(1);
            closed = KEY(1, 0);
            ticks(1);
        }
        CHECK(sent_count == before); /* bounce produced nothing */
        closed = 0;
        ticks(80);
        CHECK(sent_count == before); /* 9.875 ms open: not yet released */
        ticks(1);
        CHECK(sent_count == before + 1U && all_up(last()));
    }

    /* 6. Six-key limit: a 7th simultaneous key is ignored and counted. */
    {
        unsigned before;
        closed = 0;
        for (unsigned i = 0; i < 7U; i++)
        {
            closed |= (uint16_t)(1U << i);
            ticks(1);
        }
        CHECK(keys_are(last(), 6, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09));
        CHECK(matrix_rollover_dropped == 1U);
        before = sent_count;
        closed &= (uint16_t)~KEY(1, 2); /* release the ignored 7th key (0x0A) */
        ticks(RELEASE_WAIT);
        CHECK(sent_count == before); /* releasing an unreported key sends nothing */
        closed &= (uint16_t)~KEY(0, 0); /* release the first key */
        ticks(RELEASE_WAIT);
        CHECK(keys_are(last(), 5, 0x05, 0x06, 0x07, 0x08, 0x09, 0));
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(all_up(last()));
    }

    /* 7. Busy USB: a quick tap of two different keys keeps every press and
     * release, in order.
     */
    {
        unsigned before = sent_count;
        blocked = 1;
        closed = KEY(0, 0);
        ticks(1);
        closed = 0;
        ticks(RELEASE_WAIT);
        closed = KEY(0, 1);
        ticks(1);
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(sent_count == before);
        blocked = 0;
        ticks(6);
        CHECK(sent_count == before + 4U);
        CHECK(keys_are(sent[before], 1, 0x04, 0, 0, 0, 0, 0));
        CHECK(all_up(sent[before + 1U]));
        CHECK(keys_are(sent[before + 2U], 1, 0x05, 0, 0, 0, 0, 0));
        CHECK(all_up(sent[before + 3U]));
    }

    /* 8. Keys held while USB reconnects are silent until released; other keys
     * still work meanwhile; a fresh press of a held key works after release.
     */
    {
        unsigned before;
        usb_device.dev_state = 0;
        closed = KEY(0, 0) | KEY(1, 1);
        ticks(3);
        usb_device.dev_state = USBD_STATE_CONFIGURED;
        keyboard_epoch++;
        ticks(100);
        before = sent_count;
        CHECK(all_up(last())); /* resync report, no held key */
        closed |= KEY(2, 1); /* a third key pressed while the others are held */
        ticks(1);
        CHECK(sent_count == before + 1U && keys_are(last(), 1, 0x0D, 0, 0, 0, 0, 0));
        closed &= (uint16_t)~KEY(2, 1);
        ticks(RELEASE_WAIT);
        CHECK(all_up(last()));
        before = sent_count;
        closed = KEY(1, 1); /* release the first held key, keep the second held */
        ticks(RELEASE_WAIT);
        CHECK(sent_count == before); /* silent: it was never reported */
        closed = KEY(1, 1) | KEY(0, 0); /* press it again: now it counts */
        ticks(1);
        CHECK(keys_are(last(), 1, 0x04, 0, 0, 0, 0, 0));
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(all_up(last()));
    }

    /* 9. Queue overflow with a stalled USB endpoint: recover with all keys up,
     * and ignore a key that was still held when it happened.
     */
    {
        closed = KEY(0, 3);
        ticks(1);
        CHECK(keys_are(last(), 1, 0x07, 0, 0, 0, 0, 0));
        blocked = 1;
        for (unsigned n = 0; n < 20U; n++)
        {
            closed = KEY(0, 3) | KEY(0, 0);
            ticks(1);
            closed = KEY(0, 3);
            ticks(RELEASE_WAIT);
        }
        CHECK(key_queue_overflow > 0U);
        blocked = 0;
        ticks(100);
        CHECK(all_up(last())); /* nothing left stuck down */
        closed = 0;
        ticks(RELEASE_WAIT);
        closed = KEY(0, 3);
        ticks(1);
        CHECK(keys_are(last(), 1, 0x07, 0, 0, 0, 0, 0)); /* usable again */
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(all_up(last()));
    }

    /* 10. The 32-bit microsecond clock wraps: debounce must survive it. */
    {
        usb_device.dev_state = 0;
        now_us = 0xFFFFE000U;
        Keyboard_Task();
        ticks(100);
        usb_device.dev_state = USBD_STATE_CONFIGURED;
        keyboard_epoch++;
        ticks(100);
        closed = KEY(0, 0);
        ticks(1);
        CHECK(keys_are(last(), 1, 0x04, 0, 0, 0, 0, 0));
        now_us = 0xFFFFFF00U;
        Keyboard_Task();
        closed = 0;
        ticks(RELEASE_WAIT);
        CHECK(all_up(last()));
    }

    /* 11. Cadence: one full-matrix read per 125 us, none between periods. */
    {
        uint32_t scans = key_scan_count;
        uint32_t reads = read_calls;
        ticks(8000); /* one nominal second */
        CHECK(key_scan_count - scans == 8000U);
        CHECK(read_calls - reads == 8000U);
        scans = key_scan_count;
        Keyboard_Task(); /* no time passed: must not scan */
        CHECK(key_scan_count == scans);
    }

    /* 12. Report builder: modifiers, repeats, ignored zero, overflow result. */
    {
        uint8_t r[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        CHECK(Matrix_ReportApply(r, 0xE1, 1) == 1U && r[0] == 0x02U); /* left shift */
        CHECK(Matrix_ReportApply(r, 0xE0, 1) == 1U && r[0] == 0x03U); /* + left ctrl */
        CHECK(Matrix_ReportApply(r, 0xE1, 0) == 1U && r[0] == 0x01U);
        CHECK(Matrix_ReportApply(r, 0, 1) == 1U && r[2] == 0U);
        CHECK(Matrix_ReportApply(r, 0x04, 1) == 1U && Matrix_ReportApply(r, 0x04, 1) == 1U);
        CHECK(r[2] == 0x04U && r[3] == 0U); /* a repeated press adds no duplicate */
        for (uint8_t u = 0x05; u <= 0x09; u++)
            CHECK(Matrix_ReportApply(r, u, 1) == 1U);
        CHECK(Matrix_ReportApply(r, 0x0A, 1) == 0U); /* seventh key dropped */
        CHECK(Matrix_ReportApply(r, 0x0A, 0) == 1U); /* releasing it is harmless */
    }

    /* 13. The keymap: 15 unique, non-zero usages; the encoder cell is empty. */
    {
        unsigned used = 0;
        for (unsigned i = 0; i < MATRIX_KEYS; i++)
        {
            if (matrix_keymap[i] == 0U)
                continue;
            used++;
            for (unsigned j = i + 1U; j < MATRIX_KEYS; j++)
                CHECK(matrix_keymap[i] != matrix_keymap[j]);
        }
        CHECK(used == 15U && matrix_keymap[15] == 0U);
    }
    return 0;
}
