/* tmr_hw.c - mux address pins, PC4 and ADC1 for the TMR sensor test.
 * DIAGNOSTIC IMAGE ONLY. The keyboard build does not compile this file.
 *
 * ADC: ADC1, 14-bit, single software-started conversions of ADC1_IN13 (PC4).
 * Kernel clock = HSE 16 MHz (the crystal the USB PHY already needs), so the
 * ADC timing does not depend on the 160 MHz PLL setup. 68.5-cycle sampling =
 * 4.3 us, long enough to charge the ADC's sampling capacitor from the 4.7 nF
 * filter capacitor without disturbing it.
 */
#include "tmr_hw.h"
#include "board_config.h" /* STATUS_LED_PORT / STATUS_LED_PIN (PA5) */
#include "key_clock.h"
#include "main.h"
#include "tmr_config.h"

static ADC_HandleTypeDef hadc1;

static uint8_t ConfigChannel(uint32_t channel, uint32_t sampling_time)
{
    ADC_ChannelConfTypeDef c = {0};
    c.Channel = channel;
    c.Rank = ADC_REGULAR_RANK_1;
    c.SamplingTime = sampling_time;
    c.SingleDiff = ADC_SINGLE_ENDED;
    c.OffsetNumber = ADC_OFFSET_NONE;
    c.Offset = 0;
    return HAL_ADC_ConfigChannel(&hadc1, &c) == HAL_OK;
}

static void InitOutput(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef g = {0};
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET); /* low before it becomes an output */
    g.Pin = pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &g);
}

uint32_t TmrHw_Init(void)
{
    GPIO_InitTypeDef g = {0};

    /* The ADC's analog supply is isolated after reset until VDDA is marked
     * valid. The keyboard image never needed it, so main.c does not do this.
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_EnableVddA();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Mux address S0..S3 (push-pull, start at channel 0). */
    InitOutput(TMR_MUX_S0_PORT, TMR_MUX_S0_PIN);
    InitOutput(TMR_MUX_S1_PORT, TMR_MUX_S1_PIN);
    InitOutput(TMR_MUX_S2_PORT, TMR_MUX_S2_PIN);
    InitOutput(TMR_MUX_S3_PORT, TMR_MUX_S3_PIN);

    /* PC4: analog input, no pull (the pulls are only used by the probe). */
    g.Pin = TMR_SIG_PIN;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(TMR_SIG_PORT, &g);

    /* SW3: PC13 to GND, internal pull-up. */
    g.Pin = TMR_BUTTON_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(TMR_BUTTON_PORT, &g);

    RCC_PeriphCLKInitTypeDef pc = {0};
    pc.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
    pc.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_HSE;
    if (HAL_RCCEx_PeriphCLKConfig(&pc) != HAL_OK)
        return 1;
    __HAL_RCC_ADC12_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1; /* 16 MHz */
    hadc1.Init.Resolution = ADC_RESOLUTION_14B;
    hadc1.Init.GainCompensation = 0;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    /* Conversions are started irregularly (gaps of up to ~1 ms), so use the
     * low-frequency trigger mode, as the HAL notes require for long idle gaps.
     */
    hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_LOW;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
        return 2;
    /* Offset + linearity calibration while the ADC is still disabled. */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK)
        return 3;
    if (!ConfigChannel(TMR_ADC_CHANNEL, ADC_SAMPLETIME_68CYCLES))
        return 4;
    return 0;
}

void TmrHw_Select(uint32_t channel)
{
    TMR_MUX_S0_PORT->BSRR = (channel & 1U) ? TMR_MUX_S0_PIN : ((uint32_t)TMR_MUX_S0_PIN << 16);
    TMR_MUX_S1_PORT->BSRR = (channel & 2U) ? TMR_MUX_S1_PIN : ((uint32_t)TMR_MUX_S1_PIN << 16);
    TMR_MUX_S2_PORT->BSRR = (channel & 4U) ? TMR_MUX_S2_PIN : ((uint32_t)TMR_MUX_S2_PIN << 16);
    TMR_MUX_S3_PORT->BSRR = (channel & 8U) ? TMR_MUX_S3_PIN : ((uint32_t)TMR_MUX_S3_PIN << 16);
}

void TmrHw_WaitUs(uint32_t us)
{
    uint32_t start = KeyClock_NowUs();
    while ((uint32_t)(KeyClock_NowUs() - start) < us)
    {
    }
}

uint32_t TmrHw_ReadCode(void)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
        return 0;
    if (HAL_ADC_PollForConversion(&hadc1, 2) != HAL_OK)
        return 0;
    return HAL_ADC_GetValue(&hadc1);
}

uint32_t TmrHw_ReadSum(uint32_t count)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
        sum += TmrHw_ReadCode();
    return sum;
}

uint32_t TmrHw_ReadVddaMv(void)
{
    /* The internal-reference path can only be switched on while the ADC is
     * disabled, so stop (= disable) it first. HAL_ADC_Start re-enables it.
     */
    (void)HAL_ADC_Stop(&hadc1);
    if (!ConfigChannel(ADC_CHANNEL_VREFINT, ADC_SAMPLETIME_814CYCLES))
    {
        (void)ConfigChannel(TMR_ADC_CHANNEL, ADC_SAMPLETIME_68CYCLES);
        return 0;
    }
    HAL_Delay(2); /* reference buffer start-up */
    uint32_t sum = TmrHw_ReadSum(16);
    (void)HAL_ADC_Stop(&hadc1);
    (void)ConfigChannel(TMR_ADC_CHANNEL, ADC_SAMPLETIME_68CYCLES);
    uint32_t avg = (sum + 8U) / 16U;
    if (avg == 0U)
        return 0;
    /* VDDA = 3.0 V * factory VREFINT reading / our VREFINT reading. */
    return __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC1, avg, LL_ADC_RESOLUTION_14B);
}

/* PC4 mode: MODER field 0 = input, 3 = analog. PUPDR field 0 = none,
 * 1 = pull-up, 2 = pull-down. Direct register writes keep the switch fast.
 */
static void SigMode(uint32_t moder, uint32_t pupdr)
{
    const uint32_t shift = TMR_SIG_PIN_NUMBER * 2U;
    TMR_SIG_PORT->PUPDR = (TMR_SIG_PORT->PUPDR & ~(3U << shift)) | (pupdr << shift);
    TMR_SIG_PORT->MODER = (TMR_SIG_PORT->MODER & ~(3U << shift)) | (moder << shift);
}

void TmrHw_PullProbe(uint32_t channel, uint32_t *code_pullup, uint32_t *code_pulldown)
{
    TmrHw_Select(channel);
    TmrHw_WaitUs(TMR_MUX_SETTLE_US);

    /* ~40 k pull into 4.7 nF + 50 pF is about 0.19 ms per time constant. */
    SigMode(0U, 1U);
    TmrHw_WaitUs(TMR_PULL_SETTLE_US);
    SigMode(3U, 0U);
    *code_pullup = TmrHw_ReadCode();

    SigMode(0U, 2U);
    TmrHw_WaitUs(TMR_PULL_SETTLE_US);
    SigMode(3U, 0U);
    *code_pulldown = TmrHw_ReadCode();
}

uint8_t TmrHw_ButtonDown(void)
{
    return HAL_GPIO_ReadPin(TMR_BUTTON_PORT, TMR_BUTTON_PIN) == GPIO_PIN_RESET;
}

void TmrHw_Led(uint8_t on)
{
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
