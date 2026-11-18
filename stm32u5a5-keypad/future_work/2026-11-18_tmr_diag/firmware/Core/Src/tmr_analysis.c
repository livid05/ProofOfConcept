/* tmr_analysis.c - board-independent logic of the TMR sensor test.
 * DIAGNOSTIC IMAGE ONLY. See tmr_analysis.h. No HAL, no globals: the same
 * file is compiled into the firmware and into the PC test harness.
 */
#include "tmr_analysis.h"
#include "tmr_config.h"

/* Value as a share of the supply in 0.1 % steps (1000 = equal to VDDA). */
static uint32_t PerMille(uint32_t mv, uint32_t vdda_mv)
{
    return (uint32_t)(((uint64_t)mv * 1000U + vdda_mv / 2U) / vdda_mv);
}

tmr_status_t TmrA_Classify(uint32_t vdda_mv, uint32_t pullup_mv, uint32_t pulldown_mv,
                           uint32_t rest_mv)
{
    if (vdda_mv == 0U)
        return TMR_ST_UNTESTED;

    uint32_t up = PerMille(pullup_mv, vdda_mv);
    uint32_t down = PerMille(pulldown_mv, vdda_mv);

    /* Floating input: it simply follows whichever pull was applied. */
    if (up >= TMR_OPEN_HIGH_PERMILLE && down <= TMR_OPEN_LOW_PERMILLE)
        return TMR_ST_OPEN;

    /* How far the two pulls managed to move it. A real sensor output (it can
     * drive 10 k or more) moves only a few millivolts against ~40 k.
     */
    uint32_t span = up > down ? up - down : down - up;
    if (span > TMR_DRIVEN_SPAN_PERMILLE)
    {
        /* Pull-down reaches ~0 but pull-up gets stuck partway: the classic
         * shape of an output whose chip has no supply (the pull-up current
         * leaks through the chip's protection diode into its empty VDD rail).
         */
        if (down <= TMR_NOPOWER_LOW_PERMILLE && up > TMR_NOPOWER_LOW_PERMILLE &&
            up < TMR_NOPOWER_HIGH_PERMILLE)
            return TMR_ST_NOPOWER;
        return TMR_ST_WEAK;
    }

    /* Driven: judge where it rests. */
    uint32_t rest = PerMille(rest_mv, vdda_mv);
    if (rest < TMR_STUCK_LOW_PERMILLE)
        return TMR_ST_STUCK_LOW;
    if (rest > TMR_STUCK_HIGH_PERMILLE)
        return TMR_ST_STUCK_HIGH;
    if (rest < TMR_REST_MIN_PERMILLE || rest > TMR_REST_MAX_PERMILLE)
        return TMR_ST_OFFSET;
    return TMR_ST_OK;
}

uint8_t TmrA_StatusUsable(tmr_status_t s)
{
    return s == TMR_ST_OK || s == TMR_ST_OFFSET;
}

const char *TmrA_StatusName(tmr_status_t s)
{
    switch (s)
    {
    case TMR_ST_OK:
        return "ok";
    case TMR_ST_OFFSET:
        return "OFF-CTR";
    case TMR_ST_OPEN:
        return "OPEN";
    case TMR_ST_NOPOWER:
        return "NO POWER?";
    case TMR_ST_WEAK:
        return "WEAK";
    case TMR_ST_STUCK_LOW:
        return "STUCK 0V";
    case TMR_ST_STUCK_HIGH:
        return "STUCK 3V3";
    default:
        return "untested";
    }
}

const char *TmrA_StatusHint(tmr_status_t s)
{
    switch (s)
    {
    case TMR_ST_OK:
        return "";
    case TMR_ST_OFFSET:
        return "works, but rests far from half supply: magnet very close at rest, or a weak sensor";
    case TMR_ST_OPEN:
        return "nothing drives this input: jumper missing, wrong J8 hole, or sensor pin 1 not soldered";
    case TMR_ST_NOPOWER:
        return "output there but unpowered: check numpad J1 pin 25 (+3.3V) to devboard 3V3, and sensor pin 2";
    case TMR_ST_WEAK:
        return "barely driven: cracked/cold joint on the sensor, or a loose jumper";
    case TMR_ST_STUCK_LOW:
        return "held at 0 V: no 3.3 V at the sensor, output shorted to GND, or sensor rotated";
    case TMR_ST_STUCK_HIGH:
        return "held at 3.3 V: output shorted to 3.3 V, or a magnet right on the sensor";
    default:
        return "";
    }
}

void TmrA_KeyClearStats(tmr_key_t *k)
{
    k->peak_pos_mv = 0;
    k->peak_neg_mv = 0;
    k->press_peak_mv = 0;
    k->presses = 0;
    k->hold_ms = 0;
}

void TmrA_KeyInit(tmr_key_t *k, int32_t rest_uv)
{
    k->rest_uv = rest_uv;
    k->delta_mv = 0;
    k->pressed = 0;
    k->down_ms = 0;
    TmrA_KeyClearStats(k);
}

void TmrA_KeyRezero(tmr_key_t *k, int32_t rest_uv)
{
    k->rest_uv = rest_uv;
    k->delta_mv = 0;
    k->pressed = 0;
}

static int32_t Abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

tmr_event_t TmrA_KeyUpdate(tmr_key_t *k, int32_t uv, uint32_t now_ms, int32_t press_mv,
                           int32_t release_mv)
{
    int32_t d_uv = uv - k->rest_uv;
    int32_t d = d_uv / 1000; /* toward zero: never overstates a movement */
    k->delta_mv = d;
    if (d > k->peak_pos_mv)
        k->peak_pos_mv = d;
    if (d < k->peak_neg_mv)
        k->peak_neg_mv = d;

    if (!k->pressed)
    {
        if (Abs32(d) >= press_mv)
        {
            k->pressed = 1;
            k->presses++;
            k->down_ms = now_ms;
            k->press_peak_mv = d;
            return TMR_EV_DOWN;
        }
        /* Only follow drift while the reading is quiet and close to rest,
         * so a slow press can never be "learned" as the new rest point.
         */
        if (Abs32(d) <= TMR_DRIFT_WINDOW_MV)
            k->rest_uv += d_uv / TMR_DRIFT_DIV;
        return TMR_EV_NONE;
    }

    if (Abs32(d) > Abs32(k->press_peak_mv))
        k->press_peak_mv = d;
    if (Abs32(d) <= release_mv)
    {
        k->pressed = 0;
        k->hold_ms = now_ms - k->down_ms;
        return TMR_EV_UP;
    }
    return TMR_EV_NONE;
}

int32_t TmrA_KeyMaxSwing(const tmr_key_t *k)
{
    int32_t neg = -k->peak_neg_mv;
    return k->peak_pos_mv > neg ? k->peak_pos_mv : neg;
}

tmr_verdict_t TmrA_Verdict(tmr_status_t s, const tmr_key_t *k, int32_t good_swing_mv)
{
    if (!TmrA_StatusUsable(s))
        return TMR_VERDICT_FAULT;
    if (k->presses == 0U)
        return TMR_VERDICT_UNPRESSED;
    if (TmrA_KeyMaxSwing(k) < good_swing_mv)
        return TMR_VERDICT_WEAK_SWING;
    return TMR_VERDICT_PASS;
}

const char *TmrA_VerdictName(tmr_verdict_t v)
{
    switch (v)
    {
    case TMR_VERDICT_PASS:
        return "PASS";
    case TMR_VERDICT_WEAK_SWING:
        return "weak swing";
    case TMR_VERDICT_UNPRESSED:
        return "press it";
    default:
        return "FAULT";
    }
}

void TmrA_StatsReset(tmr_stats_t *s)
{
    s->n = 0;
    s->min = UINT32_MAX;
    s->max = 0;
    s->sum = 0;
    s->sum_sq = 0;
}

void TmrA_StatsAdd(tmr_stats_t *s, uint32_t code)
{
    s->n++;
    if (code < s->min)
        s->min = code;
    if (code > s->max)
        s->max = code;
    s->sum += code;
    s->sum_sq += (uint64_t)code * code;
}

uint32_t TmrA_StatsMean(const tmr_stats_t *s)
{
    return s->n ? (uint32_t)((s->sum + s->n / 2U) / s->n) : 0U;
}

static uint32_t Isqrt64(uint64_t v)
{
    uint64_t r = 0, bit = (uint64_t)1 << 62;
    while (bit > v)
        bit >>= 2;
    while (bit)
    {
        if (v >= r + bit)
        {
            v -= r + bit;
            r = (r >> 1) + bit;
        }
        else
            r >>= 1;
        bit >>= 2;
    }
    return (uint32_t)r;
}

uint32_t TmrA_StatsStdX100(const tmr_stats_t *s)
{
    if (s->n < 2U)
        return 0;
    /* variance * n^2 = n*sum_sq - sum^2 ; std*100 = sqrt(that * 10000) / n */
    uint64_t n = s->n;
    uint64_t var_n2 = n * s->sum_sq - s->sum * s->sum;
    return (uint32_t)(Isqrt64(var_n2 * 10000U) / n);
}

int32_t TmrA_CodesToUv(uint32_t sum, uint32_t count, uint32_t vdda_mv)
{
    if (count == 0U)
        return 0;
    /* 14-bit full scale is 16383 codes = VDDA. */
    return (int32_t)(((uint64_t)sum * vdda_mv * 1000U + (16383U * count) / 2U) /
                     (16383U * (uint64_t)count));
}
