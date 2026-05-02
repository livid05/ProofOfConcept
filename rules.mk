ENCODER_ENABLE = yes
ENCODER_MAP_ENABLE = yes

SRC += pio_uart.c

UART_DRIVER_REQUIRED = yes

# Uncomment these when ready to use the HC4067 MUX:
# CUSTOM_MATRIX = lite
# SRC += mux_analog.c
# ANALOG_DRIVER_REQUIRED = yes
# CONSOLE_ENABLE = yes
