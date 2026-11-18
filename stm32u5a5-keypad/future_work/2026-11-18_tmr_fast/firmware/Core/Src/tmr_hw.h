/* tmr_hw.h - the only part of the TMR test that touches the mux and the ADC.
 * DIAGNOSTIC IMAGE ONLY.
 */
#ifndef TMR_HW_H
#define TMR_HW_H

#include <stdint.h>

/* Enables VDDA, sets up the mux address pins, PC4 and ADC1 (14-bit, offset +
 * linearity calibration). Returns 0 on success, otherwise a step number that
 * the console prints (1 = ADC clock, 2 = ADC init, 3 = calibration,
 * 4 = channel setup).
 */
uint32_t TmrHw_Init(void);

/* Put mux channel 0..15 on the ADC. Does not wait; call TmrHw_WaitUs. */
void TmrHw_Select(uint32_t channel);
void TmrHw_WaitUs(uint32_t us);

/* One 14-bit conversion of the selected channel (0..16383). */
uint32_t TmrHw_ReadCode(void);
/* Sum of `count` conversions (divide by count for the average). */
uint32_t TmrHw_ReadSum(uint32_t count);

/* Measures VDDA (the ADC reference) in mV with the chip's factory-calibrated
 * internal reference. Restores the sensor channel afterwards.
 */
uint32_t TmrHw_ReadVddaMv(void);

/* Open-input probe of one channel: charges the selected input through PC4's
 * internal pull-up, reads it, then the same with the pull-down. A floating
 * input follows the pulls; a real sensor output does not. Leaves PC4 as a
 * plain analog input again. Codes are single 14-bit conversions.
 */
void TmrHw_PullProbe(uint32_t channel, uint32_t *code_pullup, uint32_t *code_pulldown);

/* ---- Fast scan (tmr_fast image) --------------------------------------------
 * Register-level scanning of all 16 inputs. Each conversion is started on the
 * ADC registers; as soon as the ADC has finished sampling (EOSMP) the mux is
 * switched to the next input, so the mux settles while the conversion of the
 * previous input is still running. Timing uses the CPU cycle counter (DWT).
 */
typedef struct
{
    uint32_t sampling;       /* LL_ADC_SAMPLINGTIME_xxCYCLES (ADC clock = 16 MHz)  */
    uint32_t samples;        /* conversions added up per input, 1 or more          */
    uint32_t prewait_cycles; /* extra wait after a mux change before the next start,
                              * in CPU cycles (160 per us)                         */
} tmr_fast_cfg_t;

/* Turns the cycle counter on and enables the ADC (one normal conversion). */
void TmrHw_FastBegin(void);
/* CPU cycles per microsecond (SystemCoreClock / 1e6). */
uint32_t TmrHw_CyclesPerUs(void);
/* Sets the ADC sampling time of the sensor channel. Not while a scan runs. */
void TmrHw_FastSetSampling(uint32_t ll_sampling);
/* Puts the mux on the first input of the next scan (0, or 15 when `reverse`) and
 * waits `wait_us` so the first reading of the next scan is as clean as the rest.
 */
void TmrHw_FastPrime(uint8_t reverse, uint32_t wait_us);
/* One scan of all 16 inputs, in order 0..15 (or 15..0 when `reverse`).
 * sums[ch] = sum of cfg->samples 14-bit codes. Leaves the mux on the first input
 * of the next scan in the same direction. Returns the CPU cycles it took.
 * *min_settle_cycles (may be NULL) gets the shortest time seen between a mux
 * change and the end of the sampling window that follows it.
 */
uint32_t TmrHw_FastScan(const tmr_fast_cfg_t *cfg, uint8_t reverse, uint32_t sums[16],
                        uint32_t *min_settle_cycles);

/* SW3 (PC13) held down. */
uint8_t TmrHw_ButtonDown(void);
/* Red LED D1 (PA5). */
void TmrHw_Led(uint8_t on);

#endif
