#include "pio_uart.h"
#include "uart.h"  // QMK UART driver — uart_init / uart_write (uart_sio.c on RP2040)

// ─── Hardware UART TX via QMK's SIO UART driver ───────────────────────────────
// Requires in rules.mk:  UART_DRIVER_REQUIRED = yes
// Requires in config.h:  UART_DRIVER SIOD0 / UART_TX_PIN GP0 / UART_TX_PAL_MODE 2
// Requires in mcuconf.h: RP_SIO_USE_UART0 TRUE
//
// QMK's uart_sio.c calls sioStart() + configures GP0 as alternate-2 (UART TX).
// uart_write() uses chnPutTimeout() — fully buffered, never spin-locks.

void pio_uart_init(uint8_t gpio_pin, uint32_t baud) {
    (void)gpio_pin;  // pin is configured by QMK via UART_TX_PIN in config.h
    uart_init(baud);
}

// ─── Send one byte ───────────────────────────────────────────────────────────
void pio_uart_putc(uint8_t byte) {
    uart_write(byte);
}
