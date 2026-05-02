#pragma once
#include "quantum.h"

// ─── HC4067 pin definitions ──────────────────────────────────────────────────
// Override any of these in config.h before including this header.
#ifndef MUX_S0_PIN
#    define MUX_S0_PIN GP22
#endif
#ifndef MUX_S1_PIN
#    define MUX_S1_PIN GP26
#endif
#ifndef MUX_S2_PIN
#    define MUX_S2_PIN GP27
#endif
#ifndef MUX_S3_PIN
#    define MUX_S3_PIN GP28
#endif
#ifndef MUX_SIG_PIN
#    define MUX_SIG_PIN GP29
#endif

// ─── Tuning ──────────────────────────────────────────────────────────────────
// Key fires when ADC drops this many percent below its calibrated resting value.
#ifndef MUX_THRESHOLD_PCT
#    define MUX_THRESHOLD_PCT 15
#endif

// Samples averaged per channel at boot to build the baseline.
#ifndef MUX_CALIBRATION_SAMPLES
#    define MUX_CALIBRATION_SAMPLES 32
#endif

// Channels whose resting ADC is below this are treated as unconnected.
#ifndef MUX_MIN_BASELINE
#    define MUX_MIN_BASELINE 100
#endif

// Debounce window in milliseconds.
#ifndef MUX_DEBOUNCE_MS
#    define MUX_DEBOUNCE_MS 5
#endif

// ─── Channel → matrix position map ──────────────────────────────────────────
#define MUX_CHANNELS 16
#define MUX_UNUSED   255  // sentinel for unconnected channels

typedef struct {
    uint8_t row;
    uint8_t col;
} mux_channel_map_t;

// Defined in mux_analog.c — edit that table to match your wiring.
extern const mux_channel_map_t mux_channel_map[MUX_CHANNELS];


