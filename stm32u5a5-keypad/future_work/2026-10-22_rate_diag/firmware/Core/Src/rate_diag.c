/* rate_diag.c - USB report-rate test stream (replaces Keyboard_Task).
 * DIAGNOSTIC IMAGE ONLY. Never part of the normal keyboard firmware.
 *
 * What it does: as soon as USB is configured, it sends an 8-byte report every
 * time the endpoint is free, as fast as the host allows. Each report carries
 *   bytes 0..3  a counter that goes up by one per accepted report
 *   bytes 4..7  the device microsecond clock (TIM2) when it was submitted
 * both little-endian. A PC program (tools/measure-rate.ps1) reads the stream
 * and works out how many reports per second really arrive and how evenly
 * they are spaced. At 8K polling the ideal spacing is 125 microseconds.
 *
 * Why this measures the real limit: the endpoint holds one report at a time.
 * The next one can only be queued after the USB-completion interrupt of the
 * previous one. If the firmware answers fast enough, one report goes out in
 * every 125 us microframe; if it is too slow or the host does not poll that
 * often, the rate drops (e.g. to 1000 per second) and the spacing shows it.
 *
 * No key scanning happens here. The status LED blinks slowly when streaming.
 */
#include "board_config.h"
#include "key_clock.h"
#include "keyboard.h"
#include "rate_diag.h"

/* Counters for a debugger (optional): reports accepted and busy refusals. */
volatile uint32_t diag_reports_sent, diag_busy_refusals;

void Diag_FillReport(uint8_t report[8], uint32_t counter, uint32_t time_us)
{
    for (uint32_t i = 0; i < 4U; i++)
    {
        report[i] = (uint8_t)(counter >> (8U * i));
        report[4U + i] = (uint8_t)(time_us >> (8U * i));
    }
}

void Keyboard_Task(void)
{
    static uint32_t counter;
    uint32_t now_us = KeyClock_NowUs();
    uint8_t report[8];
    Diag_FillReport(report, counter, now_us);

    /* Same brief interrupt mask the keyboard firmware uses around a send, so
     * the endpoint state cannot change between the check and the submission.
     */
    uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    uint8_t configured = usb_device.dev_state == USBD_STATE_CONFIGURED;
    if (configured)
    {
        if (Keyboard_SendReport(report))
        {
            counter++; /* count only reports USB actually accepted */
            diag_reports_sent++;
        }
        else
        {
            diag_busy_refusals++;
        }
    }
    __set_PRIMASK(interrupt_mask);

    uint32_t now = HAL_GetTick(); /* milliseconds */
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                      ((now / (configured ? 500U : 125U)) & 1U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
