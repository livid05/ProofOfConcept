/* DIAGNOSTIC COPY for the TMR test (found before firmware/Core/Inc/usbd_conf.h).
 * Only change: two interfaces instead of one. */
#ifndef USBD_CONF_H
#define USBD_CONF_H
#include "stm32u5xx_hal.h"
#include <string.h>
#define USBD_MAX_NUM_INTERFACES 2U /* CDC: control + data interface */
#define USBD_MAX_NUM_CONFIGURATION 1U
#define USBD_MAX_STR_DESC_SIZ 128U
#define USBD_SELF_POWERED 0U
#define USBD_DEBUG_LEVEL 0U
#define USBD_LPM_ENABLED 0U
#define USBD_memset memset
#define USBD_memcpy memcpy
#define USBD_Delay HAL_Delay
#define USBD_UsrLog(...)                                                                           \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#define USBD_ErrLog(...)                                                                           \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#define USBD_DbgLog(...)                                                                           \
    do                                                                                             \
    {                                                                                              \
    } while (0)
#endif
