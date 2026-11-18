/* One-switch input state machine and reliable USB press/release delivery.
 *
 * Data path:
 *   PB4 electrical level -> 10 ms debounce -> event queue -> USB report.
 *
 * A press report contains KEY_HID_USAGE. A release report contains zero.
 * Both events must reach the host; dropping release can cause a stuck key.
 */
#include "board_config.h"
#include "keyboard.h"

/* Watch these in a debugger. They count physical debounced presses and
 * accepted report submissions, not proof that a host displayed a character.
 * usb_report_count includes the initial all-keys-up synchronization report.
 */
volatile uint32_t key_press_count, usb_report_count, key_queue_overflow;
volatile uint8_t key_pressed;
void Keyboard_Idle(void);

void Keyboard_Task(void)
{
    static uint32_t last_sample_ms, raw_changed_ms, observed_usb_epoch;
    static uint8_t raw_pressed, stable_pressed, armed, was_configured;
    /* Ring buffer holds 15 transitions while the USB endpoint is busy.
     * Persistent storage is required: USB sends asynchronously from the IRQ.
     */
    static uint8_t queue[16], head, tail, sync_pending;
    uint32_t now = HAL_GetTick();

    /* Sample at most once per millisecond; do not stall USB with HAL_Delay. */
    if (now == last_sample_ms)
        return;
    last_sample_ms = now;
    uint8_t sample = HAL_GPIO_ReadPin(KEY_COL_PORT, KEY_COL_PIN) == GPIO_PIN_RESET;
    if (sample != raw_pressed)
    {
        raw_pressed = sample;
        raw_changed_ms = now;
    }

    /* Every bounce restarts the timer. Only a continuously stable level is
     * accepted. Unsigned subtraction also handles HAL_GetTick wraparound.
     */
    uint8_t edge = 0;
    if (raw_pressed != stable_pressed && (uint32_t)(now - raw_changed_ms) >= KEY_DEBOUNCE_MS)
    {
        stable_pressed = raw_pressed;
        key_pressed = stable_pressed;
        edge = 1;
        if (stable_pressed)
            key_press_count++;
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
        if (!stable_pressed && !raw_pressed && (uint32_t)(now - raw_changed_ms) >= KEY_DEBOUNCE_MS)
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
         * next millisecond. Thus a quick release waits behind its press.
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
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                      stable_pressed || ((now / (configured ? 500U : 125U)) & 1U) ? GPIO_PIN_SET
                                                                                  : GPIO_PIN_RESET);
}
