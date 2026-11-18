/* rate_diag.h - report format of the 8K rate test stream. DIAGNOSTIC ONLY. */
#ifndef RATE_DIAG_H
#define RATE_DIAG_H
#include <stdint.h>

/* Writes counter (bytes 0..3) and microsecond time (bytes 4..7), little-endian. */
void Diag_FillReport(uint8_t report[8], uint32_t counter, uint32_t time_us);

extern volatile uint32_t diag_reports_sent, diag_busy_refusals;
#endif
