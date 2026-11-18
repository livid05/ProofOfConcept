/* usb_cdc_diag.h - USB virtual COM port for the TMR test image.
 * DIAGNOSTIC IMAGE ONLY. Replaces usb_keyboard.c in this build.
 */
#ifndef USB_CDC_DIAG_H
#define USB_CDC_DIAG_H

#include <stdint.h>

/* Queue text for the PC. Never blocks: returns how many bytes fitted. */
uint32_t Cdc_Write(const char *data, uint32_t len);
/* Moves queued text to the USB endpoint. Call often from the main loop. */
void Cdc_Task(void);
/* Next byte typed in the terminal, or -1 if none. */
int Cdc_GetChar(void);
/* Discard text that is queued but not yet sent. */
void Cdc_Flush(void);
/* USB configured by the PC. */
uint8_t Cdc_Configured(void);
/* Terminal has the port open (DTR set). Not every terminal sets DTR. */
uint8_t Cdc_TerminalOpen(void);
/* Counts every time a terminal opens the port (DTR goes 0 -> 1). */
uint32_t Cdc_OpenCount(void);
/* Bytes dropped because the queue was full (usually: no terminal open). */
uint32_t Cdc_Dropped(void);

#endif
