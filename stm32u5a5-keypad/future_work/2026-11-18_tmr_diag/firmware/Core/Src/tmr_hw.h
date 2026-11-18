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

/* SW3 (PC13) held down. */
uint8_t TmrHw_ButtonDown(void);
/* Red LED D1 (PA5). */
void TmrHw_Led(uint8_t on);

#endif
