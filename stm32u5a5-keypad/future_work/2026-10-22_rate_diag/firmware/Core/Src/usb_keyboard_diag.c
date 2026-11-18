/* Standard eight-byte boot keyboard; no vendor driver or RTOS. */
#include "board_config.h"
#include "keyboard.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"
USBD_HandleTypeDef usb_device;
volatile uint8_t keyboard_busy;
volatile uint32_t keyboard_epoch;
/* tx must remain valid and unchanged until endpoint DataIn completion.
 * control_report is separate so a GET_REPORT request cannot corrupt tx.
 * leds receives the host Caps/Num/Scroll Lock bitmap (not displayed yet).
 */
static uint8_t protocol = 1, alt, leds, tx[8], control_report[8];
/* SET_IDLE writes this in the USB IRQ; the foreground task reads it. */
static volatile uint8_t idle;
static uint32_t last_report_ms;
/* Boot keyboard report layout (no report ID):
 * byte 0 = eight modifier bits (Ctrl/Shift/Alt/GUI), byte 1 = reserved,
 * bytes 2..7 = up to six simultaneously held key usages; zero = no key.
 * Output report = one byte containing host keyboard LED states.
 * The matrix firmware fills all six key positions (first key pressed first).
 */
/* *** DIAGNOSTIC COPY *** This is NOT a keyboard. It is a vendor-defined HID
 * device (usage page 0xFF00) with one 8-byte input report and no report ID, so
 * Windows lets a normal program read every report (it forbids that for real
 * keyboards). Everything else - endpoint 0x81, 8-byte packets, high-speed
 * interval USB_HS_BINTERVAL, the ST USB core - is identical to the keyboard.
 * Report bytes 0..3 = report counter, 4..7 = device microsecond clock (both
 * little-endian), filled in by rate_diag.c.
 */
static uint8_t report_descriptor[] = {
    0x06, 0x00, 0xff, /* Usage Page (vendor defined 0xFF00) */
    0x09, 0x01,       /* Usage (1) */
    0xa1, 0x01,       /* Collection (Application) */
    0x15, 0x00,       /*   Logical Minimum 0 */
    0x26, 0xff, 0x00, /*   Logical Maximum 255 */
    0x75, 0x08,       /*   Report Size 8 bits */
    0x95, 0x08,       /*   Report Count 8 -> 8 bytes */
    0x09, 0x02,       /*   Usage (2) */
    0x81, 0x02,       /*   Input (Data, Variable, Absolute) */
    0xc0};            /* End Collection */
/* One configuration, one HID boot-keyboard interface, interrupt IN EP 0x81.
 * Total descriptor length=34 bytes. Bus powered, 100 mA, no remote wakeup.
 * Endpoint maximum packet=8 bytes. FS fallback remains 1 ms. HS interval=1
 * requests one service opportunity per 125 us microframe (8K). Sending only
 * on key changes is normal: an idle endpoint may NAK rather than send data.
 */
static uint8_t cfg[] = {9, 2, 34, 0,    1, 1, 0,    0x80, 50, 9, 4, 0,    0,
                        1, 3, 0,  0,    0, 9, 0x21, 0x11, 1,  0, 1, 0x22, sizeof(report_descriptor),
                        0, 7, 5,  0x81, 3, 8, 0,    1};
static uint8_t other[34];
static uint8_t qualifier[] = {10, 6, 0, 2, 0, 0, 0, 64, 1, 0};
static uint8_t *GetFullSpeedConfig(uint16_t *n)
{
    cfg[1] = 2;
    cfg[33] = USB_FS_BINTERVAL;
    *n = sizeof cfg;
    return cfg;
}
static uint8_t *GetHighSpeedConfig(uint16_t *n)
{
    cfg[1] = 2;
    cfg[33] = USB_HS_BINTERVAL;
    *n = sizeof cfg;
    return cfg;
}
static uint8_t *GetOtherSpeedConfig(uint16_t *n)
{
    memcpy(other, cfg, sizeof cfg);
    other[1] = 7;
    other[33] = usb_device.dev_speed == USBD_SPEED_HIGH ? USB_FS_BINTERVAL : USB_HS_BINTERVAL;
    *n = sizeof other;
    return other;
}
static uint8_t *GetQualifier(uint16_t *n)
{
    *n = sizeof qualifier;
    return qualifier;
}
/* Called when the host selects the configuration. Incrementing epoch tells
 * the switch task to discard events from a previous connection.
 */
static uint8_t Keyboard_Init(USBD_HandleTypeDef *d, uint8_t c)
{
    (void)c;
    keyboard_busy = 0;
    keyboard_epoch++;
    idle = 0;
    protocol = 1;
    memset(tx, 0, 8);
    d->ep_in[1].is_used = 1;
    d->ep_in[1].bInterval = d->dev_speed == USBD_SPEED_HIGH ? USB_HS_BINTERVAL : USB_FS_BINTERVAL;
    return USBD_LL_OpenEP(d, 0x81, USBD_EP_TYPE_INTR, 8);
}
static uint8_t Keyboard_DeInit(USBD_HandleTypeDef *d, uint8_t c)
{
    (void)c;
    keyboard_busy = 0;
    keyboard_epoch++;
    d->ep_in[1].is_used = 0;
    return USBD_LL_CloseEP(d, 0x81);
}
/* Control endpoint requests used by normal OS keyboard drivers:
 * SET/GET_PROTOCOL selects boot/report protocol (same 8-byte layout here).
 * SET/GET_IDLE controls repeat-report timing in units of 4 ms.
 * GET_REPORT returns current input; SET_REPORT accepts keyboard LED output.
 * Unknown or oversized requests are stalled instead of overrunning buffers.
 */
static uint8_t Keyboard_Setup(USBD_HandleTypeDef *d, USBD_SetupReqTypedef *r)
{
    uint8_t *p = 0;
    uint16_t n = 0;
    if ((r->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS)
    {
        switch (r->bRequest)
        {
        case 0x0b:
            protocol = r->wValue & 1;
            return USBD_OK;
        case 0x03:
            p = &protocol;
            n = 1;
            break;
        case 0x0a:
            idle = r->wValue >> 8;
            return USBD_OK;
        case 0x02:
            control_report[0] = idle;
            p = control_report;
            n = 1;
            break;
        case 0x01:
            memcpy(control_report, tx, 8);
            p = control_report;
            n = 8;
            break;
        case 0x09:
            if (r->wLength == 1 && (r->wValue >> 8) == 2)
                return USBD_CtlPrepareRx(d, &leds, 1);
            goto bad;
        default:
            goto bad;
        }
    }
    else if ((r->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
        switch (r->bRequest)
        {
        case USB_REQ_GET_DESCRIPTOR:
            if ((r->wValue >> 8) == 0x22)
            {
                p = report_descriptor;
                n = sizeof report_descriptor;
            }
            else if ((r->wValue >> 8) == 0x21)
            {
                p = cfg + 18;
                n = 9;
            }
            else
            {
                goto bad;
            }
            break;
        case USB_REQ_GET_INTERFACE:
            p = &alt;
            n = 1;
            break;
        case USB_REQ_SET_INTERFACE:
            if (r->wValue != 0)
                goto bad;
            return USBD_OK;
        case USB_REQ_GET_STATUS:
            control_report[0] = control_report[1] = 0;
            p = control_report;
            n = 2;
            break;
        default:
            goto bad;
        }
    }
    else
        goto bad;
    return USBD_CtlSendData(d, p, MIN(n, r->wLength));
bad:
    USBD_CtlError(d, r);
    return USBD_FAIL;
}
/* Interrupt IN transfer complete: the persistent tx buffer is reusable. */
static uint8_t Keyboard_DataIn(USBD_HandleTypeDef *d, uint8_t n)
{
    (void)d;
    (void)n;
    keyboard_busy = 0;
    return USBD_OK;
}
static uint8_t Keyboard_ControlRxReady(USBD_HandleTypeDef *d)
{
    (void)d;
    return USBD_OK;
}
USBD_ClassTypeDef keyboard_class = {.Init = Keyboard_Init,
                                    .DeInit = Keyboard_DeInit,
                                    .Setup = Keyboard_Setup,
                                    .EP0_RxReady = Keyboard_ControlRxReady,
                                    .DataIn = Keyboard_DataIn,
                                    .GetHSConfigDescriptor = GetHighSpeedConfig,
                                    .GetFSConfigDescriptor = GetFullSpeedConfig,
                                    .GetOtherSpeedConfigDescriptor = GetOtherSpeedConfig,
                                    .GetDeviceQualifierDescriptor = GetQualifier};
/* Caller briefly masks interrupts to serialize this with reset/completion.
 * Return 1 only after the USB layer accepts the report. Busy returns 0.
 * Report payload stays in tx; never transmit a pointer to a local array.
 */
uint8_t Keyboard_SendReport(const uint8_t report[8])
{
    if (usb_device.dev_state != USBD_STATE_CONFIGURED || keyboard_busy)
        return 0;
    uint8_t old[8];
    memcpy(old, tx, 8);
    memcpy(tx, report, 8); /* copy into the persistent buffer; report may be a temporary */
    keyboard_busy = 1;
    if (USBD_LL_Transmit(&usb_device, 0x81, tx, 8) != USBD_OK)
    {
        keyboard_busy = 0;
        memcpy(tx, old, 8);
        return 0;
    }
    last_report_ms = HAL_GetTick();
    return 1;
}
/* Single-key convenience wrapper kept for the one-key firmware's API. */
uint8_t Keyboard_Send(uint8_t usage)
{
    uint8_t report[8] = {0, 0, usage, 0, 0, 0, 0, 0};
    return Keyboard_SendReport(report);
}
/* Called under the same brief interrupt mask as event transmission.
 * Repeats the whole last report (all held keys), not just one key.
 */
void Keyboard_Idle(void)
{
    if (idle && !keyboard_busy && (uint32_t)(HAL_GetTick() - last_report_ms) >= (uint32_t)idle * 4)
    {
        uint8_t again[8];
        memcpy(again, tx, 8);
        Keyboard_SendReport(again);
    }
}
/* ST demonstration VID/PID: prototype only, obtain an assigned pair for release. */
static uint8_t device[] = {18, 1, 0, 2, 0, 0, 0, 64, 0x83, 0x04, 0x11, 0x57, 0, 1, 1, 2, 3, 1}; /* PID 0x5711 (keyboard is 0x5710) so Windows treats it as a fresh device */
static uint8_t lang[] = {4, 3, 9, 4}, str[128];
static uint8_t *GetDeviceDescriptor(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    *n = sizeof device;
    return device;
}
static uint8_t *GetLanguageDescriptor(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    *n = sizeof lang;
    return lang;
}
static uint8_t *MakeStringDescriptor(const char *t, uint16_t *n)
{
    USBD_GetString((uint8_t *)t, str, n);
    return str;
}
static uint8_t *GetManufacturer(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    return MakeStringDescriptor("Capstone", n);
}
static uint8_t *GetProduct(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    return MakeStringDescriptor("8K Rate Test", n);
}
/* Unique MCU ID gives each board a stable USB serial number. */
static uint8_t *GetSerial(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    static char hex[25];
    uint32_t uid[3] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    for (unsigned i = 0; i < 24; i++)
    {
        hex[i] = "0123456789ABCDEF"[(uid[i / 8] >> (28 - 4 * (i % 8))) & 15];
    }
    return MakeStringDescriptor(hex, n);
}
static uint8_t *GetInterfaceLabel(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    return MakeStringDescriptor("Rate Test Interface", n);
}
USBD_DescriptorsTypeDef keyboard_descriptors = {
    GetDeviceDescriptor, GetLanguageDescriptor, GetManufacturer,  GetProduct,
    GetSerial,           GetInterfaceLabel,     GetInterfaceLabel};
