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
 * sync.cpp - provide sync thread implementations of CPlayerSession
 */
#include <stdlib.h>
#include "player_session.h"
#include <SDL.h>
#include <SDL_thread.h>
#include "player_util.h"

/*
 * get_current_time.  Gets the time of day, subtracts off the start time
 * to get the current play time.
 */
uint64_t CPlayerSession::get_current_time ()
{
  uint64_t current_time;
  struct timeval t;
  struct timezone z;

  gettimeofday(&t, &z);
  current_time = (t.tv_sec * 1000) + (t.tv_usec / 1000);
  
  if (current_time < m_start) {
    if (m_clock_wrapped == -1) {
      return (0);
    } else {
      m_clock_wrapped = 1;
    }
  } else{
    if (m_clock_wrapped > 0) {
	  uint64_t temp;
	  temp = 1;
	  temp <<= 32;
	  temp /= 1000;
      current_time += temp;
    } else {
      m_clock_wrapped = 0;
    }
  }
  return(current_time - m_start);
}

/*
 * sync_thread - where the work is done for the sync
 */
int CPlayerSession::sync_thread (void) 
{
  int init_done;
  m_clock_wrapped = 0;
  int synced;
  CMsg *newmsg;

  /*
   * First up - wait until the audio/video sync classes indicate they're 
   * ready.
   */
  do {
    init_done = 0;
    int ret = 1;
    for (CPlayerMedia *mptr = m_my_media;
	 mptr != NULL && ret == 1;
	 mptr = mptr->get_next()) {
      if (mptr->is_video()) {
	ret = m_video_sync->initialize_video(m_session_name,
					     m_screen_pos_x,
					     m_screen_pos_y);
      } else {
	ret = m_audio_sync->initialize_audio(m_video_sync != NULL);
      }
    }
    if (ret == -1) {
      player_error_message("Fatal error while initializing hardware");
      if (m_video_sync != NULL) {
	m_video_sync->flush_sync_buffers();
      }
      if (m_audio_sync != NULL) {
	m_audio_sync->flush_sync_buffers();
      }
      m_master_msg_queue->send_message(MSG_SESSION_FINISHED, 
				       NULL, 
				       0, 
				       m_master_msg_queue_sem);
      m_hardware_error = 1;
    }
	 
    if (ret == 1)
      init_done = 1; 
    else {
      SDL_Delay(100);
    }
  } while (init_done == 0 && m_hardware_error == 0);

  if (m_hardware_error == 1) {
#ifndef _WINDOWS
    while (1) {
      SDL_SemWait(m_sync_sem);
      while ((newmsg = m_sync_thread_msg_queue.get_message()) != NULL) {
	if (newmsg->get_value() == MSG_STOP_THREAD) {
	  return(0);
	}
      }
    }
#else
	return (-1);
#endif
  }
      
  m_current_time = 0;
  struct timeval t;
  struct timezone z;

  gettimeofday(&t, &z);
  m_start = (t.tv_sec * 1000) + (t.tv_usec / 1000);
  synced = 0;
  uint64_t audio_resync_time = 0, video_resync_time = 0;
  int64_t video_status = 0;
  int wait_for_signal = 1;
  int stop_thread = 0;
  int is_paused = 1;
  int playing = 1;
  int have_audio_eof = 0;
  int have_video_eof = 0;

  while (stop_thread == 0) {
    while ((newmsg = m_sync_thread_msg_queue.get_message()) != NULL) {
      switch (newmsg->get_value()) {
      case MSG_PAUSE_SESSION:
	synced = 0;
	is_paused = 1;
	playing = 0;
	if (m_video_sync != NULL) {
	  m_video_sync->flush_sync_buffers();
	}
	if (m_audio_sync != NULL) {
	  m_audio_sync->flush_sync_buffers();
	}
	player_debug_message("Sync got pause");
	m_sync_pause_done = 1;
	break;
      case MSG_START_SESSION:
	playing = 1;
	have_audio_eof = 0;
	have_video_eof = 0;
	player_debug_message("Sync got Start");
	break;
      case MSG_STOP_THREAD:
	stop_thread = 1;
	break;
      case MSG_SYNC_RESIZE_SCREEN:
	if (m_video_sync) {
	  m_video_sync->do_video_resize();
	}
	break;
      default:
	player_error_message("Sync thread received message %d", 
			     newmsg->get_value());
	break;
      }
      delete newmsg;
      newmsg = NULL;
    }
    /*
     * Various process that needs to be done if we got a message
     */
    if (stop_thread == 1) {
      continue;
    }
    if (is_paused == 1) {
      if (stop_thread == 0 && playing == 0) {
	SDL_SemWait(m_sync_sem);
	continue;
      }
    } else if (m_waiting_for_audio != 0) {
      SDL_SemWait(m_sync_sem);
      continue;
    }

    /*
     * See if we have audio/video sync 
     */
    if (synced == 0) {
      // We're not synced.  See if the video is ready, and if the audio
      // is ready.  Then start them going...
      int vsynced = 1, asynced = 1, ret;
      uint64_t astart, vstart;
      if (m_video_sync) {
	ret = m_video_sync->is_video_ready(vstart);
	if (ret == 0) {
	  vsynced = 0;
	} else {
	  //player_debug_message("return from video start is %llu", vstart);
	}
      } 

      if (m_audio_sync) {
	ret = m_audio_sync->is_audio_ready(astart); 
	if (ret == 0) {
	  asynced = 0;
	} else {
	  player_debug_message("return from check_audio is %llu", astart);
	}
      }
      if (vsynced == 1 && asynced == 1) {
	/*
	 * Audio and video are synced. 
	 */
	synced = 1;
	if (m_audio_sync) {
	  /* 
	   * If we have audio, we use that for syncing.  Start it up
	   */
	  m_current_time = astart;
	  m_waiting_for_audio = 1;
	  m_audio_sync->play_audio();
	  is_paused = 0;
	} else 	if (m_video_sync) {
	  /*
	   * Video only - set up the start time based on the video time
	   * returned
	   */
	  m_current_time = vstart;
	  m_waiting_for_audio = 0;
	  gettimeofday(&t, &z);
	  m_start = (t.tv_sec * 1000) + (t.tv_usec / 1000);
	  if (is_paused == 1) {
	    m_start -= m_current_time;
	    is_paused = 0;
	  }
	}
	player_debug_message("Resynced at time %llu", m_current_time);
	m_session_state = SESSION_PLAYING;
	/*
	 * Play off some video
	 */
	if (m_video_sync && m_current_time >= vstart)
	  m_video_sync->play_video();
	/*
	 * If we have audio, we need to wait until it starts up.
	 * The audio sync class will indicate this to us.
	 */
	if (m_waiting_for_audio == 1) {
	  continue;
	}
      } else {
	SDL_Delay(100);
	continue;
      }
    }

    // current time is time in msec since we started.
    m_current_time = get_current_time();
    if (m_audio_sync) {
      audio_resync_time = m_audio_sync->check_audio_sync(m_current_time, 
							 have_audio_eof);
    }
    if (m_video_sync) {

      video_status = m_video_sync->play_video_at(m_current_time, 
						 video_resync_time,
						 have_video_eof);
    }

    if (m_video_sync && m_audio_sync) {
      if (video_status > 0 || audio_resync_time == 0) {
	if (video_status < 9) {
	  wait_for_signal = 0;
	} else {
	  wait_for_signal = 1;
	}
      } else {
	wait_for_signal = 0;
      }
    } else if (m_video_sync) {
      if (have_video_eof == 1) {
#ifndef _WINDOWS
	m_master_msg_queue->send_message(MSG_SESSION_FINISHED,
					 NULL,
					 0, 
					 m_master_msg_queue_sem);
	wait_for_signal = 1;
#else
	return (1);
#endif
      } else if (video_status >= 9) {
	wait_for_signal = 1;
      } else {
	wait_for_signal = 0;
      }
    } else {
      // audio only
      if (have_audio_eof == 1) {
#ifndef _WINDOWS
	m_master_msg_queue->send_message(MSG_SESSION_FINISHED, 
					 NULL, 
					 0, 
					 m_master_msg_queue_sem);
	wait_for_signal = 1;
#else
	return (1);
#endif
      } else if (audio_resync_time != 0) {
#if 0
	player_debug_message("Audio resync is %llu", audio_resync_time);
	player_debug_message("Current time is %llu", m_current_time);
#endif
	if (audio_resync_time - m_current_time > 750) {
	  wait_for_signal = 1;
	} else {
	  wait_for_signal = 0;
	}
      } else {
	wait_for_signal = 1;
      }
    }
    // This most likely needs some work...
    if (wait_for_signal) {
      int ret = SDL_SemWaitTimeout(m_sync_sem, 9);
      if (ret == SDL_MUTEX_TIMEDOUT && is_paused == 0) {
	if (m_range != NULL) {
	  uint64_t range_end;
	  range_end = (uint64_t)(m_range->range_end * 1000.0);
	  if (m_current_time > range_end) {
	    //player_debug_message("timeout reached %llu", m_current_time);
#ifndef _WINDOWS
	    m_master_msg_queue->send_message(MSG_SESSION_FINISHED, 
					     NULL, 
					     0, 
					     m_master_msg_queue_sem);
	    SDL_SemWait(m_sync_sem);
#else
		return (1);
#endif
	  }
	} else {
	  if ((m_audio_sync == NULL || have_audio_eof == 1) &&
	      (m_video_sync == NULL || have_video_eof == 1)) {
	    SDL_SemWait(m_sync_sem);
	  }
	}
      }
    }
  }
  return (0);
}

/*
 * audio_is_ready - when the audio indicates that it's ready, it will
 * send a latency number, and a play time
 */
void CPlayerSession::audio_is_ready (uint64_t latency, uint64_t time)
{
  struct timeval t;
  struct timezone z;

  m_waiting_for_audio = 0;
  gettimeofday(&t, &z);
  m_start = (t.tv_sec * 1000) + (t.tv_usec / 1000);
  m_start -= time;
  m_start += latency;
  if (latency != 0) {
    m_clock_wrapped = -1;
  }
  player_debug_message("Audio is ready %llu - latency %llu", time, latency);
  SDL_SemPost(m_sync_sem);
}

/* end sync.cpp */
