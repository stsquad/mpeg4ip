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
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiosync")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiosync", fmt)
#endif

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
  //SDL_Init(SDL_INIT_AUDIO);
  m_audio_waiting_buffer = 0;
  m_audio_waiting = NULL; // will be set by decode thread
  m_eof_found = 0;
  m_skipped_buffers = 0;
  m_didnt_fill_buffers = 0;
  m_play_time = 0         ;
  m_buffer_latency = 0;
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
  m_first_time = 1;
  m_first_filled = 1;
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
  audio_message(LOG_NOTICE, 
		"Audio sync skipped %u buffers", 
		m_skipped_buffers);
  audio_message(LOG_NOTICE, "didn't fill %u buffers", m_didnt_fill_buffers);
}

/*
 * codec api - set up information about the stream
 */
void CAudioSync::set_config (int freq, 
			     int channels, 
			     int format, 
			     uint32_t sample_size) 
{
  if (m_config_set != 0) 
    return;
  
  if (format == AUDIO_U8 || format == AUDIO_S8)
    m_bytes_per_sample = 1;
  else
    m_bytes_per_sample = 2;

  m_buffer_size = channels * sample_size * m_bytes_per_sample;
  for (int ix = 0; ix < DECODE_BUFFERS_MAX; ix++) {
    m_buffer_filled[ix] = 0;
    // I'm not sure where the 2 * comes in... Check this out
    m_sample_buffer[ix] = 
      (unsigned char *)malloc(2 * m_buffer_size);
  }
  m_freq = freq;
  m_channels = channels;
  m_format = format;
  m_config_set = 1;
  m_msec_per_frame = sample_size * 1000 / m_freq;
};

/*
 * Codec api - get_audio_buffer - will wait if there are no available
 * buffers
 */
unsigned char *CAudioSync::get_audio_buffer (void)
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
  uint32_t fill_index, temp2;
  // m_dont_fill will be set when we have a pause
  if (m_dont_fill == 1) {
    return;
  }
  fill_index = m_fill_index;
  m_fill_index++;
  m_fill_index %= DECODE_BUFFERS_MAX;

  SDL_LockAudio();
  if (m_first_filled != 0) {
    m_first_filled = 0;
  } else {
    uint64_t diff;
    diff = ts - m_last_fill_timestamp;
    if (diff - m_msec_per_frame > m_msec_per_frame) {
      // have a hole here - don't want to resync
#ifdef DEBUG_AUDIO_FILL
      audio_message(LOG_DEBUG, 
		    "Filling - last %llu new %llu", m_last_fill_timestamp, ts);
#endif
      if (diff > ((m_msec_per_frame + 1) * 4)) {
	resync = 1;
      } else {
	// try to fill the holes
	m_last_fill_timestamp += m_msec_per_frame + 1; // fill plus extra
	SDL_UnlockAudio();
	int64_t ts_diff;
	do {
	  unsigned char *retbuffer;
	  // Get and swap buffers.
	  retbuffer = get_audio_buffer();
	  if (retbuffer == NULL) {
	    return;
	  }
	  if (retbuffer != m_sample_buffer[m_fill_index]) {
	    audio_message(LOG_ERR, "retbuffer not fill index in audio sync");
	    return;
	  }
	  SDL_LockAudio();
	  m_sample_buffer[m_fill_index] = m_sample_buffer[fill_index];
	  m_sample_buffer[fill_index] = retbuffer;
	  memset(retbuffer, m_obtained.silence, m_buffer_size);
	  m_buffer_time[fill_index] = m_last_fill_timestamp;
	  m_buffer_filled[fill_index] = 1;
	  m_samples_loaded += m_buffer_size;
	  fill_index++;
	  fill_index %= DECODE_BUFFERS_MAX;
	  m_fill_index++;
	  m_fill_index %= DECODE_BUFFERS_MAX;
	  SDL_UnlockAudio();
	  audio_message(LOG_NOTICE, "Filling timestamp %llu with silence",
			m_last_fill_timestamp);
	  m_last_fill_timestamp += m_msec_per_frame + 1; // fill plus extra
	  ts_diff = ts - m_last_fill_timestamp;
	  audio_message(LOG_DEBUG, "diff is %lld", ts_diff);
	} while (ts_diff > 0);
	SDL_LockAudio();
      }
    } else {
      if (m_last_fill_timestamp == ts) {
	audio_message(LOG_NOTICE, "Repeat timestamp with audio %llu", ts);
	SDL_UnlockAudio();
	return;
      }
    }
  }
  m_last_fill_timestamp = ts;
  m_buffer_filled[fill_index] = 1;
  temp2 = m_samples_loaded += m_buffer_size;
  m_buffer_time[fill_index] = ts;
  if (resync) {
    m_resync_required = 1;
    m_resync_buffer = fill_index;
#ifdef DEBUG_SYNC
    audio_message(LOG_DEBUG, "Resync from filled_audio_buffer");
#endif
  }
  SDL_UnlockAudio();

  // Check this - we might not want to do this unless we're resyncing
  if (resync)
    m_psptr->wake_sync_thread();
#ifdef DEBUG_AUDIO_FILL
  audio_message(LOG_DEBUG, "Filling " LLU " %u %u", 
		ts, fill_index, m_samples_loaded);
#endif
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
      int sample_size;
      sample_size = m_buffer_size / (m_channels * m_bytes_per_sample);
#ifndef _WINDOWS
      uint32_t ix;
      for (ix = 2; ix <= 0x8000; ix <<= 1) {
	if ((sample_size & ~(ix - 1)) == 0) {
	  break;
	}
      }
      ix >>= 1;
      audio_message(LOG_DEBUG, "Sample size is %d", ix);
      m_sample_size = ix;
#else
      m_sample_size = 4096;
#endif
      if ((m_do_sync == 0) && m_sample_size < 4096)
	m_sample_size = 4096;
      wanted->samples = 4096;
      wanted->callback = c_audio_callback;
      wanted->userdata = this;
#if DEBUG_SYNC
       audio_message(LOG_INFO, 
		     "requested f %d chan %d format %x samples %d size %u",
		     wanted->freq,
		     wanted->channels,
		     wanted->format,
		     wanted->samples,
		     wanted->size);
#endif
      int ret = SDL_OpenAudio(wanted, &m_obtained);
      if (ret < 0) {
	audio_message(LOG_CRIT, "Couldn't open audio, %s", SDL_GetError());
	return (-1);
      }
#if 1
       audio_message(LOG_INFO, "got f %d chan %d format %x samples %d size %u",
		     m_obtained.freq,
		     m_obtained.channels,
		     m_obtained.format,
		     m_obtained.samples,
		     m_obtained.size);
#endif

      m_audio_initialized = 1;
      m_use_SDL_delay = SDL_HasAudioDelayMsec();
      if (m_use_SDL_delay)
	audio_message(LOG_NOTICE, "Using delay measurement from SDL");
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
      SDL_LockAudio();
      if (m_use_SDL_delay) {
	current_time +=SDL_AudioDelayMsec();
      } else {
	current_time += m_buffer_latency;
      }
      uint64_t cmptime;
      // Compare with times in buffer - we may need to skip if we fell
      // behind.
      do {
	cmptime = m_buffer_time[m_resync_buffer];

	if (cmptime < current_time) {
#ifdef DEBUG_SYNC
	  audio_message(LOG_INFO, "Passed time " LLU " " LLU " %u", 
			       cmptime, current_time, m_resync_buffer);
#endif
	  m_buffer_filled[m_resync_buffer] = 0;
	  m_resync_buffer++;
	  m_resync_buffer %= DECODE_BUFFERS_MAX;
	}
      } while (m_buffer_filled[m_resync_buffer] == 1 && 
	       cmptime < current_time);

      if (m_audio_waiting_buffer) {
	m_audio_waiting_buffer = 0;
	SDL_SemPost(m_audio_waiting);
      }
      SDL_UnlockAudio();
      if (m_buffer_filled[m_resync_buffer] == 0) {
	return (0);
      }

      if (cmptime == current_time) {
	m_play_index = m_resync_buffer;
	play_audio();
#ifdef DEBUG_SYNC
	audio_message(LOG_INFO, "Resynced audio at " LLU " %u %u", current_time, m_resync_buffer, m_play_index);
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
void CAudioSync::audio_callback (Uint8 *stream, int ilen)
{
  int freed_buffer = 0;
  uint32_t len = (uint32_t)ilen;
  uint64_t this_time;
  int delay = 0;

  if (m_resync_required) {
    // resync required from codec side.  Shut down, and notify sync task
    if (m_resync_buffer == m_play_index) {
      SDL_PauseAudio(1);
      m_audio_paused = 1;
      m_psptr->wake_sync_thread();
#ifdef DEBUG_SYNC
      audio_message(LOG_DEBUG, "sempost");
#endif
      return;
    }
  }

  m_play_time = m_psptr->get_current_time();
  if (m_use_SDL_delay != 0) {
    delay = SDL_AudioDelayMsec();
    if (delay < 0) delay = 0;
#ifdef DEBUG_SYNC
    audio_message(LOG_DEBUG, "Audio delay is %d", delay);
#endif
  }

  if ((m_first_time == 0) &&
      (m_use_SDL_delay == 0)) {
    /*
     * If we're not the first time, see if we're about a frame or more
     * around the current time, with latency built in.  If not, skip
     * the buffer.  This prevents us from playing past-due buffers.
     */
    int time_okay = 0;
    this_time = 0;
    while ((m_buffer_filled[m_play_index] == 1) &&
	   (time_okay == 0)) {
      uint64_t buffertime, playtime;
      buffertime = m_buffer_time[m_play_index];
      if (m_play_sample_index != 0) {
	uint64_t temp;
	temp = (uint64_t) m_play_sample_index * (uint64_t)m_msec_per_frame;
	temp /= (uint64_t) m_buffer_size;
	buffertime += temp;
      }

      if (m_use_SDL_delay != 0) 
	playtime = m_play_time + delay; // m_buffer_latency;
      else 
	playtime = m_play_time + m_buffer_latency;

      if (m_play_time != 0 && buffertime + m_msec_per_frame < playtime) {
	audio_message(LOG_DEBUG, 
		      "Skipped audio buffer " LLU "("LLU") at " LLU, 
		      m_buffer_time[m_play_index],
		      buffertime,
		      playtime);
	m_buffer_filled[m_play_index] = 0;
	m_play_index++;
	m_play_index %= DECODE_BUFFERS_MAX;
	m_skipped_buffers++;
	m_buffer_latency = 0; // recalculate...
	m_first_time = 0;
	m_play_sample_index = 0;
	uint32_t diff;
	diff = m_buffer_size - m_play_sample_index;
	if (m_samples_loaded >= diff) {
	  m_samples_loaded -= diff;
	} else {	
	  m_samples_loaded = 0;
	}
	m_consec_wrong_latency = 0;  // reset all latency calcs..
	m_wrong_latency_total = 0;
      } else {
	time_okay = 1;
	this_time = buffertime;
      }
    }
  } else {
    this_time = m_buffer_time[m_play_index];
    if (m_first_time == 0) {
      if (m_play_sample_index != 0) {
	uint64_t temp;
	temp = (uint64_t) m_play_sample_index * (uint64_t)m_msec_per_frame;
	temp /= (uint64_t) m_buffer_size;
	this_time += temp;
      }
    }
  }


  // Do we have a buffer ?  If no, see if we need to stop.
  if (m_samples_loaded == 0) {
    if (m_eof_found) {
      SDL_PauseAudio(1);
      m_audio_paused = 1;
      m_psptr->wake_sync_thread();
      return;
    }
#ifdef DEBUG_SYNC
    audio_message(LOG_DEBUG, "No buffer in audio callback %u %u", 
		  m_samples_loaded, len);
#endif
    m_consec_no_buffers++;
    if (m_consec_no_buffers > 10) {
      SDL_PauseAudio(1);
      m_audio_paused = 1;
      m_resync_required = 1;
      m_resync_buffer = m_play_index;
      m_psptr->wake_sync_thread();
    }
    if (m_audio_waiting_buffer) {
      m_audio_waiting_buffer = 0;
      SDL_SemPost(m_audio_waiting);
    }

    return;
  }

  // We have a valid buffer.  Push it to SDL.
  m_consec_no_buffers = 0;

  while (len > 0) {
    uint32_t thislen;
    thislen = m_buffer_size - m_play_sample_index;
    if (len < thislen) thislen = len;
    SDL_MixAudio(stream, 
		 &m_sample_buffer[m_play_index][m_play_sample_index],
		 thislen,
		 m_volume);
    len -= thislen;
    stream += thislen;
    if (thislen <= m_samples_loaded)
      m_samples_loaded -= thislen;
    else 
      m_samples_loaded = 0;
    m_play_sample_index += thislen;
    if (m_play_sample_index >= m_buffer_size) {
      m_buffer_filled[m_play_index] = 0;
      m_play_index++;
      m_play_index %= DECODE_BUFFERS_MAX;
      m_play_sample_index = 0;
      freed_buffer = 1;
    }
  }
      
  // Increment past this buffer.
  if (m_first_time != 0) {
    // First time through - tell the sync task we've started, so it can
    // keep sync time.
    m_first_time = 0;
    if (m_use_SDL_delay != 0) 
      m_buffer_latency = delay;
    else
      m_buffer_latency = 0;
    m_psptr->audio_is_ready(m_buffer_latency, this_time);
    m_consec_wrong_latency = 0;
    m_wrong_latency_total = 0;
  } 
  else if (m_do_sync) {
#define ALLOWED_LATENCY 2
    if (m_use_SDL_delay != 0) {
      // Here, we're using the delay value from the audio buffers,
      // rather than the calculated time...
      // Okay - now we check for latency changes.
      uint64_t index_time = delay + m_play_time;

      if (this_time > index_time + ALLOWED_LATENCY || 
	  this_time < index_time - ALLOWED_LATENCY) {
#if DEBUG_SYNC
	audio_message(LOG_DEBUG, 
		      "potential change - index time "LLU" time "LLU, 
		      index_time, this_time);
#endif
	m_consec_wrong_latency++;
	m_wrong_latency_total += this_time - index_time;
	int64_t test;
	test = m_wrong_latency_total / m_consec_wrong_latency;
	if (test > ALLOWED_LATENCY || test < -ALLOWED_LATENCY) {
	  if (m_consec_wrong_latency > 3) {
	    m_consec_wrong_latency = 0;
	    m_wrong_latency_total = 0;
	    m_psptr->adjust_start_time(test);
	  }
	} else {
	  // average wrong latency is not greater than allowed latency
	  m_consec_wrong_latency = 0;
	  m_wrong_latency_total = 0;
	}
      } else {
	m_consec_wrong_latency = 0;
	m_wrong_latency_total = 0;
      }
    } else {
      // We're using the calculate latency values - they're not very
      // accurate, but better than nothing...
      // we have a latency number - see if it really is correct
      uint64_t index_time = delay + m_play_time;
#if DEBUG_SYNC
      audio_message(LOG_DEBUG, 
		    "latency - time " LLU " index " LLU " latency " LLU " %u", 
		    this_time, index_time, m_buffer_latency, m_samples_loaded);
#endif
      if (this_time > index_time + ALLOWED_LATENCY || 
	  this_time < index_time - ALLOWED_LATENCY) {
	m_consec_wrong_latency++;
	m_wrong_latency_total += this_time - index_time;
	int64_t test;
	test = m_wrong_latency_total / m_consec_wrong_latency;
	if (test > ALLOWED_LATENCY || test < -ALLOWED_LATENCY) {
	  if (m_consec_wrong_latency > 20) {
	    m_consec_wrong_latency = 0;
	    if (test < 0 && test + m_buffer_latency > 0) {
	      m_buffer_latency = 0;
	    } else {
	      m_buffer_latency += test; 
	    }
	    m_psptr->audio_is_ready(m_buffer_latency, this_time);
	    audio_message(LOG_INFO, "Latency off by " LLD " - now is " LLU, 
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
  } else {
#ifdef DEBUG_SYNC
    audio_message(LOG_DEBUG, "playing %llu %llu latency %llu", 
		  this_time, m_play_time, m_buffer_latency);
#endif
  }

  // If we had the decoder task waiting, signal it.
  if (freed_buffer != 0 && m_audio_waiting_buffer) {
    m_audio_waiting_buffer = 0;
    SDL_SemPost(m_audio_waiting);
  }
}

void CAudioSync::play_audio (void)
{
  m_first_time = 1;
  m_resync_required = 0;
  m_audio_paused = 0;
  m_play_sample_index = 0;
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
  m_first_filled = 1;
  SDL_LockAudio();
  for (int ix = 0; ix < DECODE_BUFFERS_MAX; ix++) {
    m_buffer_filled[ix] = 0;
  }
  m_play_index = m_fill_index = 0;
  m_audio_paused = 1;
  m_resync_buffer = 0;
  m_samples_loaded = 0;
  SDL_UnlockAudio();
  //player_debug_message("flushed decode");
}

void CAudioSync::set_volume (int volume)
{
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}

/* end audio.cpp */
