/* usb_cdc_diag.c - USB CDC-ACM "virtual COM port" for the TMR test image.
 * *** DIAGNOSTIC COPY *** Replaces usb_keyboard.c in this build only; the
 * keyboard image is unchanged.
 *
 * Same ST USB core, same usb_port.c (HS PHY setup that is proven on this
 * board) and same main.c as the keyboard. Only the class is different: a
 * standard CDC-ACM serial port, which Windows 10/11 binds to its built-in
 * usbser driver ("USB Serial Device (COMx)") with no driver install.
 *
 * Interfaces: 0 = CDC control (notification EP 0x81, never used),
 *             1 = CDC data (bulk OUT 0x02, bulk IN 0x82).
 * High speed: 512-byte bulk packets. Full-speed fallback: 64 bytes.
 *
 * The names usb_device, keyboard_class and keyboard_descriptors are kept
 * because the shared main.c registers them; here they describe a COM port.
 */
#include <string.h>

#include "keyboard.h"
#include "main.h"
#include "tmr_config.h"
#include "usb_cdc_diag.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"

/* Defined in the shared usb_port.c. */
extern PCD_HandleTypeDef hpcd;

USBD_HandleTypeDef usb_device;

#define EP_NOTIFY 0x81U
#define EP_OUT 0x02U
#define EP_IN 0x82U
#define NOTIFY_SIZE 16U
#define TX_RING_SIZE 8192U /* power of two */
#define RX_RING_SIZE 256U  /* power of two */

/* TX: the foreground writes the ring and also drains it (Cdc_Task), so the
 * ring needs no locking. tx_packet must stay unchanged until DataIn says the
 * USB transfer is complete.
 */
static uint8_t tx_ring[TX_RING_SIZE];
static uint32_t tx_head, tx_tail, tx_dropped;
static uint8_t tx_packet[512];
static volatile uint8_t tx_busy;

/* RX: filled in the USB interrupt, emptied by the foreground. */
static uint8_t rx_packet[512];
static uint8_t rx_ring[RX_RING_SIZE];
static volatile uint32_t rx_head, rx_tail;

/* Line coding is stored and echoed back only: a USB COM port has no baud
 * rate, so any terminal setting works. Default 115200 8N1.
 */
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8};
static uint8_t line_rx[7];
static volatile uint8_t line_pending;
static volatile uint8_t dtr;
static volatile uint32_t open_count;
static uint8_t alt, status_reply[2];

/* ---- Descriptors ------------------------------------------------------- */
/* Device: class EF/02/01 = "uses an Interface Association Descriptor", which
 * is how Windows finds the two-interface CDC function. VID/PID: tmr_config.h.
 */
static uint8_t device[] = {18,   1,    0x00, 0x02, 0xEF, 0x02, 0x01, 64,
                           (uint8_t)(TMR_USB_VID & 0xFF), (uint8_t)(TMR_USB_VID >> 8),
                           (uint8_t)(TMR_USB_PID & 0xFF), (uint8_t)(TMR_USB_PID >> 8),
                           0x00, 0x01, 1,    2,    3,    1};

#define CFG_LEN 75U
/* Configuration: 9 config + 8 IAD + 9 control interface + 5 header + 5 call
 * management + 4 ACM + 5 union + 7 notify EP + 9 data interface + 7 OUT EP +
 * 7 IN EP = 75 bytes. Bus powered, 100 mA.
 * Byte 47 = notify interval, bytes 64/65 and 71/72 = bulk packet size.
 */
#define CFG_BYTES(notify_interval, bulk_lo, bulk_hi)                                               \
    {9, 2, CFG_LEN, 0, 2, 1, 0, 0x80, 50,                     /* configuration */                 \
     8, 0x0B, 0, 2, 0x02, 0x02, 0x01, 0,                      /* IAD: itf 0-1 = CDC ACM */        \
     9, 4, 0, 0, 1, 0x02, 0x02, 0x01, 0,                      /* itf 0: CDC control */            \
     5, 0x24, 0x00, 0x10, 0x01,                               /* header, CDC 1.10 */              \
     5, 0x24, 0x01, 0x00, 1,                                  /* call management, data itf 1 */   \
     4, 0x24, 0x02, 0x02,                                     /* ACM: line coding + state */      \
     5, 0x24, 0x06, 0, 1,                                     /* union: 0 controls 1 */           \
     7, 5, EP_NOTIFY, 3, NOTIFY_SIZE, 0, notify_interval,     /* notification, interrupt IN */    \
     9, 4, 1, 0, 2, 0x0A, 0, 0, 0,                            /* itf 1: CDC data */               \
     7, 5, EP_OUT, 2, bulk_lo, bulk_hi, 0,                    /* bulk OUT */                      \
     7, 5, EP_IN, 2, bulk_lo, bulk_hi, 0}                     /* bulk IN */

/* Notify interval: full speed in ms (16), high speed 2^(8-1) x 125 us = 16 ms. */
static uint8_t cfg_fs[CFG_LEN] = CFG_BYTES(16, 64, 0);
static uint8_t cfg_hs[CFG_LEN] = CFG_BYTES(8, 0x00, 0x02);
static uint8_t cfg_other[CFG_LEN];
static uint8_t qualifier[] = {10, 6, 0x00, 0x02, 0xEF, 0x02, 0x01, 64, 1, 0};

static uint8_t *GetFullSpeedConfig(uint16_t *n)
{
    *n = CFG_LEN;
    return cfg_fs;
}
static uint8_t *GetHighSpeedConfig(uint16_t *n)
{
    *n = CFG_LEN;
    return cfg_hs;
}
static uint8_t *GetOtherSpeedConfig(uint16_t *n)
{
    memcpy(cfg_other, usb_device.dev_speed == USBD_SPEED_HIGH ? cfg_fs : cfg_hs, CFG_LEN);
    cfg_other[1] = 7; /* OTHER_SPEED_CONFIGURATION */
    *n = CFG_LEN;
    return cfg_other;
}
static uint8_t *GetQualifier(uint16_t *n)
{
    *n = sizeof qualifier;
    return qualifier;
}

static uint16_t BulkSize(const USBD_HandleTypeDef *d)
{
    return d->dev_speed == USBD_SPEED_HIGH ? 512U : 64U;
}

/* ---- Class callbacks (USB interrupt context) ---------------------------- */
static uint8_t Cdc_Init(USBD_HandleTypeDef *d, uint8_t cfgidx)
{
    (void)cfgidx;
    uint16_t mps = BulkSize(d);
    tx_busy = 0;
    dtr = 0;

    /* usb_port.c sizes the FIFOs for the keyboard: RX 512 words, TX0 and TX1
     * 64 words each. Bulk IN on EP2 needs its own 512-byte (128-word) FIFO.
     * Total 512 + 64 + 64 + 128 = 768 of the OTG_HS core's 1024 words.
     */
    if (HAL_PCDEx_SetTxFiFo(&hpcd, 2, 0x80) != HAL_OK)
        return USBD_FAIL;

    d->ep_in[EP_NOTIFY & 0x0FU].is_used = 1;
    d->ep_in[EP_NOTIFY & 0x0FU].maxpacket = NOTIFY_SIZE;
    d->ep_in[EP_NOTIFY & 0x0FU].bInterval = d->dev_speed == USBD_SPEED_HIGH ? 8U : 16U;
    d->ep_in[EP_IN & 0x0FU].is_used = 1;
    d->ep_in[EP_IN & 0x0FU].maxpacket = mps;
    d->ep_out[EP_OUT & 0x0FU].is_used = 1;
    d->ep_out[EP_OUT & 0x0FU].maxpacket = mps;
    if (USBD_LL_OpenEP(d, EP_NOTIFY, USBD_EP_TYPE_INTR, NOTIFY_SIZE) != USBD_OK ||
        USBD_LL_OpenEP(d, EP_IN, USBD_EP_TYPE_BULK, mps) != USBD_OK ||
        USBD_LL_OpenEP(d, EP_OUT, USBD_EP_TYPE_BULK, mps) != USBD_OK)
        return USBD_FAIL;
    return USBD_LL_PrepareReceive(d, EP_OUT, rx_packet, mps) == USBD_OK ? USBD_OK : USBD_FAIL;
}

static uint8_t Cdc_DeInit(USBD_HandleTypeDef *d, uint8_t cfgidx)
{
    (void)cfgidx;
    tx_busy = 0;
    dtr = 0;
    d->ep_in[EP_NOTIFY & 0x0FU].is_used = 0;
    d->ep_in[EP_IN & 0x0FU].is_used = 0;
    d->ep_out[EP_OUT & 0x0FU].is_used = 0;
    (void)USBD_LL_CloseEP(d, EP_NOTIFY);
    (void)USBD_LL_CloseEP(d, EP_IN);
    (void)USBD_LL_CloseEP(d, EP_OUT);
    return USBD_OK;
}

/* CDC-ACM class requests used by Windows usbser and terminal programs:
 *   0x20 SET_LINE_CODING (7 data bytes), 0x21 GET_LINE_CODING,
 *   0x22 SET_CONTROL_LINE_STATE (bit 0 = DTR = terminal open), 0x23 SEND_BREAK.
 * Anything else is stalled rather than guessed at.
 */
static uint8_t Cdc_Setup(USBD_HandleTypeDef *d, USBD_SetupReqTypedef *r)
{
    uint8_t *p = 0;
    uint16_t n = 0;
    if ((r->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS)
    {
        switch (r->bRequest)
        {
        case 0x20:
            if (r->wLength != sizeof line_rx)
                goto bad;
            line_pending = 1;
            return USBD_CtlPrepareRx(d, line_rx, sizeof line_rx);
        case 0x21:
            p = line_coding;
            n = sizeof line_coding;
            break;
        case 0x22:
        {
            uint8_t now = (uint8_t)(r->wValue & 1U);
            if (now && !dtr)
                open_count++;
            dtr = now;
            return USBD_OK;
        }
        case 0x23:
            return USBD_OK;
        default:
            goto bad;
        }
    }
    else if ((r->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
        switch (r->bRequest)
        {
        case USB_REQ_GET_INTERFACE:
            p = &alt;
            n = 1;
            break;
        case USB_REQ_SET_INTERFACE:
            if (r->wValue != 0)
                goto bad;
            return USBD_OK;
        case USB_REQ_GET_STATUS:
            status_reply[0] = status_reply[1] = 0;
            p = status_reply;
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

static uint8_t Cdc_EP0_RxReady(USBD_HandleTypeDef *d)
{
    (void)d;
    if (line_pending)
    {
        memcpy(line_coding, line_rx, sizeof line_coding);
        line_pending = 0;
    }
    return USBD_OK;
}

static uint8_t Cdc_DataIn(USBD_HandleTypeDef *d, uint8_t epnum)
{
    (void)d;
    if (epnum == (EP_IN & 0x0FU))
        tx_busy = 0;
    return USBD_OK;
}

static uint8_t Cdc_DataOut(USBD_HandleTypeDef *d, uint8_t epnum)
{
    if (epnum != (EP_OUT & 0x0FU))
        return USBD_OK;
    uint32_t n = USBD_LL_GetRxDataSize(d, epnum);
    for (uint32_t i = 0; i < n && i < sizeof rx_packet; i++)
    {
        if (rx_head - rx_tail < RX_RING_SIZE)
        {
            rx_ring[rx_head & (RX_RING_SIZE - 1U)] = rx_packet[i];
            rx_head++;
        }
    }
    return USBD_LL_PrepareReceive(d, EP_OUT, rx_packet, BulkSize(d)) == USBD_OK ? USBD_OK : USBD_FAIL;
}

USBD_ClassTypeDef keyboard_class = {.Init = Cdc_Init,
                                    .DeInit = Cdc_DeInit,
                                    .Setup = Cdc_Setup,
                                    .EP0_RxReady = Cdc_EP0_RxReady,
                                    .DataIn = Cdc_DataIn,
                                    .DataOut = Cdc_DataOut,
                                    .GetHSConfigDescriptor = GetHighSpeedConfig,
                                    .GetFSConfigDescriptor = GetFullSpeedConfig,
                                    .GetOtherSpeedConfigDescriptor = GetOtherSpeedConfig,
                                    .GetDeviceQualifierDescriptor = GetQualifier};

/* ---- Foreground API ------------------------------------------------------ */
uint32_t Cdc_Write(const char *data, uint32_t len)
{
    uint32_t space = TX_RING_SIZE - (tx_head - tx_tail);
    uint32_t n = len < space ? len : space;
    for (uint32_t i = 0; i < n; i++)
        tx_ring[(tx_head + i) & (TX_RING_SIZE - 1U)] = (uint8_t)data[i];
    tx_head += n;
    tx_dropped += len - n;
    return n;
}

void Cdc_Task(void)
{
    if (usb_device.dev_state != USBD_STATE_CONFIGURED || tx_busy)
        return;
    uint32_t avail = tx_head - tx_tail;
    if (avail == 0U)
        return;
    uint32_t mps = BulkSize(&usb_device);
    /* At most 511 bytes, and never an exact multiple of the packet size, so
     * every transfer ends in a short packet and the PC hands the text to the
     * terminal immediately (no zero-length packet needed).
     */
    uint32_t n = avail < 511U ? avail : 511U;
    if (n >= mps && (n % mps) == 0U)
        n--;
    for (uint32_t i = 0; i < n; i++)
        tx_packet[i] = tx_ring[(tx_tail + i) & (TX_RING_SIZE - 1U)];

    /* Same brief interrupt mask the keyboard uses around submission, so a
     * bus reset or completion interrupt cannot interleave with it.
     */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (usb_device.dev_state == USBD_STATE_CONFIGURED && !tx_busy)
    {
        tx_busy = 1;
        if (USBD_LL_Transmit(&usb_device, EP_IN, tx_packet, n) == USBD_OK)
            tx_tail += n;
        else
            tx_busy = 0;
    }
    __set_PRIMASK(primask);
}

int Cdc_GetChar(void)
{
    if (rx_tail == rx_head)
        return -1;
    int c = rx_ring[rx_tail & (RX_RING_SIZE - 1U)];
    rx_tail++;
    return c;
}

void Cdc_Flush(void)
{
    tx_tail = tx_head;
}

uint8_t Cdc_Configured(void)
{
    return usb_device.dev_state == USBD_STATE_CONFIGURED;
}

uint8_t Cdc_TerminalOpen(void)
{
    return dtr;
}

uint32_t Cdc_OpenCount(void)
{
    return open_count;
}

uint32_t Cdc_Dropped(void)
{
    return tx_dropped;
}

/* ---- Strings --------------------------------------------------------------- */
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
    return MakeStringDescriptor("Keypad U5A5 - TMR Test", n);
}
static uint8_t *GetSerial(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    static char hex[25];
    uint32_t uid[3] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    for (unsigned i = 0; i < 24; i++)
        hex[i] = "0123456789ABCDEF"[(uid[i / 8] >> (28 - 4 * (i % 8))) & 15];
    return MakeStringDescriptor(hex, n);
}
static uint8_t *GetInterfaceLabel(USBD_SpeedTypeDef s, uint16_t *n)
{
    (void)s;
    return MakeStringDescriptor("TMR Test Console", n);
}
USBD_DescriptorsTypeDef keyboard_descriptors = {
    GetDeviceDescriptor, GetLanguageDescriptor, GetManufacturer,  GetProduct,
    GetSerial,           GetInterfaceLabel,     GetInterfaceLabel};
