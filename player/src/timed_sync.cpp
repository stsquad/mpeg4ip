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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "sync.h"
#include "mpeg4ip_utils.h"

CTimedSync::CTimedSync (const char *name, 
			CPlayerSession *psptr) : CSync(psptr)
{
  m_next = NULL;
  m_name = name;
  m_consec_skipped = 0;
  m_behind_frames = 0;
  m_total_frames = 0;
  m_filled_frames = 0;
  m_behind_time = 0;
  m_behind_time_max = 0;
  m_skipped_render = 0;
  m_msec_per_frame = 0;
  m_max_buffers = 0;
  m_buffer_filled = NULL;
  m_last_filled_time = TO_U64(0x7fffffffffffffff);
  m_config_set = false;
  m_initialized = false;
  m_decode_waiting = false;
  m_dont_fill = false;
  m_fill_index = m_play_index = m_max_buffers = 0;
}

CTimedSync::~CTimedSync (void)
{
  message(LOG_NOTICE, m_name, "%s Sync Stats:", m_name);
  message(LOG_NOTICE, m_name, "Displayed-behind frames %d", m_behind_frames);
  message(LOG_NOTICE, m_name, "Total frames displayed %d", m_total_frames);
  message(LOG_NOTICE, m_name, "Max behind time " U64, m_behind_time_max);
  if (m_behind_frames > 0) 
    message(LOG_NOTICE, m_name, "Average behind time "U64, m_behind_time / m_behind_frames);
  message(LOG_NOTICE, m_name, "Skipped rendering %u", m_skipped_render);
  message(LOG_NOTICE, m_name, "Filled frames %u", m_filled_frames);

  CHECK_AND_FREE(m_buffer_filled);
  CHECK_AND_FREE(m_play_this_at);
}
void CTimedSync::initialize_indexes (uint32_t num_buffers) 
{
  void *temp = malloc(sizeof(bool) * num_buffers);
  memset(temp, false, sizeof(bool) * num_buffers);
  m_buffer_filled = (volatile bool *)temp;
  temp = malloc(sizeof(uint64_t) * num_buffers);
  m_play_this_at = (uint64_t *)temp;
  m_max_buffers = num_buffers;
}

void CTimedSync::increment_fill_index (void) 
{
  m_buffer_filled[m_fill_index] = true;
  m_fill_index++;
  m_fill_index %= m_max_buffers;
  m_filled_frames++;
}
 
void CTimedSync::increment_play_index (void) 
{
  m_buffer_filled[m_play_index] = false;
  m_play_index++;
  m_play_index %= m_max_buffers;
  m_total_frames++;
}
 
void CTimedSync::notify_decode_thread (void) 
{
  if (m_decode_waiting) {
    // If the decode thread is waiting, signal it.
    m_decode_waiting = false;
    SDL_SemPost(m_decode_sem);
  }
}

bool CTimedSync::have_buffer_to_fill (void) 
{
  if (dont_fill()) return false;

  if (m_buffer_filled[m_fill_index]) {
    m_decode_waiting = true;
    SDL_SemWait(m_decode_sem);
    if (dont_fill()) return false;
    if (m_buffer_filled[m_fill_index]) {
#ifdef VIDEO_SYNC_FILL
      video_message(LOG_DEBUG, "Wait but filled %d", m_fill_index);
#endif
      return false;
    }
  }  
  return true;
}

void CTimedSync::save_last_filled_time (uint64_t ts) 
{
  if (ts > m_last_filled_time) {
    uint64_t temp;
    temp = ts - m_last_filled_time;
    if (temp < m_msec_per_frame) {
      m_msec_per_frame = temp;
    }
  }
  m_last_filled_time = ts;
}

bool CTimedSync::play_at (uint64_t current_time, 
			    bool have_audio_resync,
			    uint64_t &next_time,
			    bool &have_eof)
{
  uint64_t play_this_at = 0;
  bool done = false;
  bool used_frame = false;
  bool have_next_time = true;
  while (done == false) {
    /*
     * m_buffer_filled is ring buffer of YUV data from decoders, with
     * timestamps.
     * If the next buffer is not filled, indicate that, as well
     */
    //message(LOG_DEBUG, "ts", "play %d", m_buffer_filled[m_play_index]);
    if (m_buffer_filled[m_play_index] == false) {
      /*
       * If we have end of file, indicate it
       */
      have_eof = get_eof();
      done = true;
      have_next_time = false;
      continue;
    }
  
    /*
     * we have a buffer.  If it is in the future, don't play it now
     */
    play_this_at = m_play_this_at[m_play_index];
#if 0
    message(LOG_DEBUG, "ts", "play "U64" at "U64 " %d "U64, play_this_at, current_time,
		  m_play_index, m_msec_per_frame);
#endif
    if (play_this_at > current_time) {
      have_next_time = true;
      done = true;
      continue;
    }


    /*
     * If we're behind - see if we should just skip it
     */
    if (play_this_at < current_time) {
      uint64_t behind = current_time - play_this_at;
      if (have_audio_resync && behind > TO_U64(1000)) {
	have_next_time = true;
	done = true;
	continue;
      }
      m_behind_frames++;
      m_behind_time += behind;
      if (behind > m_behind_time_max) m_behind_time_max = behind;
#if 0
      if ((m_behind_frames % 64) == 0) {
	video_message(LOG_DEBUG, "behind "U64" avg "U64" max "U64,
		      behind, m_behind_time / m_behind_frames,
		      m_behind_time_max);
      }
#endif
    }
    /*
     * If we're within 1/2 of the frame time, go ahead and display
     * this frame
     */
#if 1
    // a new thought (2005-03-18, wmay) - if we don't have the next
    // frame filled, or the current time is less than the next time, 
    // go ahead and render.
    uint32_t play_index = m_play_index;
    increment_play_index();
#if 0
    message(LOG_DEBUG, "ts", "next %d "U64" "U64,
	    m_buffer_filled[m_play_index],
	    m_play_this_at[m_play_index],
	    current_time);
#endif

    if (m_buffer_filled[m_play_index] == false ||
	m_play_this_at[m_play_index] > current_time) {
      render(play_index);
    } else {
      m_skipped_render++;
      m_consec_skipped++;
#if 0
      message(LOG_DEBUG, "ts", "skipped "U64" current "U64,
	      m_play_this_at[m_play_index], current_time);
#endif
    }
#else
    if ((play_this_at + m_msec_per_frame) > current_time) {
      m_consec_skipped = 0;
      render(m_play_index);
    } else {
      /*
       * Else - we're lagging - just skip and hope we catch up...
       */
      m_skipped_render++;
      m_consec_skipped++;
    }
    /*
     * Advance the ring buffer, indicating that the decoder can fill this
     * buffer.
     */
    increment_play_index();
#endif
    used_frame = true;
  }
  // end of loop - notify decode thread outside loop
  if (used_frame) {
    notify_decode_thread();
  }
  next_time = play_this_at;
  return have_next_time;
}




void CTimedSync::flush_sync_buffers (void)
{
  // Just restart decode thread if waiting...
  m_dont_fill = true;
  flush();
  clear_eof();
  notify_decode_thread();
}

// called from decode thread on both stop/start.
void CTimedSync::flush_decode_buffers (void)
{
  if (m_buffer_filled != NULL) {
    for (uint32_t ix = 0; ix < m_max_buffers; ix++) {
      m_buffer_filled[ix] = false;
    }
  }

  m_fill_index = m_play_index = 0;
  m_dont_fill = false;
}
