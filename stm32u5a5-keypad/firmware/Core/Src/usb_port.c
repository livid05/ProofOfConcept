/* STM32U5A5 internal HS PHY. Adapted from ST CubeU5 V1.8.0 USBX example. */
#include "keyboard.h"
#include "main.h"
/* PCD = HAL peripheral controller driver. USBD = portable USB device core.
 * Their pData pointers link each handle back to the other. The callbacks
 * below forward hardware interrupts into the portable USB state machine.
 */
PCD_HandleTypeDef hpcd;
static USBD_StatusTypeDef status(HAL_StatusTypeDef s)
{
    return s == HAL_OK ? USBD_OK : USBD_FAIL;
}
/* HAL invokes this before initializing the controller.
 * PA11/PA12 are owned by the embedded HS PHY, not a GPIO alternate-function
 * setup. The PHY takes HSE=16 MHz and makes its USB clock internally.
 * The board has no dedicated VBUS input here: VBUS sensing is disabled.
 */
void HAL_PCD_MspInit(PCD_HandleTypeDef *p)
{
    (void)p;
    RCC_PeriphCLKInitTypeDef c = {0};
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    c.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
    c.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_HSE;
    if (HAL_RCCEx_PeriphCLKConfig(&c) != HAL_OK)
        Error_Handler();
    HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USBPHYC_CLK_ENABLE();
    HAL_PWREx_EnableVddUSB();
    if (HAL_PWREx_EnableUSBHSTranceiverSupply() != HAL_OK)
        Error_Handler();
    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
}
void OTG_HS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd);
}
/* Use the embedded HS PHY even on a USB 2.0 full-speed fallback connection.
 * DMA and low-power modes are deliberately disabled for this first test.
 * FIFO sizes are in 32-bit WORDS, not bytes: RX=512 words, TX0/TX1=64 each.
 */
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *d)
{
    hpcd.Instance = USB_OTG_HS;
    hpcd.pData = d;
    d->pData = &hpcd;
    hpcd.Init.dev_endpoints = 9;
    hpcd.Init.speed = PCD_SPEED_HIGH;
    hpcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
    hpcd.Init.Sof_enable = DISABLE;
    hpcd.Init.low_power_enable = DISABLE;
    hpcd.Init.lpm_enable = DISABLE;
    hpcd.Init.use_dedicated_ep1 = DISABLE;
    hpcd.Init.vbus_sensing_enable = DISABLE;
    hpcd.Init.dma_enable = DISABLE;
    if (HAL_PCD_Init(&hpcd) != HAL_OK)
        return USBD_FAIL;
    if (HAL_PCDEx_SetRxFiFo(&hpcd, 0x200) != HAL_OK ||
        HAL_PCDEx_SetTxFiFo(&hpcd, 0, 0x40) != HAL_OK ||
        HAL_PCDEx_SetTxFiFo(&hpcd, 1, 0x40) != HAL_OK)
        return USBD_FAIL;
    return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *d)
{
    return status(HAL_PCD_DeInit(d->pData));
}
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *d)
{
    return status(HAL_PCD_Start(d->pData));
}
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *d)
{
    return status(HAL_PCD_Stop(d->pData));
}
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *d, uint8_t a, uint8_t t, uint16_t n)
{
    return status(HAL_PCD_EP_Open(d->pData, a, n, t));
}
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *d, uint8_t a)
{
    return status(HAL_PCD_EP_Close(d->pData, a));
}
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *d, uint8_t a)
{
    return status(HAL_PCD_EP_Flush(d->pData, a));
}
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *d, uint8_t a)
{
    return status(HAL_PCD_EP_SetStall(d->pData, a));
}
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *d, uint8_t a)
{
    return status(HAL_PCD_EP_ClrStall(d->pData, a));
}
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *d, uint8_t a)
{
    PCD_HandleTypeDef *p = d->pData;
    return (a & 0x80) ? p->IN_ep[a & 0x7f].is_stall : p->OUT_ep[a & 0x7f].is_stall;
}
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *d, uint8_t a)
{
    return status(HAL_PCD_SetAddress(d->pData, a));
}
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *d, uint8_t a, uint8_t *b, uint32_t n)
{
    return status(HAL_PCD_EP_Transmit(d->pData, a, b, n));
}
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *d, uint8_t a, uint8_t *b, uint32_t n)
{
    return status(HAL_PCD_EP_Receive(d->pData, a, b, n));
}
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *d, uint8_t a)
{
    return HAL_PCD_EP_GetRxCount(d->pData, a);
}
void USBD_LL_Delay(uint32_t n)
{
    HAL_Delay(n);
}
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *p)
{
    USBD_LL_SetupStage(p->pData, (uint8_t *)p->Setup);
}
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *p, uint8_t n)
{
    USBD_LL_DataOutStage(p->pData, n, p->OUT_ep[n].xfer_buff);
}
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *p, uint8_t n)
{
    USBD_LL_DataInStage(p->pData, n, p->IN_ep[n].xfer_buff);
}
/* HAL updates Init.speed from the negotiated link speed before this callback;
 * inform the core so the host receives matching FS or HS descriptors.
 */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *p)
{
    USBD_LL_SetSpeed(p->pData, p->Init.speed == PCD_SPEED_HIGH ? USBD_SPEED_HIGH : USBD_SPEED_FULL);
    USBD_LL_Reset(p->pData);
}
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *p)
{
    USBD_LL_Suspend(p->pData);
}
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *p)
{
    USBD_LL_Resume(p->pData);
}
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *p)
{
    USBD_LL_DevConnected(p->pData);
}
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *p)
{
    USBD_LL_DevDisconnected(p->pData);
}
