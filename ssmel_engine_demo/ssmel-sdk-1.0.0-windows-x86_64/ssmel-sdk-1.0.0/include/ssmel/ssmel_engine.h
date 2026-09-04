// Copyright (c) 2026 nux. All rights reserved.
//
// SSMEL engine — stable C ABI for host apps.
// Host owns format parsing and WAV decode; maps are built with ssmel_map.h.

#ifndef SSMEL_ENGINE_H
#define SSMEL_ENGINE_H

#include "ssmel/ssmel_api.h"
#include "ssmel/ssmel_map.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SSMEL_ENGINE_MAX_BLOCK 2048u
#define SSMEL_ENGINE_MAX_VOICES 32u
/** Fixed per-voice linear gain (−12 dB). Host output gain multiplies on top. */
#define SSMEL_ENGINE_VOICE_HEADROOM 0.25f

typedef struct ssmel_engine ssmel_engine_t;

typedef enum ssmel_result
{
  SSMEL_OK = 0,
  SSMEL_ERR_NULL = -1,
  SSMEL_ERR_STATE = -2,
} ssmel_result_t;

typedef enum ssmel_instrument_preset
{
  SSMEL_INSTRUMENT_UPRIGHT = 0,
  SSMEL_INSTRUMENT_EP = 1,
} ssmel_instrument_preset_t;

typedef enum ssmel_channel_layout
{
  SSMEL_CHANNEL_MONO = 0,
  SSMEL_CHANNEL_STEREO = 1,
} ssmel_channel_layout_t;

typedef enum ssmel_legato_mode
{
  SSMEL_LEGATO_OFF = 0,
  SSMEL_LEGATO_SYNTH = 1,
  SSMEL_LEGATO_ACOUSTIC = 2,
} ssmel_legato_mode_t;

/** Mod matrix indices (matches nx_mod_src_t / nx_mod_dst_t). */
typedef enum ssmel_mod_src
{
  SSMEL_MOD_LFO1 = 0,
  SSMEL_MOD_LFO2 = 1,
  SSMEL_MOD_EG1 = 2,
  SSMEL_MOD_EG2 = 3,
  SSMEL_MOD_EG3 = 4,
  SSMEL_MOD_VEL = 5,
  SSMEL_MOD_NOTE = 6,
} ssmel_mod_src_t;

typedef enum ssmel_mod_dst
{
  SSMEL_MOD_DST_PITCH = 0,
  SSMEL_MOD_DST_FILTER = 1,
  SSMEL_MOD_DST_AMP = 2,
} ssmel_mod_dst_t;

typedef enum ssmel_filter_mode
{
  SSMEL_FILTER_LP = 0,
  SSMEL_FILTER_BP = 1,
  SSMEL_FILTER_HP = 2,
} ssmel_filter_mode_t;

SSMEL_ENGINE_API ssmel_engine_t* ssmel_engine_create(void);
SSMEL_ENGINE_API void ssmel_engine_destroy(ssmel_engine_t* engine);

SSMEL_ENGINE_API ssmel_result_t ssmel_engine_prepare(
    ssmel_engine_t* engine, double sample_rate);

/**
 * Bind a client-built sample map. Engine does not take ownership of @p sample_map.
 * @p sample_map must be created with the same SDK ABI (see ssmel_abi_version).
 */
SSMEL_ENGINE_API ssmel_result_t ssmel_engine_set_sample_map(
    ssmel_engine_t* engine, ssmel_map_t* sample_map,
    ssmel_instrument_preset_t preset, ssmel_channel_layout_t layout);

SSMEL_ENGINE_API void ssmel_engine_note_on(ssmel_engine_t* engine, int note,
                                                         float velocity, int voice_id);
SSMEL_ENGINE_API void ssmel_engine_note_off(ssmel_engine_t* engine,
                                                            int note, float velocity,
                                                            int voice_id);
SSMEL_ENGINE_API void ssmel_engine_control_change(ssmel_engine_t* engine,
                                                                  int controller, int value);
SSMEL_ENGINE_API void ssmel_engine_all_notes_off(ssmel_engine_t* engine);

SSMEL_ENGINE_API void ssmel_engine_set_output_gain(ssmel_engine_t* engine,
                                                                   float gain);
SSMEL_ENGINE_API void ssmel_engine_set_pan(ssmel_engine_t* engine,
                                                           float pan_norm);

SSMEL_ENGINE_API void ssmel_engine_render_mono(ssmel_engine_t* engine,
                                                               float* out, size_t frames);
SSMEL_ENGINE_API void ssmel_engine_render_stereo(ssmel_engine_t* engine,
                                                                 float* out_l, float* out_r,
                                                                 size_t frames);

SSMEL_ENGINE_API int ssmel_engine_is_active(const ssmel_engine_t* engine);
SSMEL_ENGINE_API const char* ssmel_engine_version_string(void);

/* Phase 3 — Dream Synth tab (filter / EG / LFO / mod matrix) */

SSMEL_ENGINE_API void ssmel_engine_set_filter_cutoff_hz(
    ssmel_engine_t* engine, double cutoff_hz);
SSMEL_ENGINE_API double ssmel_engine_get_filter_cutoff_hz(
    const ssmel_engine_t* engine);
SSMEL_ENGINE_API void ssmel_engine_set_filter_Q(ssmel_engine_t* engine,
                                                                double Q);
SSMEL_ENGINE_API double ssmel_engine_get_filter_Q(
    const ssmel_engine_t* engine);
SSMEL_ENGINE_API void ssmel_engine_set_filter_mode(
    ssmel_engine_t* engine, ssmel_filter_mode_t mode);
SSMEL_ENGINE_API ssmel_filter_mode_t ssmel_engine_get_filter_mode(
    const ssmel_engine_t* engine);

SSMEL_ENGINE_API void ssmel_engine_set_mod_amount(ssmel_engine_t* engine,
                                                                 ssmel_mod_src_t src,
                                                                 ssmel_mod_dst_t dst,
                                                                 float amount);

SSMEL_ENGINE_API void ssmel_engine_set_eg_attack_ms(ssmel_engine_t* engine,
                                                                    int eg_index, double ms);
SSMEL_ENGINE_API void ssmel_engine_set_eg_decay_ms(ssmel_engine_t* engine,
                                                                   int eg_index, double ms);
SSMEL_ENGINE_API void ssmel_engine_set_eg_sustain_level(
    ssmel_engine_t* engine, int eg_index, float level);
SSMEL_ENGINE_API void ssmel_engine_set_eg_release_ms(ssmel_engine_t* engine,
                                                                     int eg_index, double ms);

SSMEL_ENGINE_API void ssmel_engine_set_lfo_rate_hz(ssmel_engine_t* engine,
                                                                   int lfo_index, float hz);
SSMEL_ENGINE_API void ssmel_engine_set_lfo_depth(ssmel_engine_t* engine,
                                                                   int lfo_index, float depth);

/* Phase 4 — Dream Global / layer playback */

SSMEL_ENGINE_API void ssmel_engine_set_tuning_cents(ssmel_engine_t* engine,
                                                                    int midi_key, float cents);
SSMEL_ENGINE_API void ssmel_engine_set_keyboard_mod_cents(
    ssmel_engine_t* engine, int midi_key, float cents);
SSMEL_ENGINE_API void ssmel_engine_set_keyboard_mod_enabled(
    ssmel_engine_t* engine, int enabled);
SSMEL_ENGINE_API void ssmel_engine_set_legato_mode(
    ssmel_engine_t* engine, ssmel_legato_mode_t mode);

SSMEL_ENGINE_API void ssmel_engine_set_resonance_enabled(
    ssmel_engine_t* engine, int enabled);
SSMEL_ENGINE_API void ssmel_engine_set_harmonic_resonance_enabled(
    ssmel_engine_t* engine, int enabled);
SSMEL_ENGINE_API void ssmel_engine_set_attack_release_fade_ms(
    ssmel_engine_t* engine, double fade_ms);

#ifdef __cplusplus
}
#endif

#endif  // SSMEL_ENGINE_H
