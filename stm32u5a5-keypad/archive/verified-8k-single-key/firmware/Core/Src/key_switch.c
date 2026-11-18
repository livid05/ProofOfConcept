/* One-switch input state machine and reliable USB press/release delivery.
 *
 * Data path:
 *   PB4 -> 125 us sampling -> eager press / debounced release -> queue -> USB.
 *
 * A press report contains KEY_HID_USAGE. A release report contains zero.
 * Both events must reach the host; dropping release can cause a stuck key.
 */
#include "board_config.h"
#include "key_clock.h"
#include "keyboard.h"

/* Watch these in a debugger. They count physical debounced presses and
 * accepted report submissions, not proof that a host displayed a character.
 * usb_report_count includes the initial all-keys-up synchronization report.
 */
volatile uint32_t key_press_count, usb_report_count, key_queue_overflow;
volatile uint8_t key_pressed;
/* Timing instrumentation: target cadence is 8K, not a measured guarantee.
 * A missed period means one or more scheduled sampling slots were skipped.
 * Watch max gap/counts with a debugger; do not infer USB timing from them.
 */
volatile uint32_t key_scan_count, key_scan_missed_periods, key_scan_max_gap_us;
void Keyboard_Idle(void);

void Keyboard_Task(void)
{
    static uint32_t last_sample_us, last_observed_scan_us, raw_changed_us, observed_usb_epoch;
    static uint8_t clock_started;
    static uint8_t raw_pressed, stable_pressed, armed, was_configured;
    /* Ring buffer holds 15 transitions while the USB endpoint is busy.
     * Persistent storage is required: USB sends asynchronously from the IRQ.
     */
    static uint8_t queue[16], head, tail, sync_pending;
    uint32_t now_us = KeyClock_NowUs();
    uint8_t edge = 0;
    if (!clock_started)
    {
        /* Start observing now, not at the earlier beginning of USB init. */
        clock_started = 1;
        last_sample_us = last_observed_scan_us = raw_changed_us = now_us;
    }

    uint32_t elapsed_us = now_us - last_sample_us;
    if (elapsed_us >= KEY_SCAN_PERIOD_US)
    {
        key_scan_count++;
        uint32_t actual_gap_us = now_us - last_observed_scan_us;
        last_observed_scan_us = now_us;
        if (actual_gap_us > key_scan_max_gap_us)
            key_scan_max_gap_us = actual_gap_us;
        /* Keep phase when a scan is late. Never invent catch-up samples of
         * old switch states: read once, count missed slots, advance schedule.
         */
        uint32_t periods = elapsed_us / KEY_SCAN_PERIOD_US;
        key_scan_missed_periods += periods - 1U;
        last_sample_us += periods * KEY_SCAN_PERIOD_US;
        uint8_t sample = HAL_GPIO_ReadPin(KEY_COL_PORT, KEY_COL_PIN) == GPIO_PIN_RESET;
        if (sample != raw_pressed)
        {
            raw_pressed = sample;
            raw_changed_us = now_us;
        }

        /* Eager press: first observed closure is accepted immediately.
         * While pressed, all contact bounce is ignored until the contact
         * stays released for the entire debounce time. This avoids duplicate
         * key presses without adding that debounce delay to every press.
         * With eager mode disabled both edges require a stable debounce time.
         */
        if (raw_pressed != stable_pressed &&
            ((KEY_EAGER_PRESS && raw_pressed) ||
             (uint32_t)(now_us - raw_changed_us) >= KEY_DEBOUNCE_US))
        {
            stable_pressed = raw_pressed;
            key_pressed = stable_pressed;
            edge = 1;
            if (stable_pressed)
                key_press_count++;
        }
    }

    /* USB configuration/completion changes in an interrupt. Mask interrupts
     * only around checking that state and starting a nonblocking transfer.
     * Restoring PRIMASK preserves the caller's previous interrupt state.
     */
    uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    uint8_t configured = usb_device.dev_state == USBD_STATE_CONFIGURED;
    if (!configured || observed_usb_epoch != keyboard_epoch || !was_configured)
    {
        observed_usb_epoch = keyboard_epoch;
        head = tail = 0;
        armed = 0;
        sync_pending = 1;
        was_configured = configured;
    }

    if (configured)
    {
        /* Suppress a key held during connection. The user must first release
         * it, then make a fresh press. Also flush old events after USB reset.
         */
        if (!stable_pressed && !raw_pressed &&
            (uint32_t)(now_us - raw_changed_us) >= KEY_DEBOUNCE_US)
            armed = 1;

        if (edge && armed)
        {
            uint8_t next_head = (head + 1U) & 15U;
            if (next_head == tail)
            {
                /* If a broken/stalled connection fills the queue, recover
                 * with all keys released instead of leaving a key stuck.
                 * The current gesture is discarded; release before retry.
                 */
                key_queue_overflow++;
                head = tail = 0;
                sync_pending = 1;
                armed = 0;
            }
            else
            {
                queue[head] = stable_pressed ? KEY_HID_USAGE : 0;
                head = next_head;
            }
        }

        /* Only advance the queue when USB accepts a report. Busy means retry
         * next loop iteration, even between scans. A release waits behind its press.
         */
        if (sync_pending)
        {
            if (Keyboard_Send(0))
            {
                sync_pending = 0;
                usb_report_count++;
            }
        }
        else if (head != tail)
        {
            if (Keyboard_Send(queue[tail]))
            {
                tail = (tail + 1U) & 15U;
                usb_report_count++;
            }
        }
        else
        {
            /* HID SET_IDLE can request periodic repeats of the same report.
             * This is separate from the operating system's text key repeat.
             */
            Keyboard_Idle();
        }
    }
    __set_PRIMASK(interrupt_mask);

    /* LED: pressed=solid, configured idle=slow blink, awaiting USB=fast blink.
     * Error_Handler has its own rapid blink if initialization fails.
     */
    uint32_t now = HAL_GetTick(); /* LED blink remains in milliseconds. */
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                      stable_pressed || ((now / (configured ? 500U : 125U)) & 1U) ? GPIO_PIN_SET
                                                                                  : GPIO_PIN_RESET);
}
