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
#include "audio.h"
#include "player_util.h"

/*
 * c routine to call into the AudioSync class callback
 */
static void c_audio_callback (void *userdata, Uint8 *stream, int len)
{
  CAudioSync *a = (CAudioSync *)userdata;
  a->audio_callback(stream, len);
}

/*
 * Create an CAudioSync for a session.  Don't alloc any buffers until
 * config is called by codec
 */
CAudioSync::CAudioSync (CPlayerSession *psptr, int volume)
{
  m_psptr = psptr;
  m_fill_index = m_play_index = 0;
  for (int ix = 0; ix < DECODE_BUFFERS_MAX; ix++) {
    m_buffer_filled[ix] = 0;
    m_sample_buffer[ix] = NULL;
  }
  m_buffer_size = 0;
  m_config_set = 0;
  m_audio_initialized = 0;
  m_audio_paused = 1;
  m_resync_required = 1;
  m_dont_fill = 0;
  m_consec_no_buffers = 0;
  SDL_Init(SDL_INIT_AUDIO);
  m_audio_waiting_buffer = 0;
  m_audio_waiting = NULL; // will be set by decode thread
  m_eof_found = 0;
  m_skipped_buffers = 0;
  m_didnt_fill_buffers = 0;
  m_play_time = -1LLU;
  m_buffer_latency = 0;
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}

/*
 * Close out audio sync - stop and disconnect from SDL
 */
CAudioSync::~CAudioSync (void)
{
  SDL_PauseAudio(1);
  SDL_CloseAudio();
  for (int ix = 0; ix < DECODE_BUFFERS_MAX; ix++) {
    if (m_sample_buffer[ix] != NULL)
      free(m_sample_buffer[ix]);
    m_sample_buffer[ix] = NULL;
  }
  player_debug_message("Audio sync skipped %u buffers", m_skipped_buffers);
  player_debug_message("didn't fill %u buffers", m_didnt_fill_buffers);
}

/*
 * codec api - set up information about the stream
 */
void CAudioSync::set_config (int freq, 
			     int channels, 
			     int format, 
			     size_t max_buffer_size) 
{
  if (m_config_set != 0) 
    return;
  for (int ix = 0; ix < DECODE_BUFFERS_MAX; ix++) {
    m_buffer_filled[ix] = 0;
    // I'm not sure where the 2 * comes in... Check this out
    m_sample_buffer[ix] = 
      (short *)malloc(2 * channels * max_buffer_size * sizeof(short));
  }
  m_buffer_size = max_buffer_size;
  m_freq = freq;
  m_channels = channels;
  m_format = format;
  m_config_set = 1;
  m_msec_per_frame = m_buffer_size * 1000 / m_freq;
};

/*
 * Codec api - get_audio_buffer - will wait if there are no available
 * buffers
 */
short *CAudioSync::get_audio_buffer (void)
{
  int ret;
  if (m_dont_fill == 1) 
    return (NULL);

  SDL_LockAudio();
  ret = m_buffer_filled[m_fill_index];
  SDL_UnlockAudio();
  if (ret == 1) {
    m_audio_waiting_buffer = 1;
    SDL_SemWait(m_audio_waiting);
    m_audio_waiting_buffer = 0;
    if (m_dont_fill != 0) return (NULL);
    SDL_LockAudio();
    ret = m_buffer_filled[m_fill_index];
    SDL_UnlockAudio();
    if (ret == 1) {
      return (NULL);
    }
  }
  return (m_sample_buffer[m_fill_index]);
}

/*
 * filled_audio_buffer - codec API - after codec fills the buffer from
 * get_audio_buffer, it will call here.
 */
void CAudioSync::filled_audio_buffer (uint64_t ts, int resync)
{
  size_t temp;
  // m_dont_fill will be set when we have a pause
  if (m_dont_fill == 1) {
    return;
  }
  SDL_LockAudio();

  if (m_audio_paused == 0 && m_first_time == 1 && ts < m_play_time) {
    SDL_UnlockAudio();
    m_didnt_fill_buffers++;
    player_debug_message("Eliminating past time %llu %llu", 
			 ts, m_play_time);
    return;
  }

  m_buffer_filled[m_fill_index] = 1;
    
  SDL_UnlockAudio();
  m_buffer_time[m_fill_index] = ts;
  if (resync) {
    m_resync_required = 1;
    m_resync_buffer = m_fill_index;
#ifdef DEBUG_SYNC
    player_debug_message("Resync from filled_audio_buffer");
#endif
  }
  temp = m_fill_index;
  m_fill_index++;
  m_fill_index %= DECODE_BUFFERS_MAX;
  // Check this - we might not want to do this unless we're resyncing
  SDL_SemPost(m_sync_sem);
  //player_debug_message("Filling %llu %u", ts, temp);
}

// Sync task api - initialize the sucker.
// May need to check out non-standard frequencies, see about conversion.
// returns 0 for not yet, 1 for initialized, -1 for error
int CAudioSync::initialize_audio (int have_video) 
{
  if (m_audio_initialized == 0) {
    if (m_config_set) {
      SDL_AudioSpec *wanted;
      m_do_sync = have_video;
      wanted = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
      if (wanted == NULL) {
	return (-1);
      }
      wanted->freq = m_freq;
      wanted->channels = m_channels;
      wanted->format = m_format;
      //      int sample_size;
      //if (m_format == AUDIO_U8 || m_format == AUDIO_S8) sample_size = 1;
      //else sample_size = 2;
      wanted->samples = m_buffer_size;
      wanted->callback = c_audio_callback;
      wanted->userdata = this;
      int ret = SDL_OpenAudio(wanted, &m_obtained);
      if (ret < 0) {
	player_error_message("Couldn't open audio, %s", SDL_GetError());
	return (-1);
      }
#if 0
       player_debug_message("got f %d chan %d format %d samples %d size %u",
                             m_obtained.freq,
                             m_obtained.channels,
                             m_obtained.format,
                             m_obtained.samples,
                             m_obtained.size);
#endif

      m_audio_initialized = 1;
    } else {
      return 0; // check again pretty soon...
    }
  }
  return (1);
}

// This is used by the sync thread to determine if a valid amount of
// buffers are ready, and what time they start.  It is used to determine
// when we should start.
int CAudioSync::is_audio_ready (uint64_t &disptime)
{
  disptime = m_buffer_time[m_play_index];
  return (m_dont_fill == 0 && m_buffer_filled[m_play_index] == 1);
}

// Used by the sync thread to see if resync is needed.
// 0 - in sync.  > 0 - sync time we need. -1 - need to do sync 
uint64_t CAudioSync::check_audio_sync (uint64_t current_time, int &have_eof)
{
  if (m_eof_found != 0) {
    have_eof = 1;
    return (0);
  }
  // audio is initialized.
  if (m_resync_required) {
    if (m_audio_paused && m_buffer_filled[m_resync_buffer]) {
      // Calculate the current time based on the latency
      current_time += m_buffer_latency;
      uint64_t cmptime;
      // Compare with times in buffer - we may need to skip if we fell
      // behind.
      do {
	cmptime = m_buffer_time[m_resync_buffer];

	if (cmptime < current_time) {
#ifdef DEBUG_SYNC
	  player_debug_message("Passed time %llu %llu %u", 
			       cmptime, current_time, m_resync_buffer);
#endif
	  SDL_LockAudio();
	  m_buffer_filled[m_resync_buffer] = 0;
	  m_resync_buffer++;
	  m_resync_buffer %= DECODE_BUFFERS_MAX;
	  SDL_UnlockAudio();
	  if (m_audio_waiting_buffer) {
	    m_audio_waiting_buffer = 0;
	    SDL_SemPost(m_audio_waiting);
	  }
	}
      } while (m_buffer_filled[m_resync_buffer] == 1 && 
	       cmptime < current_time);

      if (m_buffer_filled[m_resync_buffer] == 0) {
	return (0);
      }

      if (cmptime == current_time) {
	m_play_index = m_resync_buffer;
	play_audio();
#ifdef DEBUG_SYNC
	player_debug_message("Resynced audio at %llu %u %u", current_time, m_resync_buffer, m_play_index);
#endif
	return (0);
      } else {
	return (cmptime);
      }
    } else {
      return (0);
    }
  }
  return (0);
}
// Audio callback from SDL.
//    static int fc = 0;
void CAudioSync::audio_callback (Uint8 *stream, int len)
{
  
  if (m_resync_required) {
    // resync required from codec side.  Shut down, and notify sync task
    if (m_resync_buffer == m_play_index) {
      SDL_PauseAudio(1);
      m_audio_paused = 1;
      SDL_SemPost(m_sync_sem);
#ifdef DEBUG_SYNC
      player_debug_message("sempost");
#endif
      return;
    }
  }

  m_play_time = m_psptr->get_current_time();

  if (m_first_time == 0) {
    /*
     * If we're not the first time, see if we're about a frame or more
     * around the current time, with latency built in.  If not, skip
     * the buffer.  This prevents us from playing past-due buffers.
     */
    while ((m_buffer_filled[m_play_index] != 0) &&
	   (m_buffer_time[m_play_index] + m_msec_per_frame < 
	    m_play_time + m_buffer_latency)) {
      m_buffer_filled[m_play_index] = 0;
      player_debug_message("Skipped audio buffer %llu at %llu", 
			   m_buffer_time[m_play_index] + m_msec_per_frame,
			   m_play_time + m_buffer_latency);
      m_play_index++;
      m_play_index %= DECODE_BUFFERS_MAX;
      m_skipped_buffers++;
    }
  }

  // Do we have a buffer ?  If no, see if we need to stop.
  if (m_buffer_filled[m_play_index] == 0) {
    if (m_eof_found) {
      SDL_PauseAudio(1);
      m_audio_paused = 1;
      SDL_SemPost(m_sync_sem);
      return;
    }
#ifdef DEBUG_SYNC
    player_debug_message("No buffer in audio callback %u %u %u", 
			 m_play_index, ix % DECODE_BUFFERS_MAX,
			 bufsneeded);
#endif
    m_consec_no_buffers++;
    if (m_consec_no_buffers > 10) {
      SDL_PauseAudio(1);
      m_audio_paused = 1;
      m_resync_required = 1;
      m_resync_buffer = m_play_index;
      SDL_SemPost(m_sync_sem);
    }
    if (m_audio_waiting_buffer) {
      m_audio_waiting_buffer = 0;
      SDL_SemPost(m_audio_waiting);
    }

    return;
  }

  // We have a valid buffer.  Push it to SDL.
  m_consec_no_buffers = 0;
#if 1
  SDL_MixAudio(stream, 
	       (Uint8 *)m_sample_buffer[m_play_index], 
	       len, 
	       m_volume);
#else
  memcpy(stream, 
	 m_sample_buffer[m_play_index], 
	 m_buffer_size * sizeof(short) * m_channels );


#endif
  // Increment past this buffer.
  uint64_t time = m_buffer_time[m_play_index];
  m_buffer_filled[m_play_index] = 0;
  //player_debug_message("Playing %u", m_play_index);
  m_play_index++;
  m_play_index %= DECODE_BUFFERS_MAX;

  if (m_first_time != 0) {
    // First time through - tell the sync task we've started, so it can
    // keep sync time.
    m_first_time = 0;
    m_psptr->audio_is_ready(0, time);  // 3 buffers is 69
    m_consec_wrong_latency = 0;
    m_wrong_latency_total = 0;
  } else if (m_do_sync) {
    // Okay - now we check for latency changes.
#define ALLOWED_LATENCY 2
    // we have a latency number - see if it really is correct
    uint64_t index_time = m_buffer_latency + m_play_time;
    if (time > index_time + ALLOWED_LATENCY || 
	time < index_time - ALLOWED_LATENCY) {
      m_consec_wrong_latency++;
      m_wrong_latency_total += time - index_time;
      int64_t test;
      test = m_wrong_latency_total / m_consec_wrong_latency;
      if (test > ALLOWED_LATENCY || test < -ALLOWED_LATENCY) {
	if (m_consec_wrong_latency > 20) {
	  m_consec_wrong_latency = 0;
	  m_buffer_latency += test; 
	  m_psptr->audio_is_ready(m_buffer_latency, time);
	  player_debug_message("Latency off by %lld - now is %llu", 
			       test, m_buffer_latency);
	}
      } else {
	// average wrong latency is not greater 5 or less -5
	m_consec_wrong_latency = 0;
	m_wrong_latency_total = 0;
      }
    } else {
      m_consec_wrong_latency = 0;
      m_wrong_latency_total = 0;
    }
  }

  // If we had the decoder task waiting, signal it.
  if (m_audio_waiting_buffer) {
    m_audio_waiting_buffer = 0;
    SDL_SemPost(m_audio_waiting);
  }
}

void CAudioSync::play_audio (void)
{
  m_first_time = 1;
  m_resync_required = 0;
  m_audio_paused = 0;
  SDL_PauseAudio(0);
}

// Called from the sync thread when we want to stop.  Pause the audio,
// and indicate that we're not to fill any more buffers - this should let
// the decode thread get back to receive the pause message.  Only called
// when pausing - could cause m_dont_fill race conditions if called on play
void CAudioSync::flush_sync_buffers (void)
{
  // we don't need to signal the decode task right now - 
  // Go ahead 
  m_eof_found = 0;
  SDL_PauseAudio(1);
  m_dont_fill = 1;
  if (m_audio_waiting_buffer) {
    m_audio_waiting_buffer = 0;
    SDL_SemPost(m_audio_waiting);
  }
  //player_debug_message("Flushed sync");
}

// this is called from the decode thread.  It gets called on entry into pause,
// and entry into play.  This way, m_dont_fill race conditions are resolved.
void CAudioSync::flush_decode_buffers (void)
{
  m_dont_fill = 0;
  SDL_LockAudio();
  for (int ix = 0; ix < DECODE_BUFFERS_MAX; ix++) {
    m_buffer_filled[ix] = 0;
  }
  m_play_index = m_fill_index = 0;
  m_audio_paused = 1;
  m_resync_buffer = 0;
  SDL_UnlockAudio();
  //player_debug_message("flushed decode");
}

void CAudioSync::set_volume (int volume)
{
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}

/* end audio.cpp */
