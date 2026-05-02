#pragma once

// Encoder rotation pins
#define ENCODER_A_PINS { GP15 }
#define ENCODER_B_PINS { GP14 }
#define ENCODER_RESOLUTION 4
#define NUM_ENCODERS 1

// Encoder push button (direct GPIO, not wired to matrix)
#define ENCODER_SWITCH_PIN GP12

// Hardware UART0 on GP0 TX → TENSTAR Serial1 RX (1 Mbaud)
#define UART_DRIVER      SIOD0
#define UART_TX_PIN      GP0
#define UART_TX_PAL_MODE 2
#define UART_RX_PIN      GP1
#define UART_RX_PAL_MODE 2

// RGB Matrix defaults (applied when EEPROM is blank / first flash)
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_LEFT_RIGHT
#define RGB_MATRIX_DEFAULT_VAL  150
#define RGB_MATRIX_DEFAULT_SPD  40
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 200
