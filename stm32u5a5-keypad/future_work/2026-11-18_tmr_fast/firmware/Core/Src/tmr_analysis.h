/* tmr_analysis.h - board-independent logic of the TMR sensor test.
 * DIAGNOSTIC IMAGE ONLY.
 *
 * Nothing in here touches hardware, so tests/host/harness_tmr.c can check the
 * classification and press detection on a PC, like the keyboard's host tests.
 */
#ifndef TMR_ANALYSIS_H
#define TMR_ANALYSIS_H

#include <stdint.h>

/* Result of the self-test for one mux channel. */
typedef enum
{
    TMR_ST_UNTESTED = 0,
    TMR_ST_OK,         /* driven, rests near half supply: usable              */
    TMR_ST_OFFSET,     /* driven, usable, but rests far from half supply      */
    TMR_ST_OPEN,       /* nothing drives the input (wire/joint/hole)          */
    TMR_ST_NOPOWER,    /* behaves like a sensor output with no 3.3 V          */
    TMR_ST_WEAK,       /* connected but barely driven (bad joint/loose wire)  */
    TMR_ST_STUCK_LOW,  /* driven to ~0 V                                      */
    TMR_ST_STUCK_HIGH, /* driven to ~3.3 V                                    */
} tmr_status_t;

/* Decide a channel's status from three readings, all in millivolts:
 *   pullup_mv / pulldown_mv: the input read right after PC4's internal
 *     pull-up / pull-down charged it. A floating input follows the pull;
 *     a sensor output holds its own voltage.
 *   rest_mv: the normal reading with no pull, key released.
 * vdda_mv is the measured ADC supply; every limit is a ratio of it
 * (see the TMR_*_PERMILLE settings in tmr_config.h).
 */
tmr_status_t TmrA_Classify(uint32_t vdda_mv, uint32_t pullup_mv, uint32_t pulldown_mv,
                           uint32_t rest_mv);

/* 1 if press detection should run on a channel with this status. */
uint8_t TmrA_StatusUsable(tmr_status_t s);

/* Short label (max 9 characters) and a one-line hint for the report. */
const char *TmrA_StatusName(tmr_status_t s);
const char *TmrA_StatusHint(tmr_status_t s);

/* Press tracking for one key. Voltages in microvolts keep slow drift
 * tracking smooth; everything printed is in millivolts.
 */
typedef struct
{
    int32_t rest_uv;       /* resting voltage, follows slow drift while up */
    int32_t delta_mv;      /* latest reading minus rest                    */
    int32_t peak_pos_mv;   /* largest positive delta seen since cleared    */
    int32_t peak_neg_mv;   /* largest negative delta seen since cleared    */
    int32_t press_peak_mv; /* signed peak of the current / last press      */
    uint32_t presses;      /* completed DOWN events since cleared          */
    uint32_t down_ms;      /* time of the latest DOWN                      */
    uint32_t hold_ms;      /* duration of the last completed press         */
    int32_t valley_mv;     /* rapid trigger: shallowest |delta| since the last UP */
    int32_t peak_abs_mv;   /* rapid trigger: deepest |delta| of the current press */
    uint8_t pressed;
} tmr_key_t;

typedef enum
{
    TMR_EV_NONE = 0,
    TMR_EV_DOWN,
    TMR_EV_UP,
} tmr_event_t;

/* Start tracking from a measured rest voltage; clears press statistics. */
void TmrA_KeyInit(tmr_key_t *k, int32_t rest_uv);
/* Move the rest point without clearing statistics (re-zero). */
void TmrA_KeyRezero(tmr_key_t *k, int32_t rest_uv);
/* Clear press statistics only. */
void TmrA_KeyClearStats(tmr_key_t *k);

/* Feed one reading. Press = moved at least press_mv from rest in either
 * direction; release = back within release_mv. Returns DOWN/UP on a change.
 */
tmr_event_t TmrA_KeyUpdate(tmr_key_t *k, int32_t uv, uint32_t now_ms, int32_t press_mv,
                           int32_t release_mv);

/* Rapid-trigger variant of TmrA_KeyUpdate (this image only; the original above
 * is unchanged and still used by the normal mode).
 *   DOWN: |delta| >= press_mv, and also >= valley + rt_press_mv, where valley is
 *         the shallowest |delta| since the last UP (0 once the key was back
 *         within rest_mv of rest). So a released-but-deep key needs a new
 *         rt_press_mv of travel to count as pressed again.
 *   UP:   |delta| has fallen rt_release_mv below the deepest point of this press,
 *         or is back within release_mv of rest.
 * Voltages in microvolts as for TmrA_KeyUpdate; everything else in millivolts.
 */
tmr_event_t TmrA_KeyUpdateRT(tmr_key_t *k, int32_t uv, uint32_t now_ms, int32_t press_mv,
                             int32_t release_mv, int32_t rt_press_mv, int32_t rt_release_mv,
                             int32_t rest_mv);

/* Largest movement either way since the statistics were cleared. */
int32_t TmrA_KeyMaxSwing(const tmr_key_t *k);

/* Final per-key verdict for the summary table. */
typedef enum
{
    TMR_VERDICT_FAULT = 0,   /* self-test failed: fix wiring/solder first  */
    TMR_VERDICT_UNPRESSED,   /* works electrically, not pressed yet        */
    TMR_VERDICT_WEAK_SWING,  /* pressed, but moved less than good_swing_mv */
    TMR_VERDICT_PASS,
} tmr_verdict_t;
tmr_verdict_t TmrA_Verdict(tmr_status_t s, const tmr_key_t *k, int32_t good_swing_mv);
const char *TmrA_VerdictName(tmr_verdict_t v);

/* Running statistics of raw ADC codes, for the noise figure. */
typedef struct
{
    uint32_t n, min, max;
    uint64_t sum, sum_sq;
} tmr_stats_t;
void TmrA_StatsReset(tmr_stats_t *s);
void TmrA_StatsAdd(tmr_stats_t *s, uint32_t code);
uint32_t TmrA_StatsMean(const tmr_stats_t *s);
/* Standard deviation in hundredths of a code (so 0.37 codes -> 37). */
uint32_t TmrA_StatsStdX100(const tmr_stats_t *s);

/* ADC code(s) -> microvolts. sum is the sum of `count` 14-bit conversions. */
int32_t TmrA_CodesToUv(uint32_t sum, uint32_t count, uint32_t vdda_mv);

#endif
