#pragma once

#include_next <mcuconf.h>

// Enable RP2040 UART0 in the ChibiOS SIO driver (GP0 TX → TENSTAR at 1 Mbaud)
#undef RP_SIO_USE_UART0
#define RP_SIO_USE_UART0 TRUE
