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
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_FILL 1
//#define DEBUG_DELAY 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiosync")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiosync", fmt)
#endif


void CAudioSync::set_eof(void) 
{ 
  audio_message(LOG_DEBUG, "Setting audio EOF");
  m_eof = 1;
}

void CAudioSync::clear_eof (void)
{
  m_eof = 0;
}

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

// Called from the sync thread when we want to stop.  Pause the audio,
// and indicate that we're not to fill any more buffers - this should let
// the decode thread get back to receive the pause message.  Only called
// when pausing - could cause m_dont_fill race conditions if called on play
void CAudioSync::flush_sync_buffers (void)
{
}

// this is called from the decode thread.  It gets called on entry into pause,
// and entry into play.  This way, m_dont_fill race conditions are resolved.
void CAudioSync::flush_decode_buffers (void)
{
}


// Note - this is from a52dec.  It seems to be a fast way to
// convert floats to int16_t.  However, simply casting, then comparing
// would probably work as well
static inline int16_t convert_float (int32_t i)
{
  if (i > 0x43c07fff)
    return INT16_MAX;
  else if (i < 0x43bf8000)
    return INT16_MIN;
  else
    return i - 0x43c00000;
}

/*
 * convert signed 8 bit to signed 16 bit.  Basically, just shift
 */
static void audio_convert_s8_to_s16 (int16_t *to,
				     uint8_t *from,
				     uint32_t samples)
{
  if (to == NULL) return;
  for (uint32_t ix = 0; ix < samples; ix++) {
    int16_t diff = (*from++ << 8);
    *to++ = diff;
  }
}


/*
 * convert unsigned 8 bit to signed 16.  Shift, then subtrack 0x7fff
 */
static void audio_convert_u8_to_s16 (int16_t *to,
				     uint8_t *from,
				     uint32_t samples)
{
  if (to == NULL) return;
  for (uint32_t ix = 0; ix < samples; ix++) {
    int16_t diff = (*from++ << 8);
    diff -= INT16_MAX;
    *to++ = diff;
  }
}

/*
 * convert unsigned 16 bit to signed 16 bit - subtract 0x7fff
 */
static void audio_convert_u16_to_s16 (int16_t *to,
				      uint16_t *from,
				      uint32_t samples)
{
  if (to == NULL) return;

  for (uint32_t ix = 0; ix < samples; ix++) {
    int32_t diff = *from++;
    diff -= INT16_MAX;
    *to++ = diff;
  }
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
/*
 * audio_convert_to_sys - convert MSB or LSB codes to the native
 * format
 */
static void audio_convert_to_sys (uint16_t *conv, 
				  uint32_t samples)
{
  for (uint32_t ix = 0; ix < samples; ix++) {
    *conv = swapit(*conv);
    conv++;
  }
}

/*
 * audio_upconvert_chans - convert from a lower amount of channels to a 
 * larger amount.  It involves copying the channels, then 0'ing out the 
 * unused channels.
 */
void audio_upconvert_chans (uint8_t *to, 
			    uint8_t *from,
			    uint32_t samples,
			    uint32_t src_chans,
			    uint32_t dst_chans)
{
  uint32_t ix;
  uint32_t src_bytes;
  uint32_t dst_bytes;
  uint32_t blank_bytes;
  uint32_t bytes_per_sample = sizeof(uint16_t);

  src_bytes = src_chans * bytes_per_sample;
  dst_bytes = dst_chans * bytes_per_sample;

  blank_bytes = dst_bytes - src_bytes;
  if (src_chans == 1) {
    blank_bytes -= src_bytes;
  }

  for (ix = 0; ix < samples; ix++) {
    memcpy(to, from, src_bytes);
    to += src_bytes;

    if (src_chans == 1) {
      memcpy(to, from, src_bytes);
      to += src_bytes;
    }
    memset(to, 0, blank_bytes);
    to += blank_bytes;

    from += src_bytes;
  }
}

/*
 * audio_downconvert_chans_remove_chans - convert from a larger 
 * amount to a smaller amount by just dropping the upper channels
 * We do this to drop the LFE, for the most part.
 */
static void audio_downconvert_chans_remove_chans (uint8_t *to, 
						  uint8_t *from,
						  uint32_t samples,
						  uint32_t src_chans,
						  uint32_t dst_chans)
{
  uint32_t bytes_per_sample = sizeof(int16_t);
  uint32_t src_bytes;
  uint32_t dst_bytes;
  src_bytes = src_chans * bytes_per_sample;
  dst_bytes = dst_chans * bytes_per_sample;

  for (uint32_t ix = 0; ix < samples; ix++) {
    memcpy(to, from, dst_bytes);
    to += dst_bytes;
    from += src_bytes;
  }
}

/*
 * convert_s16 - cap the values at INT16_MAX and INT16_MIN for sums
 */
static inline int16_t convert_s16 (int32_t val)
{
  if (val > INT16_MAX) {
    return INT16_MAX;
  }
  if (val < INT16_MIN) {
    return INT16_MIN;
  }
  return val;
}

/*
 * audio_downconvert_chans_s16 - change a higher amount to a lower
 * amount
 */
void audio_downconvert_chans_s16 (int16_t *to,
				  int16_t *from,
				  uint32_t src_chans,
				  uint32_t dst_chans,
				  uint32_t samples)
{
  uint32_t ix, jx;

  switch (dst_chans) {
  case 5:
  case 4:
    // we're doing 6 to 5 or 6 or 5 to 4 (5 to 4 should probably combine
    // the center with the left and right.
    audio_downconvert_chans_remove_chans((uint8_t *)to, 
					 (uint8_t *)from, 
					 samples,
					 src_chans, 
					 dst_chans);
    break;
  case 2:
    // downconvert to stereo
    for (ix = 0; ix < samples; ix++) {
      // we have 4, 5 or 6 chans
      int32_t l, r;
      l = from[0];
      l += from[2]; // add L, LR
      r = from[1];
      r = from[3];  // add R, RR
      if (src_chans > 4) {
	l += from[4]; // add center to both l and r
	r += from[4];
	l /= 3;
	r /= 3;
      } else {
	l /= 2; // no center
	r /= 2;
      }
      *to++ = convert_s16(l);
      *to++ = convert_s16(r);
      from += src_chans;
    }
    break;
  case 1: {
    // everything to mono - sum L, LR, R, RR and C, if they exist
    uint32_t add_chans;
    if (src_chans == 6) add_chans = 5;
    else add_chans = src_chans;
    for (ix = 0; ix < samples; ix++) {
      int32_t sum = 0;
      for (jx = 0; jx < add_chans; jx++) {
	sum += from[jx];
      }
      sum /= add_chans;
      *to++ = convert_s16(sum);
      from += src_chans;
    }
    break;
  }
  }
}

void CAudioSync::audio_convert_data (void *from, uint32_t samples)
{
  uint32_t ix;
  uint32_t src_chan_samples;

  src_chan_samples = samples * m_channels;

  // if we're here, convert everything to S16
  switch (m_decode_format) {
  case AUDIO_FMT_FLOAT: {
    // convert float to S16 
    int32_t *ffrom = (int32_t *)from;
    int16_t *to;
    if (m_got_channels == m_channels) {
      to = (int16_t *)m_convert_buffer;
    } else {
      to = (int16_t *)m_fmt_buffer;
    }
    
    for (ix = 0; ix < src_chan_samples; ix++) {
      *to++ = convert_float(ffrom[ix]);
    }
    if (m_got_channels == m_channels) {
      return;
    }
    from = m_fmt_buffer;
    break;
  }
  case AUDIO_FMT_U8:
    audio_convert_u8_to_s16(m_fmt_buffer, (uint8_t *)from, src_chan_samples);
    from = m_fmt_buffer;
    break;
  case AUDIO_FMT_S8:
    audio_convert_s8_to_s16(m_fmt_buffer, (uint8_t *)from, src_chan_samples);
    from = m_fmt_buffer;
    break;
  case AUDIO_FMT_U16MSB:
    if (AUDIO_U16SYS != AUDIO_U16MSB) {
      audio_convert_to_sys((uint16_t *)from, src_chan_samples);
    }
    audio_convert_u16_to_s16(m_fmt_buffer, (uint16_t *)from, src_chan_samples);
    from = m_fmt_buffer;
    break;
  case AUDIO_FMT_S16MSB:
    if (AUDIO_U16SYS != AUDIO_U16MSB) {
      audio_convert_to_sys((uint16_t *)from, src_chan_samples);
    }
    break;
  case AUDIO_FMT_U16LSB:
    if (AUDIO_U16SYS != AUDIO_U16LSB) {
      audio_convert_to_sys((uint16_t *)from, src_chan_samples);
    }
    audio_convert_u16_to_s16(m_fmt_buffer, (uint16_t *)from, src_chan_samples);
    from = m_fmt_buffer;
    break;
  case AUDIO_FMT_S16LSB:
    if (AUDIO_U16SYS != AUDIO_U16LSB) {
      audio_convert_to_sys((uint16_t *)from, src_chan_samples);
    }
    break;
  case AUDIO_FMT_U16:
    audio_convert_u16_to_s16(m_fmt_buffer, (uint16_t *)from, src_chan_samples);
    from = m_fmt_buffer;
    break;
  case AUDIO_FMT_S16:
    break;
  case AUDIO_FMT_HW_AC3:
    abort();
  }

  if (m_convert_buffer == NULL) {
    return;
  }
  // at this point - from points to a buffer of all S16, system based
  // ordering
  if (m_channels == m_got_channels) {
    memcpy(m_convert_buffer, from, src_chan_samples * sizeof(int16_t));
    return;
  }
  if (m_channels > m_got_channels) {
    audio_downconvert_chans_s16((int16_t *)m_convert_buffer, 
				(int16_t *)from, 
				m_channels, 
				m_got_channels,
				samples);
    return;
  }
    
  audio_upconvert_chans((uint8_t *)m_convert_buffer, 
			(uint8_t *)from, 
			samples, 
			m_channels, 
			m_got_channels);
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
      malloc(m_bytes_per_sample_input * m_channels);
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

