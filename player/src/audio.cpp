/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * audio.cpp provides an interface (CAudioSync) between the codec and
 * the SDL audio APIs.
 */
#include <stdlib.h>
#include <string.h>
#include "player_session.h"
#include "audio.h"
#include "player_util.h"
#include "audio_convert.h"
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_FILL 1
//#define DEBUG_DELAY 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiosync")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiosync", fmt)
#endif



// Sync task api - initialize the sucker.
// May need to check out non-standard frequencies, see about conversion.
// returns 0 for not yet, 1 for initialized, -1 for error
int CAudioSync::initialize_audio (int have_video) 
{
  return (0);
}

// This is used by the sync thread to determine if a valid amount of
// buffers are ready, and what time they start.  It is used to determine
// when we should start.
int CAudioSync::is_audio_ready (uint64_t &disptime)
{
  return (0);
}

// Used by the sync thread to see if resync is needed.
// 0 - in sync.  > 0 - sync time we need. -1 - need to do sync 
bool CAudioSync::check_audio_sync (uint64_t current_time, 
				   uint64_t &resync_time,
				   int64_t &wait_time,
				   bool &have_eof,
				   bool &restart_sync)
{
  return (true);
}

void CAudioSync::play_audio (void)
{
}


static uint16_t swapit (uint16_t val)
{
#ifndef WORDS_BIGENDIAN
   return ntohs(val);
#else
    uint8_t *p = (uint8_t *)&val, temp;
    temp = p[0];
    p[0] = p[1];
    p[1] = temp;
    return val;
#endif
}

void CAudioSync::audio_convert_data (void *from, uint32_t samples)
{
  audio_convert_format(m_convert_buffer,
		       from,
		       samples,
		       m_decode_format,
		       m_got_channels,
		       m_channels);
}

/*
 * interp_float - interpolate between 3 floating point values using
 * linear interpolation
 */
void interp_float (void *nv, void *tv, uint32_t chans)
{
  float *next = (float *)nv;
  float *to = (float *)tv;
  float *prev_val = next - chans;
  float *next_val = next;
  
  for (uint32_t ix = 0; ix < chans; ix++) {
    to[0] = ((*prev_val * 3) + *next_val) / 4;
    to[1 * chans] = (*prev_val + *next_val) / 2;
    to[2 * chans] = (*prev_val + (*next_val * 3)) / 4;
    prev_val++;
    next_val++;
    to++;
  }
}

/*
 * ugly code alert - this is a macro that will create functions to
 * add 3 samples using linear interpolation.  nv is the pointer to
 * the next set of samples.  This create interp_ui8, interp_i8, interp_i16
 * and interp_ui16
 */
#define INTERP(sign, val) \
static void interp_##sign##val (void *nv, void *tv, uint32_t chans) \
{ \
  sign##nt##val##_t *next = (sign##nt##val##_t *)nv, \
     *to = (sign##nt##val##_t *)tv, \
     *prev_val = next - chans, \
     *next_val = next; \
  sign##nt32_t temp; \
  for (uint32_t ix = 0; ix < chans; ix++) { \
    temp = ((*prev_val * 3) + *next_val) / 4; \
    to[0] = temp; \
    temp = (*prev_val + *next_val) / 2; \
    to[1 * chans] = temp; \
    temp = (*prev_val + (*next_val * 3))/ 4; \
    to[2 * chans] = temp; \
    prev_val++; \
    next_val++; \
    to++; \
  } \
}

INTERP(ui, 8);
INTERP(i, 8);
INTERP(i, 16);
INTERP(ui, 16);

/*
 * More ugly code - we need to swap sometimes.  Same interpolation routines,
 * but we handle byte swapping.
 */

#define INTERP_SWAP(type) \
static void interp_swap_##type (void *nv, void *tv, uint32_t chans) \
{ \
  type *next = (type *)nv; \
  type *to = (type *)tv; \
  type val[2]; \
  uint32_t temp; \
  for (uint32_t ix = 0; ix < chans; ix++) { \
    val[0] = swapit(*(next - chans)); \
    val[1] = swapit(*next); \
    temp = ((val[0] * 3) + val[1]) / 4; \
    to[0] = htons(temp); \
    temp = (val[0] + val[1]) / 2; \
    to[1 * chans] = swapit(temp); \
    temp = (val[0] + (val[3] * 3))/ 4; \
    to[2 * chans] = swapit(temp); \
    next++; \
    to++; \
  } \
}

INTERP_SWAP(int16_t);
INTERP_SWAP(uint16_t);

void *CAudioSync::interpolate_3_samples (void *next_sample)
{
  if (m_sample_interpolate_buffer == NULL) {
    m_sample_interpolate_buffer = 
      malloc(3 * m_bytes_per_sample_input * m_channels);
  }
  switch (m_decode_format) {
  case AUDIO_FMT_FLOAT:
    interp_float(next_sample, m_sample_interpolate_buffer, m_channels);
    break;
  case AUDIO_FMT_U8:
    interp_ui8(next_sample, m_sample_interpolate_buffer, m_channels);
    break;
  case AUDIO_FMT_S8:
    interp_i8(next_sample, m_sample_interpolate_buffer, m_channels);
    break;
  case AUDIO_FMT_S16MSB:
    if (AUDIO_U16SYS == AUDIO_S16MSB) {
      interp_i16(next_sample, m_sample_interpolate_buffer, m_channels);
    } else {
      interp_swap_int16_t(next_sample, m_sample_interpolate_buffer, m_channels);
    };
    break;
  case AUDIO_FMT_U16MSB:
    if (AUDIO_U16SYS == AUDIO_S16MSB) {
      interp_ui16(next_sample, m_sample_interpolate_buffer, m_channels);
    } else {
      interp_swap_uint16_t(next_sample, m_sample_interpolate_buffer, m_channels);
    };
    break;
  case AUDIO_FMT_S16LSB:
    if (AUDIO_U16SYS == AUDIO_S16LSB) {
      interp_i16(next_sample, m_sample_interpolate_buffer, m_channels);
    } else {
      interp_swap_int16_t(next_sample, m_sample_interpolate_buffer, m_channels);
    };
    break;
  case AUDIO_FMT_U16LSB:
    if (AUDIO_U16SYS == AUDIO_S16LSB) {
      interp_ui16(next_sample, m_sample_interpolate_buffer, m_channels);
    } else {
      interp_swap_uint16_t(next_sample, m_sample_interpolate_buffer, m_channels);
    };
    break;
  case AUDIO_FMT_U16:
    interp_ui16(next_sample, m_sample_interpolate_buffer, m_channels);
    break;
  case AUDIO_FMT_S16:
    interp_i16(next_sample, m_sample_interpolate_buffer, m_channels);
    break;
  case AUDIO_FMT_HW_AC3:
    return NULL;
  }
  if (m_convert_buffer) {
    audio_convert_data(m_sample_interpolate_buffer, 3);
    return m_convert_buffer;
  }
  return m_sample_interpolate_buffer;
}
/* end audio.cpp */

