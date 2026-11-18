/* tmr_config.h - every adjustable setting of the TMR fast-scan test image.
 * DIAGNOSTIC IMAGE ONLY. The keyboard's board_config.h is not used for these
 * and is not changed. Copied from 2026-11-18_tmr_diag, which stays untouched;
 * the new settings are in the "Fast scan" block at the end of this file.
 *
 * Signal path (Devboard RevK + Numpad RevE):
 *   numpad sensor Un pin 1 (TMR_OUTn) -> numpad J1 -> jumper wire ->
 *   devboard J8 hole labelled "n" -> 100 R + 4.7 nF RC -> CD74HC4067 input n-1
 *   -> mux COM -> PC4 = ADC1_IN13 (14-bit).
 * Mux select: S0 = PA7, S1 = PB0, S2 = PB1, S3 = PB2. Mux enable is tied to GND.
 */
#ifndef TMR_CONFIG_H
#define TMR_CONFIG_H

/* ---- Mux and ADC pins (from the RevK netlist) -------------------------- */
#define TMR_MUX_S0_PORT GPIOA
#define TMR_MUX_S0_PIN GPIO_PIN_7
#define TMR_MUX_S1_PORT GPIOB
#define TMR_MUX_S1_PIN GPIO_PIN_0
#define TMR_MUX_S2_PORT GPIOB
#define TMR_MUX_S2_PIN GPIO_PIN_1
#define TMR_MUX_S3_PORT GPIOB
#define TMR_MUX_S3_PIN GPIO_PIN_2
#define TMR_SIG_PORT GPIOC
#define TMR_SIG_PIN GPIO_PIN_4
#define TMR_SIG_PIN_NUMBER 4U       /* bit position of PC4 in MODER/PUPDR */
#define TMR_ADC_CHANNEL ADC_CHANNEL_13

/* User button SW3: PC13 to GND, no external pull-up (internal one used).
 * Pressing it re-runs the self-test, same as typing 't'.
 */
#define TMR_BUTTON_PORT GPIOC
#define TMR_BUTTON_PIN GPIO_PIN_13

/* ---- Channels ------------------------------------------------------------
 * 16 mux inputs. Channel c (0..15) is J8 label c+1 = numpad sensor U(c+1) =
 * key K(c+1). The numpad has 15 sensors; J8 label 16 has nothing on it and
 * should test OPEN, which proves the open-input check works.
 */
#define TMR_CHANNELS 16U
#define TMR_NUMPAD_KEYS 15U

/* Which numpad key (1..15, reading order: top row K1-K4 ... bottom row
 * K13-K15) is wired to J8 hole 1, 2, ... 16. 0 = nothing / unknown.
 * Only used for labels: every usable channel is tested whatever this says.
 *
 * Tyler's bench wiring (2026-11-18, from his description and photos): numpad
 * J1 TMR pins taken in header order (TMR_OUT1, 5, 9, 13, 2, 6, 10, 14, 15,
 * 11, 7, 3, 12, 8, 4) go to J8 holes 16, 14, 12, 10, 7, 5, 3, 1 (the row with
 * 3V3, skipping G and 3V3), then 15, 13, 11, 9, 8, 6, 4. Hole 2 is unused and
 * should test OPEN.
 * If the keys print the wrong K numbers, type 'm' in the console, press
 * K1..K15 in reading order, and paste the line it prints over this one.
 * Plain one-to-one wiring (TMR_OUTn to hole n) would be
 * {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0}.
 */
#define TMR_KEY_OF_CHANNEL {14, 0, 10, 4, 6, 8, 2, 12, 3, 13, 7, 9, 11, 5, 15, 1}

/* ---- Timing -------------------------------------------------------------- */
/* Wait after changing the mux address before sampling. RevK's 100 R / 4.7 nF
 * filter plus the mux's 50 pF settle to 14 bits in about 2.5 us; 10 us gives
 * margin for this test. The real keyboard can use less.
 */
#define TMR_MUX_SETTLE_US 10U
/* ADC samples averaged per channel per scan (each about 5-6 us). */
#define TMR_SAMPLES_PER_READ 4U
/* One full 16-channel scan this often (1 kHz is plenty for a test). */
#define TMR_SCAN_PERIOD_US 1000U
/* Open-input probe: how long PC4's internal pull-up/pull-down (~40 k) charges
 * the 4.7 nF filter capacitor before it is read (~16 time constants).
 */
#define TMR_PULL_SETTLE_US 3000U
/* Samples taken per channel for the noise figure in the self-test. */
#define TMR_NOISE_SAMPLES 256U
/* Live line ('l') and LED timing. */
#define TMR_LIVE_PERIOD_MS 250U

/* ---- Press detection (millivolts away from the resting voltage) ----------
 * A key counts as DOWN when its sensor moves this far from rest in EITHER
 * direction (magnet polarity and sensor rotation decide the sign), and UP
 * again when it comes back within TMR_RELEASE_MV. Set the press point to
 * about a third of the swing you see at bottom-out in the 's' summary.
 */
#define TMR_PRESS_MV 150
#define TMR_RELEASE_MV 100
/* A full press should move at least this far for a PASS in the summary. */
#define TMR_GOOD_SWING_MV 300
/* While a key is up and within this window, its rest value follows slow
 * drift (temperature) with a time constant of about TMR_DRIFT_DIV scans.
 */
#define TMR_DRIFT_WINDOW_MV 30
#define TMR_DRIFT_DIV 1024

/* ---- Self-test limits, in per-mille (0.1 %) of the measured VDDA ----------
 * The TMR2615 output rests at half its supply (the part's 500 mV/V offset),
 * so all limits are ratios of the supply instead of fixed millivolts.
 */
#define TMR_OPEN_HIGH_PERMILLE 800    /* pull-up reading above 80 % ...      */
#define TMR_OPEN_LOW_PERMILLE 200     /* ... and pull-down below 20 % = open */
#define TMR_DRIVEN_SPAN_PERMILLE 100  /* pulls move it < 10 %: something drives it */
#define TMR_NOPOWER_LOW_PERMILLE 100  /* pull-down < 10 % and pull-up 10-70 % */
#define TMR_NOPOWER_HIGH_PERMILLE 700 /*   looks like an output with no VDD   */
#define TMR_STUCK_LOW_PERMILLE 50     /* driven but under 5 % of VDDA        */
#define TMR_STUCK_HIGH_PERMILLE 950   /* driven but over 95 % of VDDA        */
#define TMR_REST_MIN_PERMILLE 250     /* healthy rest window: 25 % ...       */
#define TMR_REST_MAX_PERMILLE 750     /* ... to 75 % (nominal 50 %)          */

/* ---- USB IDs ----------------------------------------------------------------
 * ST demonstration VID like the keyboard (0483:5710) and the rate test
 * (0483:5711). Prototype only. tools/tmr-console.ps1 finds the COM port by
 * this pair, so change both together.
 */
#define TMR_USB_VID 0x0483U
#define TMR_USB_PID 0x5712U

/* ---- Fast scan, sweep and rapid trigger (tmr_fast image only) -------------
 * The old test scanned with blocking reads: 16 inputs took 730 us (10 us mux
 * settle + 4 averaged conversions per input). An 8K report goes out every
 * 125 us, so this image adds a register-level scan that starts each conversion
 * on the ADC registers and switches the mux while the previous conversion is
 * still running, plus a sweep ('x') that measures what each speed costs in
 * noise and in channel-to-channel "memory" before anything goes in the
 * keyboard.
 */
/* Scan period in fast mode ('f'): 125 us = one 8K USB microframe. */
#define TMR_FAST_PERIOD_US 125U
/* Full 16-input scans per row of the 'x' sweep. */
#define TMR_SWEEP_SCANS 2000U
/* The sweep's reference row waits this long after every mux change (cycles of
 * the CPU clock, 160 MHz): 20 us, plenty for 14-bit settling.
 */
#define TMR_SWEEP_REF_WAIT_US 20U
/* A channel only counts toward the memory estimate when the input scanned just
 * before it differs from it by at least this many mV (a switch's magnet puts
 * about 100 mV between neighbours).
 */
#define TMR_MEMORY_MIN_STEP_MV 40

/* Rapid trigger ('g'): DOWN at TMR_PRESS_MV from rest as before, but UP as soon
 * as the reading backs off TMR_RT_RELEASE_MV from the deepest point of this
 * press (or returns within TMR_RELEASE_MV of rest, whichever happens first).
 * While released but still deep, DOWN again after pressing TMR_RT_PRESS_MV
 * deeper than the shallowest point since the release. Below TMR_RT_REST_MV
 * the key counts as fully back at rest and the normal press point applies.
 * All far above the 2 mV peak-to-peak noise, far below the 400-600 mV swing.
 */
#define TMR_RT_RELEASE_MV 60
#define TMR_RT_PRESS_MV 60
#define TMR_RT_REST_MV 40

/* ADC trigger-frequency mode. 1 = LOW (as the original test: safe for any gap
 * between conversions). 0 = HIGH (only valid while the gap between two
 * conversions stays under the datasheet's tIdle; try it if the sweep shows
 * conversions slower than the clock cycles predict).
 */
#define TMR_ADC_TRIGGER_LOW 1

/* Settings used by fast mode ('f'). Choose them from the 'x' sweep: the
 * quietest row that still scans well inside TMR_FAST_PERIOD_US with a small
 * memory figure. Sampling time is an LL_ADC_SAMPLINGTIME_xxCYCLES value (the
 * ADC clock is 16 MHz, so 12 cycles = 0.75 us); samples are added up per input;
 * prewait is extra CPU cycles (160 per us) after each mux change.
 */
#define TMR_FAST_SAMPLING LL_ADC_SAMPLINGTIME_12CYCLES
#define TMR_FAST_SAMPLES 1U
#define TMR_FAST_PREWAIT_CYCLES 0U

#endif
