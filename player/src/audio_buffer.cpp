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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * audio_buffer.cpp contains code to handle an audio ring buffer with a
 * generic set of rendering code.
 */
#include "audio_buffer.h"
#include "player_session.h"
#include "player_util.h"
#include "our_config_file.h"

//#define DEBUG_AUDIO_FILL 1
//#define DEBUG_AUDIO_CALLBACK 1
//#define DEBUG_AUDIO_TIMING 1
//#define DEBUG_AUDIO_TIMING_CHANGES 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(audio_message, "audiobuf")
#else
#define audio_message(loglevel, fmt...) message(loglevel, "audiobuf", fmt)
#endif

CBufferAudioSync::CBufferAudioSync (CPlayerSession *psptr, int volume) :
  CAudioSync(psptr)
{
  m_sample_buffer = NULL;
  m_local_buffer = NULL;
  m_fill_offset = 0;
  m_play_offset = 0;

  m_first_filled = false;
  m_dont_fill = false;
  m_audio_waiting_buffer = false;
  m_audio_waiting = SDL_CreateSemaphore(0);

  m_have_resync = false;
  m_waiting_on_resync = false;
  m_resync_offset = 0;
  m_resync_ts = 0;
  m_resync_freq_ts = 0;
  m_audio_configured = false;
  m_mutex = SDL_CreateMutex();
 
  m_hardware_initialized = false;
  m_audio_paused = true;
  m_first_callback = true;
  m_restart_request = true;

  m_sync_samples_added = 0;
  m_sync_samples_removed = 0;
  m_total_samples_played = 0;
  m_have_jitter = false;
  m_jitter_msec = 0;
  m_jitter_msec_total = 0;
  m_last_add_samples = false;
}

CBufferAudioSync::~CBufferAudioSync (void)
{
  CHECK_AND_FREE(m_sample_buffer);
  CHECK_AND_FREE(m_local_buffer);
  SDL_DestroySemaphore(m_audio_waiting);
  SDL_DestroyMutex(m_mutex);

  // display stats
  audio_message(LOG_INFO, "total samples played: "U64, m_total_samples_played);
  audio_message(LOG_INFO, "sync samples added  : "U64, m_sync_samples_added);
  audio_message(LOG_INFO, "sync samples removed: "U64, m_sync_samples_removed);

  if (m_total_samples_played > 0) {
    uint64_t calc = m_total_samples_played + 
      m_sync_samples_added - m_sync_samples_removed;
    calc *= m_freq;
    calc /= m_total_samples_played;
    audio_message(LOG_INFO, "calculated output frequency : "U64, calc);
  }
}

/****************************************************************************
 * routines used when filling buffers
 ****************************************************************************/

/*
 * do_we_need_resync - look at the freq_ts given, and see if it matches
 * what we are expecting.
 * returns true if we need a resync, false if we're on or about the time 
 *  - silence_samples will contain the number of samples that freq_ts is
 *    past the expected time
 */  
bool CBufferAudioSync::do_we_need_resync (uint32_t freq_ts,
					  uint32_t &silence_samples)
{
  if (m_first_filled == false) return false;

  if (m_buffer_next_freq_ts == freq_ts)
    return false;

  int32_t sample_diff = freq_ts - m_buffer_next_freq_ts;
  int32_t compare_val = 4 * m_samples_per_frame;
  // note - the 2 is so that we handle rounding errors that occur when
  // converting timescales to frequency scales (ie: 90000 to 44100).
  if (sample_diff > 2 && sample_diff <= compare_val) {
    audio_message(LOG_DEBUG, "sample diff is %d %d", sample_diff, compare_val);
    audio_message(LOG_DEBUG, "freq_ts %u next %u", 
		  freq_ts, m_buffer_next_freq_ts);
    silence_samples = sample_diff;
    return false;
  } else if (sample_diff >= -1) {
    m_buffer_next_freq_ts = freq_ts; // rounding error, probably
    return false;
  }
  return true;
}

/*
 * wait_for_callback - will perform a wait until the callback routine
 * signals us
 */
void CBufferAudioSync::wait_for_callback (void)
{
  m_audio_waiting_buffer = true;
  SDL_SemWait(m_audio_waiting);
  m_audio_waiting_buffer = false;
}


/*
 * fill_silence will copy the correct number of silence bytes into
 * the output buffer
 */
void CBufferAudioSync::fill_silence (uint32_t silence_bytes)
{
  audio_message(LOG_INFO, "Inserting %u silence bytes at "U64" (%u)",
		silence_bytes, m_buffer_next_ts, m_buffer_next_freq_ts);

  bool lock = Lock();
  while (silence_bytes > 0) {
    uint32_t diff = m_sample_buffer_size - m_fill_offset;
    uint32_t copied = MIN(diff, silence_bytes);
    memset(m_sample_buffer + m_fill_offset, 
	   m_silence, 
	   copied);
    m_fill_offset += copied;
    m_filled_bytes += copied;
    if (m_fill_offset >= m_sample_buffer_size) {
      m_fill_offset = 0;
    }
    silence_bytes -= copied;
  }
  if (lock) Unlock();
}

/*
 * check_for_bytes makes sure that there is the correct number of
 * bytes in the buffer for us to insert
 */
bool CBufferAudioSync::check_for_bytes (uint32_t bytes,
					uint32_t silence_bytes)
{
  bool locked;
  uint32_t diff;
  uint32_t to_insert;
  uint32_t count;
  to_insert = bytes + silence_bytes;
  count = 0;

  // this has changed a bit - we're going to wait a couple of
  // callbacks, in case the output sample size rate is < the number
  // of bytes to fill.
  do {
    if (m_dont_fill) return false;

    locked = Lock();
    diff = m_sample_buffer_size - m_filled_bytes;
    if (locked) Unlock();
    
    if (diff >= to_insert) return true;
    count++;
    wait_for_callback();
  } while (count < 5);

  return false;
}

/**************************************************************************
 * APIs from plugins
 **************************************************************************/
/*
 * set_config - set up the audio stream configuration
 */
void CBufferAudioSync::set_config (uint32_t freq, 
				   uint32_t chans,
				   audio_format_t format, 
				   uint32_t samples_per_frame)
{
  if (m_audio_configured == false) {
    audio_message(LOG_DEBUG, "audio configure");
    m_freq = freq;
    m_decode_format = format;
    m_channels = chans;
    m_samples_per_frame = samples_per_frame;
    m_silence = 0;
    switch (format) {
    case AUDIO_FMT_U8:
      m_silence = 0x80;
      // fall through
    case AUDIO_FMT_S8:
    case AUDIO_FMT_HW_AC3:
      m_bytes_per_sample_input = sizeof(uint8_t);
      break;
    case AUDIO_FMT_U16LSB:
    case AUDIO_FMT_S16LSB:
    case AUDIO_FMT_U16MSB:
    case AUDIO_FMT_S16MSB:
    case AUDIO_FMT_U16:
    case AUDIO_FMT_S16:
      m_bytes_per_sample_input = sizeof(int16_t);
      break;
    case AUDIO_FMT_FLOAT:
      m_bytes_per_sample_input = sizeof(float);
      break;
    }
   
    // this can be used for converting directly from bytes to samples
    m_bytes_per_sample_input *= m_channels;

    if (samples_per_frame == 0) {
      if (config.get_config_value(CONFIG_LIMIT_AUDIO_SDL_BUFFER) > 1) {
	m_samples_per_frame = config.get_config_value(CONFIG_LIMIT_AUDIO_SDL_BUFFER);
      } else 
	m_samples_per_frame = m_freq / 20; // estimate for inserting silence
      m_sample_buffer_size = m_freq;
    } else {
      m_sample_buffer_size = 32 * m_samples_per_frame;
    }
    // allocate so if plugin gives the number of samples, that we
    // never have wrap...
    m_sample_buffer_size *= m_bytes_per_sample_input;
    m_sample_buffer = (uint8_t *)malloc(m_sample_buffer_size);
    audio_message(LOG_DEBUG, "ring buffer size is %u", m_sample_buffer_size);
    m_bytes_per_frame = m_samples_per_frame * m_bytes_per_sample_input;
    m_msec_per_frame = (m_samples_per_frame * 1000);
    m_msec_per_frame /= m_freq;
    m_audio_configured = true;
    m_psptr->send_sync_thread_a_message(MSG_AUDIO_CONFIGURED);
  }
}

/*
 * get_audio_buffer - get a fixed size buffer - # of samples passed in when
 * config'ed.  Paired with filled_audio_buffer to complete transaction.
 * because the get_audio_buffer and filled_audio_buffer do similiar things, 
 * and we might want to inject a couple of bytes of silence, we're just
 * using a temp buffer to fill.
 */
uint8_t *CBufferAudioSync::get_audio_buffer (uint32_t freq_ts,
					     uint64_t ts)
{

  if (m_dont_fill) {
    return NULL;
  }

  if (m_local_buffer == NULL) {
    m_local_buffer = (uint8_t *)malloc(m_bytes_per_frame);
  }

  m_got_buffer_ts = ts;
  m_got_buffer_freq_ts = freq_ts;
  return m_local_buffer;
}
  
/*
 * filled_audio_buffer - plugin is done writing frame into buffer
 */
void CBufferAudioSync::filled_audio_buffer (void)
{
  load_audio_buffer(m_local_buffer, 
		    m_bytes_per_frame, 
		    m_got_buffer_freq_ts,
		    m_got_buffer_ts);
}

/*
 * load_audio_buffer - used by routines that don't know how many
 * samples/bytes they need to write at 1 time
 */
void CBufferAudioSync::load_audio_buffer (const uint8_t *from,
					  uint32_t bytes,
					  uint32_t freq_ts,
					  uint64_t ts)
{
  uint32_t silence_bytes = 0;
  bool locked;
  bool need_resync;
  uint32_t samples;
  uint32_t write_bytes;

  if (m_dont_fill) return;
 
  if (m_decode_format == AUDIO_FMT_HW_AC3) {
    samples = 256 * 6;
    write_bytes = bytes + 2;
  } else {
    samples = bytes / m_bytes_per_sample_input;
    write_bytes = bytes;
  }

#ifdef DEBUG_AUDIO_FILL
  audio_message(LOG_DEBUG, "load samples %u ts "U64"(%u) - expect %u", 
		bytes, ts, freq_ts, m_buffer_next_freq_ts);
#endif
  need_resync = do_we_need_resync(freq_ts, silence_bytes);
  if (need_resync) {
    // we need to resync
    locked = Lock();
    if (m_have_resync) {
      // we have another resync pending asdf
      if (locked) Unlock();
      wait_for_callback();
    } else {
      if (locked) Unlock();
    }
  }
    
  // we may have a situation where we don't have enough for the
  // silence byte, but enough for the frame - this means resync - 
  // for now, we're not going to allow
  silence_bytes *= m_bytes_per_sample_input;
  if (check_for_bytes(write_bytes, silence_bytes) == false) {
    // can't insert.  If no silence bytes, return NULL
    if (silence_bytes == 0) {
      audio_message(LOG_ERR, "Couldn't insert encoded %u bytes at %u", 
		    write_bytes, freq_ts);
      return;
    }
    // See if we can insert this frame without the silence bytes - 
    // this will require a resync, but at least we don't lose anything
    silence_bytes = 0;
    need_resync = true;
    if (check_for_bytes(write_bytes, 0) == false) {
      audio_message(LOG_ERR, "Couldn't put %u resync bytes at %u", 
		    write_bytes, freq_ts);
      return;
    }
  }
  if (silence_bytes > 0) {
    fill_silence(silence_bytes);
  }
  
  locked = Lock();
  if (need_resync) {
    m_resync_offset = m_fill_offset;
    m_resync_ts = ts;
    m_resync_freq_ts = freq_ts;
    m_have_resync = need_resync;
    m_first_filled = false;
    // reset jitter calcs
    m_have_jitter = false;
    m_jitter_msec = 0;
    m_jitter_msec_total = 0;
  } else {
    if (m_first_filled) {
      int32_t diff = freq_ts - m_jitter_calc_freq_ts;
      int64_t calc_diff = (int64_t)diff;
      calc_diff *= TO_D64(1000);
      calc_diff /= (int64_t)m_freq;

      int64_t msec_diff = ts - m_jitter_calc_ts;
      int64_t add_diff;
      msec_diff -= m_jitter_msec_total;
      add_diff = msec_diff - calc_diff;
      if (add_diff != 0) {
#if 0
	static int64_t last_diff = 0;
	if (last_diff != msec_diff) {
	  audio_message(LOG_DEBUG, "first diff "D64 " %u", msec_diff,
			m_filled_bytes);
	  last_diff = msec_diff;
	}
#endif
	if (add_diff > TO_D64(10) || add_diff < TO_D64(-10)) {
	  audio_message(LOG_DEBUG, "jitter calc start %u now %u diff %d",
			m_jitter_calc_freq_ts, freq_ts, diff);
	  audio_message(LOG_DEBUG, "ts start "U64" now "U64" diff "D64,
			ts, m_jitter_calc_ts, msec_diff);
	  audio_message(LOG_DEBUG, "calc diff "D64" add "D64,
			calc_diff, add_diff);
	  m_jitter_msec_total += add_diff;
	  m_have_jitter = true;
	  m_jitter_msec += add_diff;
	}
      } 
      if (diff > (int32_t) m_freq || add_diff == 0) {
	m_jitter_calc_freq_ts = freq_ts;
	m_jitter_calc_ts = ts;
      }
    }
  }
  if (m_decode_format == AUDIO_FMT_HW_AC3) {
    // write header here
    m_sample_buffer[m_fill_offset] = bytes >> 8;
    m_fill_offset++;
    if (m_fill_offset >= m_sample_buffer_size) m_fill_offset = 0;
    m_sample_buffer[m_fill_offset] = bytes & 0xff;
    m_fill_offset++;
    if (m_fill_offset >= m_sample_buffer_size) m_fill_offset = 0;
  }

  // copy the bytes to the buffer - be aware of wrap at the end of
  // the buffer.
  while (bytes > 0) {
    uint32_t to_copy = MIN(bytes, m_sample_buffer_size - m_fill_offset);
    memcpy(m_sample_buffer + m_fill_offset, 
	   from,
	   to_copy);
    from += to_copy;
    m_fill_offset += to_copy;
    if (m_fill_offset >= m_sample_buffer_size) {
      m_fill_offset = 0;
    }
    bytes -= to_copy;
    m_filled_bytes += to_copy;
  }

  if (need_resync) {
    audio_message(LOG_DEBUG, "resync - freq ts %u should be %u",
		  freq_ts, m_buffer_next_freq_ts);
  }
  // record timestamp information for next frame.
  // note - if first_ts is used, we may need to move this inside the
  // mutex routines.
  if (m_first_filled == false) {
    m_first_ts = ts;
    m_first_freq_ts = freq_ts;
    m_first_filled = true;
    m_jitter_calc_freq_ts = freq_ts;
    m_jitter_calc_ts = ts;
  }
  /*
   * TODO note: we're going to need to calculate to see if there is
   * any jitter between the freq_ts and the ts
   */
  m_buffer_next_freq_ts = freq_ts + samples;
  m_buffer_next_ts = ts + (samples * 1000) / m_freq;
  m_have_resync |= need_resync;
  if (locked) Unlock();
#ifdef DEBUG_AUDIO_FILL
  audio_message(LOG_DEBUG, "filled %u bytes at %u - total %u %u", 
		write_bytes, freq_ts, m_filled_bytes, 
		m_sample_buffer_size);
#endif
}

/*
 * flush_decode_buffers - clear out the information so we can start
 * from scratch.
 */
void CBufferAudioSync::flush_decode_buffers (void)
{
  bool locked = Lock();
  m_dont_fill = false;
  m_first_filled = false;
  m_fill_offset = 0;
  m_play_offset = 0;
  m_filled_bytes = 0;
  // m_have_resync = true;  why ?
  m_audio_paused = true;
  m_have_jitter = false;
  m_jitter_msec = 0;
  m_jitter_msec_total = 0;
  if (locked) Unlock();
}


/****************************************************************************
 *  APIs from sync task
 ****************************************************************************/
int CBufferAudioSync::initialize_audio (int have_video)
{
  if (m_hardware_initialized == false) {
    if (m_audio_configured) {
      if (InitializeHardware() < 0) {
	return -1;
      }
      m_hardware_initialized = true;
     } else {
      return 0; // check again
    }
  }
  return 1; // yes, we're initialized
}
      
int CBufferAudioSync::is_audio_ready (uint64_t &disptime)
{
  disptime = m_first_ts;
#if 0
  audio_message(LOG_DEBUG, "%d %u %u %u", 
		m_dont_fill, m_filled_bytes, m_output_buffer_size_bytes,
		m_sample_buffer_size);
#endif
  return m_dont_fill == false && m_filled_bytes >= 2 * m_output_buffer_size_bytes;
}

// This will need to be rewritten.
bool CBufferAudioSync::check_audio_sync (uint64_t current_time, 
					 uint64_t &resync_time,
					 int64_t &wait_time,
					 bool &have_eof,
					 bool &restart_sync)
{
  // if we have an eof indication, wait until we're paused by the callback
  wait_time = 0;
  if (get_eof() != 0) {
    have_eof = m_audio_paused;
    return false;
  }

  if (m_have_resync && m_audio_paused) {
    // add latency to current_time
    wait_time = m_resync_ts - current_time;
    m_jitter_calc_ts = 
      m_first_ts = m_resync_ts;
    m_jitter_calc_freq_ts = 
      m_first_freq_ts = m_resync_freq_ts;
    if (wait_time > TO_D64(1000)) {
      // the resync time is greater than a second off.  We want to
      // change state.
      resync_time = m_resync_ts;
      return true;
    }
    if (wait_time > TO_D64(10)) {
      // we're still waiting - under 1 sec until continue - means we had
      // an outage of a number of frames.
      return false;
    }
    if (wait_time >= TO_D64(-10)) {
      // resync time is greater than 10 mseconds in the past.  We can
      // go ahead and restart
      audio_message(LOG_INFO, "restarting resync audio");
      m_have_resync = false;
      play_audio();
      return false;
    }
    // resync time is further in the past than 10 msec.  We should
    // just restart.
    resync_time = m_resync_ts;
    return true;
  } else if (m_audio_paused) {
    if (m_filled_bytes >= 2 * m_output_buffer_size_bytes) {
      // we have them resync the time here after we've gotten bytes
      resync_time = m_first_ts;

      m_jitter_calc_ts = 
	m_first_ts = resync_time;
      m_jitter_calc_freq_ts = 
	m_first_freq_ts = m_first_freq_ts;

      wait_time = resync_time - current_time;
      if (wait_time < 0) restart_sync = true;
      audio_message(LOG_INFO, 
		    "restart from stopped - resync time "U64" written "U64,
		    resync_time, m_samples_written);

      return true;
    } 
  }
  return false;
}

/*
 * play_audio - call from the sync task to start playing.
 */
void CBufferAudioSync::play_audio (void) 
{
  m_audio_paused = 0;
  m_first_callback = true;
  m_samples_written = 0;
  m_have_resync = false;
  m_have_jitter = false;
  m_jitter_msec = 0;
  m_jitter_msec_total = 0;
  StartHardware();
}

/*
 * flush_sync_buffers - basically, a pause from the sync side - 
 * stop the hardware, indicate that the decoder shouldn't fill, 
 * and trigger the decoder task if it is waiting
 */
void CBufferAudioSync::flush_sync_buffers (void)
{
  clear_eof();
  StopHardware();
  m_dont_fill = true;
  callback_done();
}

/***************************************************************************
 * routines used for callback
 ***************************************************************************/
/*
 * callback_done - wake the decoder task, if it's waiting for us
 * to get done
 */
void CBufferAudioSync::callback_done (void)
{
  if (m_audio_waiting_buffer) {
    m_audio_waiting_buffer = false;
    SDL_SemPost(m_audio_waiting);
  }
}

/*
 * audio_buffer_callback - callback that will put bytes in 
 * the output buffer. 
 * Inputs:
 *  outbuf - pointer to buffer
 *  len_bytes - length (in bytes, not sample) of output buffer
 *  latency_samples - latency in samples
 *  current_time - time latency measurement was taken
 */
bool CBufferAudioSync::audio_buffer_callback (uint8_t *outbuf, 
					      uint32_t len_bytes,
					      uint32_t latency_samples,
					      uint64_t current_time)
{
  uint32_t outBufferSamples, outBufferBytes;
  uint32_t decodedBufferBytes, decodedBufferSamples;
  uint64_t hw_start_time = 0;
  bool first_callback = m_first_callback;
#ifdef DEBUG_AUDIO_CALLBACK
  audio_message(LOG_DEBUG, "audio callback - filled %u bytes %u", 
		m_filled_bytes, len_bytes);
#endif
  // If we don't have any bytes pending, stop the hardware.  Let the
  // sync task resync us, if needed.
  if (m_filled_bytes == 0) {
    // this was commented out to let the sync task restart us... Let's
    // see if it works.
    //if (get_eof()) {
    // note - there are 2 places that do this - see below
      audio_message(LOG_DEBUG, "audio stopping");
      StopHardware();
      m_audio_paused = true;
      m_first_filled = false; 
      m_restart_request = true;
      m_have_jitter = false;
      m_jitter_msec = 0;
      m_jitter_msec_total = 0;
      m_psptr->wake_sync_thread();
      return false;
      //}
    // used to do a count of consecutive no buffers count here - 
    // I'm not sure that's a good idea.
    //callback_done();
    //return;
  }
  
  // Now, we're going to check the real timing vs. the theoretical
#ifdef DEBUG_AUDIO_TIMING
  audio_message(LOG_DEBUG, "samples "U64" latency %u", m_samples_written,
		latency_samples);
#endif
  if (m_first_callback == false &&
      m_samples_written > 5 * m_freq * m_bytes_per_sample_output) {
    // we're going to check time vs samples written, perhaps remove
    // or add samples.
    uint64_t written_samples = m_samples_written;
    uint64_t real_samples;
    uint64_t diff;
    bool add_samples;

    written_samples -= latency_samples;
    // this has something to do with when we have a latency reading
    // at the beginning.  We could have a case where the current time
    // is less than the start time.  I'm not sure why we flip the
    // sign and it seems to work, but it does.
    if (current_time >= m_hw_start_time) {
      real_samples = current_time - m_hw_start_time;
    } else {
      real_samples = m_hw_start_time - current_time;
    }
    real_samples *= m_freq;
    real_samples /= TO_U64(1000000);

    if (real_samples >= written_samples) {
      diff = real_samples - written_samples;
      add_samples = false;
    } else {
      diff = written_samples - real_samples;
      add_samples = true;
    }
#ifdef DEBUG_AUDIO_TIMING
    audio_message(LOG_DEBUG, "current "U64" hwstart "U64,
		  current_time, m_hw_start_time);
    audio_message(LOG_DEBUG, "samples "U64" real "D64 " diff "U64 " %d",
		  written_samples, real_samples, diff, add_samples);
#endif
    if (m_decode_format == AUDIO_FMT_HW_AC3) {
      /*
       * With AC3, we need to do whole frames worth - 256 * 6 samples
       */
      if (diff >= TO_U64(256 * 6)) {
	if (add_samples) {
	  m_total_samples_played += 256 * 6;
	  uint16_t len = 0;
	  CopyBytesToHardware((uint8_t *)&len,
			      outbuf, 
			      2);
	  len_bytes -= 2;
	  return true;
	} else {
	  uint16_t len = m_sample_buffer[m_play_offset] << 8;
	  m_play_offset++;
	  len |= m_sample_buffer[m_play_offset];
	  m_play_offset++;
	  m_play_offset += len;
	  m_filled_bytes -= len + 2;
	  m_samples_written += 256 * 6;
	}
      }
    } else {
      /*
       * regular PCM buffers - remove or add samples
       */
      if (add_samples) {
	if (diff >= TO_U64(3) &&
	    m_last_add_samples && 
	    m_last_diff >= TO_U64(3)) {
	  // notice that we don't touch m_samples_written
	  // also, we don't do this when play_offset is 0 - this means
	  // we're at a wrapping point, and we don't want to be there...
	  if (m_play_offset != 0) {
	    void *interp = interpolate_3_samples(&m_sample_buffer[m_play_offset]);
	    CopyBytesToHardware((uint8_t *)interp, 
				outbuf, 
				3 * m_bytes_per_sample_output);
	    
	    outbuf += 3 * m_bytes_per_sample_output;
	    len_bytes -= 3 * m_bytes_per_sample_output;
	    m_sync_samples_added += 3;
#if defined(DEBUG_AUDIO_TIMING) || defined(DEBUG_AUDIO_TIMING_CHANGES)
	    audio_message(LOG_INFO, "adding 3 samples of silence for clock sync "U64" %u",
			  diff, m_filled_bytes);
#endif
	  }
	}
      } else {
	if (diff >= TO_U64(10)) {
	  uint64_t bytes;
	  // need to remove samples
	  diff /= 2;
	  bytes = diff * m_bytes_per_sample_input;
	  if (bytes > m_filled_bytes) {
	    // restart here - this shouldn't happen...
	    audio_message(LOG_ERR, "sync requires removing "U64" bytes - we only have %u in the buffer", 
			  bytes, m_filled_bytes);
	  } else {
	    m_sync_samples_removed += diff;
	    m_samples_written += diff;
	    m_filled_bytes -= bytes;
	    m_play_offset += bytes;
	    if (m_play_offset > m_sample_buffer_size) {
	      m_play_offset -= m_sample_buffer_size;
	    }
	  }
#if defined(DEBUG_AUDIO_TIMING) || defined(DEBUG_AUDIO_TIMING_CHANGES)
	  audio_message(LOG_INFO, "removing "U64" samples for sync", 
			diff);
#endif
	}
      }
    }
    // note - at some point, we'll need to compare the msec timestamp
    // vs the frequency timestamp, to see if jitter has occurred there.
    // this will be done in the load and fill routines, and then passed
    // here or to the sync task - we'll want to adjust the start time.
    m_last_add_samples = add_samples;
    m_last_diff = diff;
  }
  if (m_decode_format == AUDIO_FMT_HW_AC3) {
    // first 2 bytes are len.
    uint16_t len = m_sample_buffer[m_play_offset] << 8;
    m_play_offset++;
    len |= m_sample_buffer[m_play_offset];
    m_play_offset++;
    if (m_play_offset >= m_sample_buffer_size) {
      m_play_offset = 0;
    }
    while (len > 0) {
      uint32_t to_copy;
      if (m_play_offset + len > m_sample_buffer_size) {
	to_copy = m_sample_buffer_size - m_play_offset;
      } else {
	to_copy = len;
      }
      CopyBytesToHardware(m_sample_buffer + m_play_offset, 
			  outbuf,
			  len);
      len -= to_copy;
      m_play_offset += len;
      if (m_play_offset >= m_sample_buffer_size) {
	m_play_offset = 0;
      }
    }
    m_filled_bytes -= len + 2;
    m_samples_written += 256 * 6; // always the same number of samples for AC3
    m_total_samples_played += 256 * 6;
  } else {
    bool done = false;
    if (m_filled_bytes / m_bytes_per_sample_input <
	len_bytes / m_bytes_per_sample_output) {
      audio_message(LOG_ERR, 
		    "filled bytes less than requested: %u (%u) req %u (%u)",
		    m_filled_bytes, m_bytes_per_sample_input,
		    len_bytes, m_bytes_per_sample_output);
    }
    while (done == false) {
      // We need to determine how many samples to write - the number
      // of output bytes might not be the number of bytes in the buffer - 
      // due to channel/format conversion
      if (m_have_resync) {
	// if we have resync, write up to the resync value
	if (m_resync_offset < m_play_offset) {
	  decodedBufferBytes = m_sample_buffer_size - m_play_offset;
	} else {
	  decodedBufferBytes = m_resync_offset - m_play_offset;
	}
      } else {
	// otherwise, make sure we're not going past the end of the
	// buffer
	if (m_play_offset + m_filled_bytes > m_sample_buffer_size) {
	  decodedBufferBytes = m_sample_buffer_size - m_play_offset;
	} else {
	  decodedBufferBytes = m_filled_bytes;
	}
      }

      // calculate the decoded samples from bytes
      decodedBufferSamples = decodedBufferBytes / m_bytes_per_sample_input;

      // calculate the number of samples we can write to the buffer
      outBufferSamples = len_bytes / m_bytes_per_sample_output;

      // take the min value
      outBufferSamples = MIN(decodedBufferSamples, outBufferSamples);

      m_samples_written += outBufferSamples;
      m_total_samples_played += outBufferSamples;
   
      // calculate the number of decoded and output buffer bytes
      decodedBufferBytes = outBufferSamples * m_bytes_per_sample_input;
      outBufferBytes = outBufferSamples * m_bytes_per_sample_output;

      if (first_callback) {
	hw_start_time = get_time_of_day_usec();
	first_callback = false;
      }
      if (m_convert_buffer) {
	// convert the data, and write it
	audio_convert_data(m_sample_buffer + m_play_offset,
			   outBufferSamples);
	CopyBytesToHardware((uint8_t *)m_convert_buffer, outbuf, outBufferBytes);
      } else {
	// no conversion, just write
	CopyBytesToHardware(m_sample_buffer + m_play_offset, 
			    outbuf, 
			    outBufferBytes);
      }
      // increment the output buffer
      outbuf += outBufferBytes;
      // increment the play offset
      m_play_offset += decodedBufferBytes;
      if (m_play_offset >= m_sample_buffer_size) {
	m_play_offset = 0;
      }
      // increment the number of bytes left to copy
      len_bytes -= outBufferBytes;
      // increment the number of bytes left in the decode buffer
      m_filled_bytes -= decodedBufferBytes;
      // 3 ways we are done - if we have no bytes to copy, 
      // if we have no bytes left in the buffer, 
      // or if we have resync and are now at the resync offset
      if (len_bytes == 0) {
	done = true;
      } else if (m_filled_bytes == 0) {
	audio_message(LOG_DEBUG, "no filled bytes");
	done = true;
      } else if (m_have_resync && m_play_offset == m_resync_offset) {
	done = true;
      }
    }
  }

  // TODO note - if m_filled_bytes is 0 here, we're going to have some
  // problems - we might want to resync at the fill offset, because
  // our # of samples written is not going to equal filled bytes - 
  // that is if len != 0
  if (m_filled_bytes == 0 && len_bytes != 0 && get_eof() == false) {
    // it's not really a resync - it's more like indicate that we
    // need to restart the sync process
    audio_message(LOG_DEBUG, "ran out of bytes");
    StopHardware();
    m_first_filled = false; 
    m_restart_request = true;
    m_audio_paused = true;
    m_have_jitter = false;
    m_jitter_msec = 0;
    m_jitter_msec_total = 0;
    m_psptr->wake_sync_thread(); 
  }
#ifdef DEBUG_AUDIO_CALLBACK
  audio_message(LOG_DEBUG, "callback fillbytes %u", m_filled_bytes);
#endif
  if (m_have_resync && m_play_offset == m_resync_offset) {
    // need to do resync
    audio_message(LOG_INFO, "stopping audio callback for resync");
    StopHardware();
    m_audio_paused = true;
    m_psptr->wake_sync_thread();
  }

  if (m_first_callback) {
    m_first_callback = false;
    // need to adjust hw_start_time by latency, so we can determine
    // when to start the video
    uint64_t latency_usec = 0;
    if (latency_samples > 0) {
      latency_usec = latency_samples * TO_U64(1000000);
      latency_usec /= m_freq;
    }
    m_hw_start_time = hw_start_time + latency_usec;
    m_latency_value = latency_usec / TO_U64(1000);
#ifdef DEBUG_AUDIO_TIMING
    audio_message(LOG_DEBUG, "hw start "U64" latency "U64,
		  m_hw_start_time, latency_usec);
#endif
    // 
    m_psptr->audio_is_ready(m_latency_value, m_first_ts);
  } else {
    if (m_have_jitter) {
      audio_message(LOG_DEBUG, "adjusting start time "D64" due to jitter",
		    m_jitter_msec);
      m_psptr->adjust_start_time(m_jitter_msec);
      m_have_jitter = false;
      m_jitter_msec = 0;
    }
  }
  callback_done();
  return true;
}

void CBufferAudioSync::display_status (void)
{
  audio_message(LOG_DEBUG, "audio buffer %d %u next %u "U64, 
		m_audio_waiting_buffer, 
		m_filled_bytes, m_buffer_next_freq_ts, m_buffer_next_ts);
}


/* end file audio_buffer.cpp */
