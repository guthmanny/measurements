// Copyright (c) 2026 nux. All rights reserved.
//
// Public sample-map factory. Hosts parse their own formats and load WAV;
// this header is the only map API they should include.

#ifndef SSMEL_MAP_H
#define SSMEL_MAP_H

#include "ssmel/ssmel_api.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SSMEL_LAYER_KBD_MAX 8u

typedef struct ssmel_map ssmel_map_t;
typedef struct ssmel_buffer ssmel_buffer_t;

typedef enum ssmel_loop_mode
{
  SSMEL_LOOP_OFF = 0,
  SSMEL_LOOP_FORWARD = 1,
} ssmel_loop_mode_t;

typedef enum ssmel_trigger
{
  SSMEL_TRIGGER_ATTACK = 0,
  SSMEL_TRIGGER_RELEASE = 1,
  SSMEL_TRIGGER_FIRST = 2,
  SSMEL_TRIGGER_LEGATO = 3,
  SSMEL_TRIGGER_RESONANCE = 4,
} ssmel_trigger_t;

typedef enum ssmel_layer_mode
{
  SSMEL_LAYER_SELECT_VELOCITY = 0,
  SSMEL_LAYER_PLAY_ALL = 1,
  SSMEL_LAYER_ROUND_ROBIN = 2,
  SSMEL_LAYER_VELOCITY_CROSSFADE = 3,
} ssmel_layer_mode_t;

typedef struct ssmel_layer_eg
{
  int enabled;
  double attack_ms;
  double hold_ms;
  double decay_ms;
  double break_ms;
  float break_level;
  double release_ms;
  float sustain;
  int oneshot;
} ssmel_layer_eg_t;

typedef struct ssmel_layer_kbd
{
  uint32_t num_points;
  int keys[SSMEL_LAYER_KBD_MAX];
  float values[SSMEL_LAYER_KBD_MAX];
} ssmel_layer_kbd_t;

typedef struct ssmel_layer_vel
{
  int enabled;
  int lo_x;
  int hi_x;
  float lo_y;
  float hi_y;
} ssmel_layer_vel_t;

typedef struct ssmel_layer_desc
{
  int lo_velocity;
  int hi_velocity;
  int root_note;
  uint32_t start_frame;
  uint32_t end_frame;
  uint32_t loop_start;
  uint32_t loop_end;
  ssmel_loop_mode_t loop_mode;
  int crossfade_below;
  int crossfade_above;
  ssmel_trigger_t trigger;
  int group;
  int off_by;
  int use_release_velocity;
  double min_hold_ms;
  double legato_offset_ms;
  float gain;
  ssmel_layer_eg_t amp_eg;
  ssmel_layer_eg_t filter_eg;
  ssmel_layer_kbd_t filter_kbd;
  float filter_cutoff_oct;
  ssmel_layer_vel_t filter_vel;
  float filter_env_amount;
  ssmel_layer_vel_t amp_vel;
} ssmel_layer_desc_t;

SSMEL_API uint32_t ssmel_abi_version(void);

SSMEL_API void ssmel_layer_desc_init(ssmel_layer_desc_t* desc);

/** Copy planar float PCM (1 or 2 channels). */
SSMEL_API ssmel_buffer_t* ssmel_buffer_create(const float* const* channels, uint32_t num_channels,
                                              uint32_t num_frames, float sample_rate);
SSMEL_API void ssmel_buffer_destroy(ssmel_buffer_t* buffer);

SSMEL_API ssmel_map_t* ssmel_map_create(void);
SSMEL_API void ssmel_map_destroy(ssmel_map_t* map);

SSMEL_API void ssmel_map_set_layer_mode(ssmel_map_t* map, ssmel_layer_mode_t mode);
SSMEL_API void ssmel_map_set_release_layer_mode(ssmel_map_t* map, ssmel_layer_mode_t mode);

/** @return zone index, or UINT32_MAX on failure. */
SSMEL_API uint32_t ssmel_map_add_zone(ssmel_map_t* map, int lo_key, int hi_key);

/**
 * Append a layer. On success, @p buffer is consumed (do not destroy it).
 * @return layer index within the zone, or UINT32_MAX on failure (buffer still owned by caller).
 */
SSMEL_API uint32_t ssmel_map_add_layer(ssmel_map_t* map, uint32_t zone_index, ssmel_buffer_t* buffer,
                                       const ssmel_layer_desc_t* desc);

#ifdef __cplusplus
}
#endif

#endif  // SSMEL_MAP_H
