#pragma once
#include <stdint.h>

// ─── Hardware UART TX (RP2040 UART0 peripheral, Pico SDK direct) ─────────────
// GP0 → UART0 TX → TENSTAR Serial1 RX
// 1 Mbaud = 10 bits × 1 µs = 10 µs/byte → 4-byte packet in 40 µs
// 16-byte hardware TX FIFO — keyboard use never overflows it.

void pio_uart_init(uint8_t gpio_pin, uint32_t baud);
void pio_uart_putc(uint8_t byte);
