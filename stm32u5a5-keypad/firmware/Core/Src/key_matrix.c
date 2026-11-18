/* key_matrix.c - 4 x 4 matrix input, replacing key_switch.c when integrated.
 * FUTURE WORK, NOT YET PART OF THE BUILD. See INTEGRATION.md.
 *
 * Data path:
 *   Matrix_ReadRaw (every 125 us) -> per-key eager press / filtered release
 *   -> 8-byte keyboard report -> 15-deep queue -> Keyboard_SendReport -> USB.
 *
 * It keeps the safety rules of the one-key version:
 *   - Every press and release must reach the host, in order. A tap that
 *     starts and ends while USB is busy is queued, never merged away.
 *   - A key held while USB (re)connects is ignored until released, so a plug-in
 *     never types a stray character.
 *   - If the queue ever overflows, send "all keys up" instead of leaving a key
 *     stuck down, and ignore the keys still held until they are released.
 *
 * No CRT functions (memcpy, memcmp) are used so the host test can link
 * without the C runtime; small loops copy the 8-byte reports instead.
 */
#include "key_matrix.h"
#include "key_clock.h"
#include "keyboard.h"

const uint8_t matrix_keymap[MATRIX_KEYS] = MATRIX_KEYMAP_INIT;

/* Counters declared in keyboard.h. They count debounced events, not proof that
 * a character appeared on screen. usb_report_count includes the startup
 * "all keys up" synchronization report.
 */
volatile uint32_t key_press_count, usb_report_count, key_queue_overflow;
volatile uint8_t key_pressed; /* 1 while any key is down */
volatile uint32_t key_scan_count, key_scan_missed_periods, key_scan_max_gap_us;
volatile uint32_t matrix_scan_last_us, matrix_scan_max_us, matrix_rollover_dropped;
volatile uint16_t matrix_pressed_mask;
void Keyboard_Idle(void);

/* Queue of finished 8-byte reports waiting for the USB endpoint. 16 slots in a
 * ring hold 15 reports (one slot stays empty to tell full from empty). The
 * storage is static because USB sends asynchronously from an interrupt.
 */
#define QUEUE_SIZE 16U
static uint8_t queue[QUEUE_SIZE][8];
static uint8_t head, tail, sync_pending, was_configured, clock_started;

/* The report as the host should see it right now, and the last one queued.
 * They differ only briefly, inside one pass of Keyboard_Task.
 */
static uint8_t report_now[8], report_queued[8];

/* One bit per matrix position (bit = row * 4 + col):
 *   raw_mask        contact state at the latest sample (1 = closed)
 *   stable_mask     debounced state (1 = key counted as pressed)
 *   suppressed_mask keys held when USB connected; ignored until released
 *   populated_mask  positions that have a key (keymap entry not zero)
 */
static uint16_t raw_mask, stable_mask, suppressed_mask, populated_mask;
static uint32_t changed_us[MATRIX_KEYS]; /* time each key's raw state last changed */
static uint32_t last_sample_us, last_observed_scan_us, observed_usb_epoch;

static void clear_report(uint8_t r[8])
{
    for (uint32_t i = 0; i < 8U; i++)
        r[i] = 0;
}

static void copy_report(uint8_t to[8], const uint8_t from[8])
{
    for (uint32_t i = 0; i < 8U; i++)
        to[i] = from[i];
}

static uint8_t same_report(const uint8_t a[8], const uint8_t b[8])
{
    for (uint32_t i = 0; i < 8U; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

uint8_t Matrix_ReportApply(uint8_t report[8], uint8_t usage, uint8_t pressed)
{
    if (usage == 0U)
        return 1;

    /* Modifier usages 0xE0..0xE7 are bits in byte 0, not key slots. */
    if (usage >= 0xE0U && usage <= 0xE7U)
    {
        uint8_t bit = (uint8_t)(1U << (usage - 0xE0U));
        if (pressed)
            report[0] |= bit;
        else
            report[0] = (uint8_t)(report[0] & (uint8_t)~bit);
        return 1;
    }

    /* Ordinary keys live in report[2..7], kept in the order pressed. */
    uint8_t *keys = &report[2];
    uint32_t found = MATRIX_REPORT_KEYS;
    for (uint32_t i = 0; i < MATRIX_REPORT_KEYS; i++)
    {
        if (keys[i] == usage)
        {
            found = i;
            break;
        }
    }

    if (pressed)
    {
        if (found < MATRIX_REPORT_KEYS)
            return 1; /* already listed */
        for (uint32_t i = 0; i < MATRIX_REPORT_KEYS; i++)
        {
            if (keys[i] == 0U)
            {
                keys[i] = usage;
                return 1;
            }
        }
        return 0; /* all six slots busy: this key is ignored */
    }

    if (found < MATRIX_REPORT_KEYS)
    {
        /* Remove it and close the gap so the remaining keys keep their order. */
        for (uint32_t i = found; i + 1U < MATRIX_REPORT_KEYS; i++)
            keys[i] = keys[i + 1U];
        keys[MATRIX_REPORT_KEYS - 1U] = 0;
    }
    return 1;
}

/* Reads the matrix once and updates every key's debounce state.
 * now_us is the time this scan started. Any press or release changes
 * report_now; the caller queues it if it differs from report_queued.
 */
static void sample_matrix(uint32_t now_us)
{
    uint32_t scan_start_us = KeyClock_NowUs();
    uint16_t raw = (uint16_t)(Matrix_ReadRaw() & populated_mask);
    uint32_t scan_us = KeyClock_NowUs() - scan_start_us;
    matrix_scan_last_us = scan_us;
    if (scan_us > matrix_scan_max_us)
        matrix_scan_max_us = scan_us;

    /* Remember WHEN each key's contact last changed. Contact bounce keeps
     * restarting this clock, so a release is only accepted after the contact
     * has stayed open for the full debounce time.
     */
    uint16_t raw_changed = (uint16_t)(raw ^ raw_mask);
    for (uint32_t i = 0; i < MATRIX_KEYS; i++)
    {
        if (raw_changed & (1U << i))
            changed_us[i] = now_us;
    }
    raw_mask = raw;

    for (uint32_t i = 0; i < MATRIX_KEYS; i++)
    {
        uint16_t bit = (uint16_t)(1U << i);
        if (!((raw ^ stable_mask) & bit))
            continue; /* contact agrees with the accepted state */

        /* Eager press: the first closed sample counts at once. Release (and
         * press when eager mode is off) needs KEY_DEBOUNCE_US of steadiness.
         */
        if (!((KEY_EAGER_PRESS && (raw & bit)) ||
              (uint32_t)(now_us - changed_us[i]) >= KEY_DEBOUNCE_US))
            continue;

        stable_mask ^= bit;
        if (stable_mask & bit)
        {
            key_press_count++;
            if (!(suppressed_mask & bit) &&
                !Matrix_ReportApply(report_now, matrix_keymap[i], 1))
                matrix_rollover_dropped++;
        }
        else if (suppressed_mask & bit)
        {
            suppressed_mask &= (uint16_t)~bit; /* held-at-connect key finally let go */
        }
        else
        {
            (void)Matrix_ReportApply(report_now, matrix_keymap[i], 0);
        }
    }
    key_pressed = stable_mask != 0U;
    matrix_pressed_mask = stable_mask;
}

void Keyboard_Task(void)
{
    uint32_t now_us = KeyClock_NowUs();
    if (!clock_started)
    {
        /* Start observing now, not at the earlier start of USB initialization. */
        clock_started = 1;
        last_sample_us = last_observed_scan_us = now_us;
        for (uint32_t i = 0; i < MATRIX_KEYS; i++)
        {
            changed_us[i] = now_us;
            if (matrix_keymap[i] != 0U)
                populated_mask |= (uint16_t)(1U << i);
        }
    }

    uint32_t elapsed_us = now_us - last_sample_us;
    if (elapsed_us >= KEY_SCAN_PERIOD_US)
    {
        key_scan_count++;
        uint32_t actual_gap_us = now_us - last_observed_scan_us;
        last_observed_scan_us = now_us;
        if (actual_gap_us > key_scan_max_gap_us)
            key_scan_max_gap_us = actual_gap_us;
        /* Keep the schedule's phase when a scan runs late. Never invent
         * catch-up samples of old switch states: read once, count the missed
         * slots, and move on.
         */
        uint32_t periods = elapsed_us / KEY_SCAN_PERIOD_US;
        key_scan_missed_periods += periods - 1U;
        last_sample_us += periods * KEY_SCAN_PERIOD_US;
        sample_matrix(now_us);
    }

    /* USB state changes in an interrupt, so mask interrupts only while checking
     * it and starting a nonblocking transfer. PRIMASK is restored afterwards so
     * the caller's previous interrupt state is preserved.
     */
    uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    uint8_t configured = usb_device.dev_state == USBD_STATE_CONFIGURED;
    if (!configured || observed_usb_epoch != keyboard_epoch || !was_configured)
    {
        /* New or lost connection: drop stale events, report "all keys up" first,
         * and ignore every key that is down right now until it is released.
         */
        observed_usb_epoch = keyboard_epoch;
        head = tail = 0;
        clear_report(report_now);
        clear_report(report_queued);
        sync_pending = 1;
        suppressed_mask = (uint16_t)(stable_mask | raw_mask);
        was_configured = configured;
    }

    if (configured)
    {
        if (!same_report(report_now, report_queued))
        {
            uint8_t next_head = (uint8_t)((head + 1U) % QUEUE_SIZE);
            if (next_head == tail)
            {
                /* Queue full: a stalled connection. Recover with all keys up
                 * rather than risk a stuck key. This change is discarded and
                 * keys still held stay ignored until released.
                 */
                key_queue_overflow++;
                head = tail = 0;
                clear_report(report_now);
                clear_report(report_queued);
                sync_pending = 1;
                suppressed_mask = (uint16_t)(stable_mask | raw_mask);
            }
            else
            {
                copy_report(queue[head], report_now);
                copy_report(report_queued, report_now);
                head = next_head;
            }
        }

        /* Only advance the queue when USB accepts a report. A busy endpoint
         * means "try again next loop", even between scans.
         */
        if (sync_pending)
        {
            uint8_t zero[8];
            clear_report(zero);
            if (Keyboard_SendReport(zero))
            {
                sync_pending = 0;
                usb_report_count++;
            }
        }
        else if (head != tail)
        {
            if (Keyboard_SendReport(queue[tail]))
            {
                tail = (uint8_t)((tail + 1U) % QUEUE_SIZE);
                usb_report_count++;
            }
        }
        else
        {
            /* HID SET_IDLE may ask for periodic repeats of the same report. */
            Keyboard_Idle();
        }
    }
    __set_PRIMASK(interrupt_mask);

    /* LED: any key down = solid, USB configured and idle = slow blink,
     * waiting for USB = fast blink. HAL_GetTick() is in milliseconds.
     */
    uint32_t now = HAL_GetTick();
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                      stable_mask != 0U || ((now / (configured ? 500U : 125U)) & 1U) ? GPIO_PIN_SET
                                                                                      : GPIO_PIN_RESET);
}
