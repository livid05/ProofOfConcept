/* tmr_diag.c - TMR sensor test, shown on a USB COM port.
 * *** DIAGNOSTIC IMAGE ONLY *** Replaces key_matrix.c in this build: the
 * shared main.c calls Keyboard_Task() forever, and here that runs the test.
 *
 * At start-up (and on 't' or the SW3 button) every J8 input gets:
 *   1. an open-input probe (PC4 pull-up, then pull-down; see tmr_hw.c),
 *   2. a resting-voltage and noise measurement (256 samples),
 *   3. a status: ok / OFF-CTR / OPEN / NO POWER? / WEAK / STUCK 0V / STUCK 3V3.
 * Then all 16 inputs are scanned every 1 ms. A key is DOWN when its sensor
 * moves TMR_PRESS_MV from rest (either direction) and UP within
 * TMR_RELEASE_MV. Every press and release is printed.
 *
 * Console keys: h help, t self-test, s summary, l live line on/off,
 * r raw voltages, z re-zero (hands off), c clear counts, m learn wiring order.
 *
 * tmr_fast additions (this copy; 2026-11-18_tmr_diag is unchanged):
 *   x  speed sweep: for each fast-scan setting prints scan time, noise and the
 *      channel-to-channel "memory" against a slow, fully settled reference
 *   f  fast mode on/off: register-level scan every TMR_FAST_PERIOD_US (8 kHz)
 *   g  rapid trigger on/off: UP when the reading backs off TMR_RT_RELEASE_MV
 *      from the press's deepest point, DOWN again after TMR_RT_PRESS_MV deeper
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "key_clock.h"
#include "keyboard.h"
#include "main.h"
#include "tmr_analysis.h"
#include "tmr_config.h"
#include "tmr_hw.h"
#include "usb_cdc_diag.h"

typedef struct
{
    tmr_status_t status;
    uint32_t pullup_mv, pulldown_mv, rest_mv;
    uint32_t noise_pp_uv, noise_rms_uv;
} channel_test_t;

static const uint8_t key_of_channel[TMR_CHANNELS] = TMR_KEY_OF_CHANNEL;
static channel_test_t test[TMR_CHANNELS];
static tmr_key_t keys[TMR_CHANNELS]; /* indexed by channel (J8 hole - 1) */
static uint32_t vdda_mv;
static uint8_t vdda_measured;
static uint32_t init_error;
static uint8_t tested, live;
static uint8_t learn_active, learn_count, learned_key[TMR_CHANNELS];
static uint8_t fast_mode, rt_mode, fast_begun, fast_dirty;

/* Debug counters (also visible in a debugger). */
volatile uint32_t tmr_scan_count, tmr_scan_last_us, tmr_scan_max_us;

static char line[200];

/* ---- Output ------------------------------------------------------------- */
static void Out(const char *s)
{
    Cdc_Write(s, (uint32_t)strlen(s));
    Cdc_Task();
}

static void Outf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    Out(line);
}

static unsigned long CodeToMv(uint32_t code)
{
    return (unsigned long)((TmrA_CodesToUv(code, 1, vdda_mv) + 500) / 1000);
}

/* "K12" or "--" for a channel with no key assigned. */
static const char *KeyLabel(uint32_t ch)
{
    static char buf[2][6];
    static uint8_t which;
    char *b = buf[which ^= 1U];
    if (key_of_channel[ch] == 0U)
        strcpy(b, "--");
    else
        snprintf(b, sizeof buf[0], "K%u", (unsigned)key_of_channel[ch]);
    return b;
}

static uint32_t UsableCount(void)
{
    uint32_t n = 0;
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
        n += TmrA_StatusUsable(test[c].status);
    return n;
}

static uint32_t PressedChannelCount(void)
{
    uint32_t n = 0;
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
        n += (TmrA_StatusUsable(test[c].status) && keys[c].presses > 0U);
    return n;
}

/* ---- Self-test ------------------------------------------------------------ */
static void SelfTest(void)
{
    uint32_t v = TmrHw_ReadVddaMv();
    vdda_measured = (v >= 2700U && v <= 3700U);
    if (vdda_measured)
        vdda_mv = v;
    else if (vdda_mv == 0U)
        vdda_mv = 3300U; /* fall back to nominal; the report says so */

    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        uint32_t up, down;
        TmrHw_PullProbe(c, &up, &down);
        /* Give a real sensor time to pull the filter capacitor back. */
        TmrHw_Select(c);
        TmrHw_WaitUs(300);

        tmr_stats_t st;
        TmrA_StatsReset(&st);
        for (uint32_t i = 0; i < TMR_NOISE_SAMPLES; i++)
            TmrA_StatsAdd(&st, TmrHw_ReadCode());
        int32_t rest_uv = TmrA_CodesToUv((uint32_t)st.sum, st.n, vdda_mv);

        channel_test_t *t = &test[c];
        t->pullup_mv = (uint32_t)CodeToMv(up);
        t->pulldown_mv = (uint32_t)CodeToMv(down);
        t->rest_mv = (uint32_t)((rest_uv + 500) / 1000);
        t->noise_pp_uv = (uint32_t)TmrA_CodesToUv(st.max - st.min, 1, vdda_mv);
        t->noise_rms_uv = (uint32_t)(TmrA_CodesToUv(TmrA_StatsStdX100(&st), 1, vdda_mv) / 100);
        t->status = TmrA_Classify(vdda_mv, t->pullup_mv, t->pulldown_mv, t->rest_mv);
        TmrA_KeyInit(&keys[c], rest_uv);
        Cdc_Task();
    }
    tested = 1;
    fast_dirty = 1; /* the self-test moved the mux and the ADC sampling time */
}

static void PrintReport(void)
{
    Outf("\r\n=== TMR sensor self-test ===   VDDA %lu mV (%s)\r\n", (unsigned long)vdda_mv,
         vdda_measured ? "measured" : "NOT measured, assumed");
    Out("J8  key  status     rest mV  rest %   pull-up / pull-down mV   noise p-p / rms uV\r\n");
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        const channel_test_t *t = &test[c];
        unsigned long pm = (unsigned long)((t->rest_mv * 1000U + vdda_mv / 2U) / vdda_mv);
        Outf("%2lu  %-3s  %-9s  %7lu  %3lu.%lu%%   %7lu / %-7lu         %5lu / %lu\r\n",
             (unsigned long)(c + 1U), KeyLabel(c), TmrA_StatusName(t->status),
             (unsigned long)t->rest_mv, pm / 10U, pm % 10U, (unsigned long)t->pullup_mv,
             (unsigned long)t->pulldown_mv, (unsigned long)t->noise_pp_uv,
             (unsigned long)t->noise_rms_uv);
    }
    uint32_t problems = 0;
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        tmr_status_t s = test[c].status;
        if (s == TMR_ST_OK || (s == TMR_ST_OPEN && key_of_channel[c] == 0U))
            continue;
        if (!problems)
            Out("\r\nCheck:\r\n");
        problems++;
        Outf("  J8 %lu (%s) %s: %s\r\n", (unsigned long)(c + 1U), KeyLabel(c), TmrA_StatusName(s),
             TmrA_StatusHint(s));
    }
    Outf("\r\nResult: %lu of 16 inputs have a working sensor (numpad has %lu).",
         (unsigned long)UsableCount(), (unsigned long)TMR_NUMPAD_KEYS);
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
        if (key_of_channel[c] == 0U && test[c].status == TMR_ST_OPEN)
            Outf(" J8 %lu reads OPEN with nothing on it, so the open check works.",
                 (unsigned long)(c + 1U));
    Out("\r\nNow press each key fully, one at a time. Type s for the summary, h for help.\r\n");
}

static void PrintHelp(void)
{
    Out("\r\nKeys: t self-test (hands off)  s summary  l live line on/off  r raw mV\r\n"
        "      z re-zero (hands off)  c clear counts  m learn wiring order  h help\r\n"
        "      x speed sweep (hands off, ~5 s)  f fast scan on/off  g rapid trigger on/off\r\n"
        "SW3 button = self-test. LED: slow blink = USB ok, fast = no USB, on = key down.\r\n");
}

static void PrintBanner(void)
{
    Out("\r\nKeypad U5A5 - TMR fast-scan test (diagnostic image, not the keyboard)\r\n");
    if (init_error)
        Outf("!! ADC setup failed at step %lu. Sensor tests cannot run.\r\n",
             (unsigned long)init_error);
}

static void PrintSummary(void)
{
    Out("\r\nJ8  key  status     presses  peak +mV  peak -mV  verdict\r\n");
    uint32_t pass = 0;
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        const tmr_key_t *k = &keys[c];
        tmr_verdict_t v = TmrA_Verdict(test[c].status, k, TMR_GOOD_SWING_MV);
        if (test[c].status == TMR_ST_OPEN && key_of_channel[c] == 0U)
            continue; /* unused hole */
        pass += (v == TMR_VERDICT_PASS);
        Outf("%2lu  %-3s  %-9s  %7lu  %+8ld  %+8ld  %s\r\n", (unsigned long)(c + 1U), KeyLabel(c),
             TmrA_StatusName(test[c].status), (unsigned long)k->presses, (long)k->peak_pos_mv,
             (long)k->peak_neg_mv, TmrA_VerdictName(v));
    }
    Outf("Mode: %s scan, %s release.\r\n", fast_mode ? "FAST" : "slow", rt_mode ? "rapid-trigger" : "normal");
    Outf("PASS: %lu of %lu keys.  Scan: 16 inputs in %lu us (max %lu us), %lu scans.  "
         "USB bytes dropped: %lu\r\n",
         (unsigned long)pass, (unsigned long)TMR_NUMPAD_KEYS, (unsigned long)tmr_scan_last_us,
         (unsigned long)tmr_scan_max_us, (unsigned long)tmr_scan_count,
         (unsigned long)Cdc_Dropped());
    if (pass == TMR_NUMPAD_KEYS)
        Out("ALL KEYS PASS.\r\n");
}

static void PrintRaw(void)
{
    Out("raw mV:");
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        TmrHw_Select(c);
        TmrHw_WaitUs(TMR_MUX_SETTLE_US);
        int32_t uv = TmrA_CodesToUv(TmrHw_ReadSum(16), 16, vdda_mv);
        Outf(" %lu:%ld", (unsigned long)(c + 1U), (long)((uv + 500) / 1000));
    }
    Out("\r\n");
    fast_dirty = 1;
}

static void PrintLive(void)
{
    Out("dmV");
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        if (!TmrA_StatusUsable(test[c].status))
            Outf(" %lu:--", (unsigned long)(c + 1U));
        else
            Outf(" %lu:%+ld%s", (unsigned long)(c + 1U), (long)keys[c].delta_mv,
                 keys[c].pressed ? "*" : "");
    }
    Out("\r\n");
}

static void Rezero(void)
{
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        TmrHw_Select(c);
        TmrHw_WaitUs(TMR_MUX_SETTLE_US);
        TmrA_KeyRezero(&keys[c], TmrA_CodesToUv(TmrHw_ReadSum(64), 64, vdda_mv));
    }
    Out("Re-zeroed (keep hands off while doing this).\r\n");
    fast_dirty = 1;
}

static void PrintLearnedMap(void)
{
    Out("\r\nPaste this over the TMR_KEY_OF_CHANNEL line in tmr_config.h:\r\n"
        "#define TMR_KEY_OF_CHANNEL {");
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
        Outf("%u%s", (unsigned)learned_key[c], c + 1U < TMR_CHANNELS ? ", " : "}\r\n");
}

/* ---- Scanning --------------------------------------------------------------- */
static void Scan(uint32_t now_ms)
{
    int32_t uv[TMR_CHANNELS];
    tmr_event_t ev[TMR_CHANNELS];
    if (fast_mode)
    {
        uint32_t sums[TMR_CHANNELS];
        if (fast_dirty)
        {
            TmrHw_FastSetSampling(TMR_FAST_SAMPLING);
            TmrHw_FastPrime(0, TMR_SWEEP_REF_WAIT_US);
            fast_dirty = 0;
        }
        const tmr_fast_cfg_t cfg = {TMR_FAST_SAMPLING, TMR_FAST_SAMPLES, TMR_FAST_PREWAIT_CYCLES};
        uint32_t cycles = TmrHw_FastScan(&cfg, 0, sums, NULL);
        tmr_scan_last_us = cycles / TmrHw_CyclesPerUs();
        for (uint32_t c = 0; c < TMR_CHANNELS; c++)
            uv[c] = TmrA_CodesToUv(sums[c], TMR_FAST_SAMPLES, vdda_mv);
    }
    else
    {
        uint32_t t0 = KeyClock_NowUs();
        for (uint32_t c = 0; c < TMR_CHANNELS; c++)
        {
            TmrHw_Select(c);
            TmrHw_WaitUs(TMR_MUX_SETTLE_US);
            uv[c] = TmrA_CodesToUv(TmrHw_ReadSum(TMR_SAMPLES_PER_READ), TMR_SAMPLES_PER_READ,
                                   vdda_mv);
        }
        tmr_scan_last_us = KeyClock_NowUs() - t0;
    }
    if (tmr_scan_last_us > tmr_scan_max_us)
        tmr_scan_max_us = tmr_scan_last_us;
    tmr_scan_count++;

    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        if (!TmrA_StatusUsable(test[c].status))
            ev[c] = TMR_EV_NONE;
        else if (rt_mode)
            ev[c] = TmrA_KeyUpdateRT(&keys[c], uv[c], now_ms, TMR_PRESS_MV, TMR_RELEASE_MV,
                                     TMR_RT_PRESS_MV, TMR_RT_RELEASE_MV, TMR_RT_REST_MV);
        else
            ev[c] = TmrA_KeyUpdate(&keys[c], uv[c], now_ms, TMR_PRESS_MV, TMR_RELEASE_MV);
    }

    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        if (ev[c] == TMR_EV_DOWN)
        {
            /* How far the other (released) sensors moved in the same scan:
             * a big number means crosstalk, a shared wire or a magnet that
             * reaches the neighbouring sensor.
             */
            int32_t others = 0;
            for (uint32_t o = 0; o < TMR_CHANNELS; o++)
            {
                if (o == c || !TmrA_StatusUsable(test[o].status) || keys[o].pressed)
                    continue;
                int32_t d = keys[o].delta_mv < 0 ? -keys[o].delta_mv : keys[o].delta_mv;
                if (d > others)
                    others = d;
            }
            Outf("%-3s (J8 %2lu) DOWN %+5ld mV   others moved <= %ld mV\r\n", KeyLabel(c),
                 (unsigned long)(c + 1U), (long)keys[c].delta_mv, (long)others);
            if (learn_active && learned_key[c] == 0U)
            {
                learned_key[c] = ++learn_count;
                Outf("    learn: J8 %lu = K%u\r\n", (unsigned long)(c + 1U), (unsigned)learn_count);
                if (learn_count >= TMR_NUMPAD_KEYS)
                {
                    learn_active = 0;
                    PrintLearnedMap();
                }
            }
        }
        else if (ev[c] == TMR_EV_UP)
        {
            Outf("%-3s (J8 %2lu) up   peak %+5ld mV, held %lu ms   [%lu of %lu keys pressed so far]\r\n",
                 KeyLabel(c), (unsigned long)(c + 1U), (long)keys[c].press_peak_mv,
                 (unsigned long)keys[c].hold_ms, (unsigned long)PressedChannelCount(),
                 (unsigned long)TMR_NUMPAD_KEYS);
        }
    }
}

/* ---- Speed sweep ('x') --------------------------------------------------------
 * Each row scans all 16 inputs TMR_SWEEP_SCANS times with one fast-scan setting.
 * Row 0 is the reference: 68-cycle sampling, 4 conversions added up, and a 20 us
 * wait after every mux change, so it is as settled as this hardware gets. Every
 * other row is compared with it:
 *   noise rms / worst p-p  per-input spread of one reading, microvolts
 *   shift vs ref           largest change of an input's average against row 0
 *   memory ppm             how much of the previous input's voltage leaks into
 *                          the next reading (least-squares fit over inputs whose
 *                          predecessor differs by TMR_MEMORY_MIN_STEP_MV or more;
 *                          "steps" = how many inputs qualified). 1000 ppm = 0.1 %.
 *   settle us              shortest time from a mux change to the end of the
 *                          sampling window that follows (measured, not assumed)
 * The memory figure needs some input-to-input differences: a switch in a socket
 * puts about 100 mV between neighbours, so keep a few switches in.
 */
typedef struct
{
    uint32_t sampling;
    uint32_t samples;
    uint32_t prewait_us;
    const char *label;
} sweep_row_t;

static const sweep_row_t sweep_rows[] = {
    {LL_ADC_SAMPLINGTIME_68CYCLES, 4U, TMR_SWEEP_REF_WAIT_US, "ref 68c x4 wait"},
    {LL_ADC_SAMPLINGTIME_68CYCLES, 1U, 0U, "68c x1"},
    {LL_ADC_SAMPLINGTIME_36CYCLES, 1U, 0U, "36c x1"},
    {LL_ADC_SAMPLINGTIME_20CYCLES, 1U, 0U, "20c x1"},
    {LL_ADC_SAMPLINGTIME_12CYCLES, 1U, 0U, "12c x1"},
    {LL_ADC_SAMPLINGTIME_5CYCLES, 1U, 0U, "5c x1"},
    {LL_ADC_SAMPLINGTIME_12CYCLES, 2U, 0U, "12c x2"},
    {LL_ADC_SAMPLINGTIME_12CYCLES, 4U, 0U, "12c x4"},
    {LL_ADC_SAMPLINGTIME_5CYCLES, 4U, 0U, "5c x4"},
};
#define SWEEP_ROWS (sizeof sweep_rows / sizeof sweep_rows[0])

static void EnsureFastBegin(void)
{
    if (!fast_begun)
    {
        TmrHw_FastBegin();
        fast_begun = 1;
    }
}

static void RunSweep(void)
{
    static tmr_stats_t st[TMR_CHANNELS];
    static int32_t ref_uv[TMR_CHANNELS];
    uint32_t sums[TMR_CHANNELS];

    if (!tested || init_error)
    {
        Out("Run the self-test first (t).\r\n");
        return;
    }
    EnsureFastBegin();
    const uint32_t cpu_us = TmrHw_CyclesPerUs();

    Outf("\r\nSweep: %lu scans per row, forward order. Hands off, about 5 s...\r\n",
         (unsigned long)TMR_SWEEP_SCANS);
    Out("row              scan us avg/max  settle us  rms uV  p-p uV  shift uV  memory ppm (steps)\r\n");

    for (uint32_t r = 0; r < SWEEP_ROWS; r++)
    {
        const sweep_row_t *row = &sweep_rows[r];
        const tmr_fast_cfg_t cfg = {row->sampling, row->samples, row->prewait_us * cpu_us};
        TmrHw_FastSetSampling(row->sampling);
        TmrHw_FastPrime(0, TMR_SWEEP_REF_WAIT_US);
        for (uint32_t c = 0; c < TMR_CHANNELS; c++)
            TmrA_StatsReset(&st[c]);

        uint64_t total_cycles = 0;
        uint32_t max_cycles = 0, min_settle = UINT32_MAX;
        for (uint32_t n = 0; n < 50U + TMR_SWEEP_SCANS; n++) /* first 50 are warm-up */
        {
            uint32_t settle;
            uint32_t cyc = TmrHw_FastScan(&cfg, 0, sums, &settle);
            if (n < 50U)
                continue;
            total_cycles += cyc;
            if (cyc > max_cycles)
                max_cycles = cyc;
            if (settle < min_settle)
                min_settle = settle;
            for (uint32_t c = 0; c < TMR_CHANNELS; c++)
                TmrA_StatsAdd(&st[c], sums[c]);
        }

        uint32_t live = 0, rms_total = 0, pp_worst = 0;
        int32_t mean_uv[TMR_CHANNELS];
        for (uint32_t c = 0; c < TMR_CHANNELS; c++)
        {
            if (!TmrA_StatusUsable(test[c].status))
                continue;
            live++;
            mean_uv[c] = TmrA_CodesToUv((uint32_t)st[c].sum, st[c].n * cfg.samples, vdda_mv);
            rms_total += (uint32_t)TmrA_CodesToUv(TmrA_StatsStdX100(&st[c]), 100U * cfg.samples,
                                                  vdda_mv);
            uint32_t pp = (uint32_t)TmrA_CodesToUv(st[c].max - st[c].min, cfg.samples, vdda_mv);
            if (pp > pp_worst)
                pp_worst = pp;
            if (r == 0U)
                ref_uv[c] = mean_uv[c];
        }

        int32_t shift_max = 0, ppm = 0;
        uint32_t steps = 0;
        if (r != 0U)
        {
            int64_t num = 0, den = 0;
            for (uint32_t c = 0; c < TMR_CHANNELS; c++)
            {
                if (!TmrA_StatusUsable(test[c].status))
                    continue;
                int32_t err = mean_uv[c] - ref_uv[c];
                int32_t aerr = err < 0 ? -err : err;
                if (aerr > shift_max)
                    shift_max = aerr;
                uint32_t p = (c + TMR_CHANNELS - 1U) % TMR_CHANNELS; /* scanned just before c */
                if (!TmrA_StatusUsable(test[p].status))
                    continue;
                int32_t step = ref_uv[p] - ref_uv[c];
                int32_t astep = step < 0 ? -step : step;
                if (astep < (int32_t)TMR_MEMORY_MIN_STEP_MV * 1000)
                    continue;
                num += (int64_t)err * step;
                den += (int64_t)step * step;
                steps++;
            }
            if (den != 0)
                ppm = (int32_t)((num * 1000000) / den);
        }

        uint32_t avg_us10 = (uint32_t)((total_cycles / TMR_SWEEP_SCANS) * 10U / cpu_us);
        uint32_t max_us10 = max_cycles * 10U / cpu_us;
        uint32_t settle100 = (min_settle * 100U + cpu_us / 2U) / cpu_us;
        char mem[24];
        if (r == 0U)
            strcpy(mem, "reference");
        else
            snprintf(mem, sizeof mem, "%+ld (%lu)", (long)ppm, (unsigned long)steps);
        Outf("%-16s %4lu.%lu/%4lu.%lu   %5lu.%02lu   %5lu  %6lu  %8ld  %s\r\n", row->label,
             (unsigned long)(avg_us10 / 10U), (unsigned long)(avg_us10 % 10U),
             (unsigned long)(max_us10 / 10U), (unsigned long)(max_us10 % 10U),
             (unsigned long)(settle100 / 100U), (unsigned long)(settle100 % 100U),
             (unsigned long)(live ? rms_total / live : 0U), (unsigned long)pp_worst, (long)shift_max,
             mem);
    }
    TmrHw_FastSetSampling(LL_ADC_SAMPLINGTIME_68CYCLES); /* back to the slow path's setting */
    fast_dirty = 1;
    Out("Done. Quieter and smaller memory is better; compare the scan time with the 125 us 8K frame.\r\n");
}

static void ToggleFast(void)
{
    fast_mode = !fast_mode;
    if (fast_mode)
    {
        EnsureFastBegin();
        fast_dirty = 1;
        Outf("Fast scan on: every %lu us, sampling setting and samples from tmr_config.h "
             "(TMR_FAST_*).\r\n", (unsigned long)TMR_FAST_PERIOD_US);
    }
    else
        Out("Fast scan off: back to the 1 ms blocking scan.\r\n");
    tmr_scan_max_us = 0;
}

static void ToggleRT(void)
{
    rt_mode = !rt_mode;
    for (uint32_t c = 0; c < TMR_CHANNELS; c++)
    {
        keys[c].pressed = 0;
        keys[c].valley_mv = 0;
    }
    Outf("Rapid trigger %s (release %d mV off the peak, press %d mV off the valley).\r\n",
         rt_mode ? "on" : "off", (int)TMR_RT_RELEASE_MV, (int)TMR_RT_PRESS_MV);
}

static void RunSelfTest(void)
{
    Out("\r\nSelf-test: hands off the keys...\r\n");
    SelfTest();
    PrintReport();
}

static void Command(int ch)
{
    if (init_error && ch != 'h' && ch != '?')
    {
        PrintBanner();
        return;
    }
    switch (ch)
    {
    case 't':
    case 'T':
    case '\r':
        RunSelfTest();
        break;
    case 's':
    case 'S':
        PrintSummary();
        break;
    case 'l':
    case 'L':
        live = !live;
        Outf("Live line %s.\r\n", live ? "on" : "off");
        break;
    case 'r':
    case 'R':
        PrintRaw();
        break;
    case 'z':
    case 'Z':
        Rezero();
        break;
    case 'c':
    case 'C':
        for (uint32_t c = 0; c < TMR_CHANNELS; c++)
            TmrA_KeyClearStats(&keys[c]);
        Out("Press counts and peaks cleared.\r\n");
        break;
    case 'm':
    case 'M':
        if (learn_active)
        {
            learn_active = 0;
            PrintLearnedMap();
            break;
        }
        memset(learned_key, 0, sizeof learned_key);
        learn_count = 0;
        learn_active = 1;
        Out("Learn wiring: press K1, K2, ... K15 in reading order (top-left first,\r\n"
            "row by row). Type m again to stop early.\r\n");
        break;
    case 'x':
    case 'X':
        RunSweep();
        break;
    case 'f':
    case 'F':
        ToggleFast();
        break;
    case 'g':
    case 'G':
        ToggleRT();
        break;
    case 'h':
    case 'H':
    case '?':
        PrintHelp();
        break;
    default:
        break;
    }
}

/* ---- Main loop hook ------------------------------------------------------------ */
void Keyboard_Task(void)
{
    static uint8_t started, button_was_down, greeted;
    static uint32_t next_scan_us, last_live_ms, seen_opens, button_ms;

    if (!started)
    {
        started = 1;
        init_error = TmrHw_Init();
        if (!init_error)
        {
            HAL_Delay(50); /* sensors' 100 us power-on time, with margin */
            SelfTest();
        }
        next_scan_us = KeyClock_NowUs();
    }

    Cdc_Task();
    uint32_t now_ms = HAL_GetTick();

    /* A terminal opened the port: start it with a fresh copy of everything. */
    if (Cdc_OpenCount() != seen_opens)
    {
        seen_opens = Cdc_OpenCount();
        greeted = 1;
        Cdc_Flush();
        PrintBanner();
        if (tested)
            PrintReport();
        PrintHelp();
    }

    int c;
    while ((c = Cdc_GetChar()) >= 0)
    {
        if (!greeted) /* terminal that never sets DTR: greet on first key */
        {
            greeted = 1;
            PrintBanner();
            if (tested)
                PrintReport();
        }
        Command(c);
    }

    if (!init_error && tested && (int32_t)(KeyClock_NowUs() - next_scan_us) >= 0)
    {
        const uint32_t period_us = fast_mode ? TMR_FAST_PERIOD_US : TMR_SCAN_PERIOD_US;
        next_scan_us += period_us;
        if ((int32_t)(KeyClock_NowUs() - next_scan_us) > (int32_t)(10U * period_us))
            next_scan_us = KeyClock_NowUs(); /* fell far behind (long print): resync */
        Scan(now_ms);
    }

    if (live && (uint32_t)(now_ms - last_live_ms) >= TMR_LIVE_PERIOD_MS)
    {
        last_live_ms = now_ms;
        PrintLive();
    }

    /* SW3: 30 ms debounce, acts on press. */
    uint8_t down = TmrHw_ButtonDown();
    if (down != button_was_down && (uint32_t)(now_ms - button_ms) >= 30U)
    {
        button_was_down = down;
        button_ms = now_ms;
        if (down && !init_error)
            RunSelfTest();
    }

    /* LED: on while any key is down, else slow blink (USB configured) or fast blink. */
    uint8_t any = 0;
    for (uint32_t k = 0; k < TMR_CHANNELS; k++)
        any |= keys[k].pressed;
    uint32_t period = Cdc_Configured() ? 500U : 125U;
    TmrHw_Led(any || init_error ? (init_error ? (uint8_t)((now_ms / 100U) & 1U) : 1U)
                                : (uint8_t)((now_ms / period) & 1U));
}
