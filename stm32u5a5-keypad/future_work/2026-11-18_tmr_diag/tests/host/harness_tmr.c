/* PC test of tmr_analysis.c (classification, press detection, statistics).
 * Returns 0 when everything passes, otherwise the source line that failed,
 * like the keyboard's host tests. No C library needed.
 */
#include "tmr_analysis.h"
#include "tmr_config.h"

#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
            return __LINE__;                                                                       \
    } while (0)

static int TestClassify(void)
{
    /* Healthy sensor: rests at half supply, pulls barely move it. */
    CHECK(TmrA_Classify(3300, 1655, 1648, 1651) == TMR_ST_OK);
    /* Ratiometric: same shape on a 3.0 V supply. */
    CHECK(TmrA_Classify(3000, 1503, 1497, 1500) == TMR_ST_OK);
    /* Nothing connected: follows both pulls. */
    CHECK(TmrA_Classify(3300, 3290, 3, 3) == TMR_ST_OPEN);
    /* Output present but its chip unpowered: pull-down to 0, pull-up stuck partway. */
    CHECK(TmrA_Classify(3300, 600, 5, 10) == TMR_ST_NOPOWER);
    /* Connected through a bad joint: pulls move it a lot, but not rail to rail. */
    CHECK(TmrA_Classify(3300, 2000, 1300, 1650) == TMR_ST_WEAK);
    /* Driven hard to a rail. */
    CHECK(TmrA_Classify(3300, 40, 2, 20) == TMR_ST_STUCK_LOW);
    CHECK(TmrA_Classify(3300, 3280, 3270, 3275) == TMR_ST_STUCK_HIGH);
    /* Driven, usable, but far from centre (81.7 %). */
    CHECK(TmrA_Classify(3300, 2700, 2690, 2695) == TMR_ST_OFFSET);
    CHECK(TmrA_StatusUsable(TMR_ST_OFFSET) && TmrA_StatusUsable(TMR_ST_OK));
    CHECK(!TmrA_StatusUsable(TMR_ST_OPEN) && !TmrA_StatusUsable(TMR_ST_STUCK_LOW));
    /* No supply measurement: refuse to judge. */
    CHECK(TmrA_Classify(0, 1650, 1650, 1650) == TMR_ST_UNTESTED);
    return 0;
}

static int TestPress(void)
{
    tmr_key_t k;
    const int32_t rest = 1650000;
    TmrA_KeyInit(&k, rest);
    CHECK(TmrA_KeyUpdate(&k, rest, 0, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_NONE);
    CHECK(TmrA_KeyUpdate(&k, rest + 149000, 1, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_NONE);
    CHECK(TmrA_KeyUpdate(&k, rest + 150000, 2, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_DOWN);
    CHECK(k.pressed && k.presses == 1);
    /* Bounce/hold inside the hysteresis band produces no events. */
    CHECK(TmrA_KeyUpdate(&k, rest + 400000, 3, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_NONE);
    CHECK(TmrA_KeyUpdate(&k, rest + 101000, 4, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_NONE);
    CHECK(TmrA_KeyUpdate(&k, rest + 160000, 5, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_NONE);
    CHECK(TmrA_KeyUpdate(&k, rest + 100000, 12, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_UP);
    CHECK(!k.pressed && k.hold_ms == 10 && k.press_peak_mv == 400 && k.peak_pos_mv == 400);

    /* Opposite magnet polarity counts the same. */
    CHECK(TmrA_KeyUpdate(&k, rest - 200000, 20, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_DOWN);
    CHECK(k.press_peak_mv == -200 && k.peak_neg_mv == -200 && k.presses == 2);
    CHECK(TmrA_KeyUpdate(&k, rest - 50000, 30, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_UP);
    CHECK(TmrA_KeyMaxSwing(&k) == 400);

    /* Verdicts. */
    CHECK(TmrA_Verdict(TMR_ST_OPEN, &k, TMR_GOOD_SWING_MV) == TMR_VERDICT_FAULT);
    CHECK(TmrA_Verdict(TMR_ST_OK, &k, TMR_GOOD_SWING_MV) == TMR_VERDICT_PASS);
    CHECK(TmrA_Verdict(TMR_ST_OK, &k, 500) == TMR_VERDICT_WEAK_SWING);
    TmrA_KeyClearStats(&k);
    CHECK(TmrA_Verdict(TMR_ST_OK, &k, TMR_GOOD_SWING_MV) == TMR_VERDICT_UNPRESSED);
    return 0;
}

static int TestDrift(void)
{
    tmr_key_t k;
    const int32_t rest = 1650000;
    TmrA_KeyInit(&k, rest);
    /* A steady 20 mV offset is followed slowly, never reported as a press. */
    for (uint32_t i = 0; i < 20000; i++)
        CHECK(TmrA_KeyUpdate(&k, rest + 20000, i, TMR_PRESS_MV, TMR_RELEASE_MV) == TMR_EV_NONE);
    CHECK(k.rest_uv > rest + 18000 && k.rest_uv <= rest + 20000);

    /* A very slow press (1 mV per scan) must still be detected: drift
     * tracking stops once the reading leaves the +/-30 mV window.
     */
    TmrA_KeyInit(&k, rest);
    int down = 0;
    for (int32_t mv = 0; mv <= 300 && !down; mv++)
        down = TmrA_KeyUpdate(&k, rest + mv * 1000, (uint32_t)mv, TMR_PRESS_MV, TMR_RELEASE_MV) ==
               TMR_EV_DOWN;
    CHECK(down);
    return 0;
}

static int TestStats(void)
{
    tmr_stats_t s;
    TmrA_StatsReset(&s);
    for (uint32_t i = 0; i < 128; i++)
    {
        TmrA_StatsAdd(&s, 8190);
        TmrA_StatsAdd(&s, 8192);
    }
    CHECK(s.n == 256 && s.min == 8190 && s.max == 8192);
    CHECK(TmrA_StatsMean(&s) == 8191);
    CHECK(TmrA_StatsStdX100(&s) == 100); /* exactly one code */
    TmrA_StatsReset(&s);
    for (uint32_t i = 0; i < 256; i++)
        TmrA_StatsAdd(&s, 5000);
    CHECK(TmrA_StatsStdX100(&s) == 0);

    /* Codes to microvolts: full scale = VDDA, half scale ~ VDDA/2. */
    CHECK(TmrA_CodesToUv(16383U * 4U, 4, 3300) == 3300000);
    CHECK(TmrA_CodesToUv(0, 4, 3300) == 0);
    int32_t half = TmrA_CodesToUv(8192, 1, 3300);
    CHECK(half > 1650000 && half < 1650300);
    CHECK(TmrA_CodesToUv(5, 0, 3300) == 0);
    return 0;
}

int main(void)
{
    int r;
    if ((r = TestClassify()) != 0)
        return r;
    if ((r = TestPress()) != 0)
        return r;
    if ((r = TestDrift()) != 0)
        return r;
    if ((r = TestStats()) != 0)
        return r;
    return 0;
}
