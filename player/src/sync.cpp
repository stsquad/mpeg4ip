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
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter @maersk-moller.net
 */
/*
 * sync.cpp - provide sync thread implementations of CPlayerSession
 */
#include <stdlib.h>
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "audio.h"
#include "video.h"
#include "our_config_file.h"

#define DEBUG_SYNC_STATE 1
//#define DEBUG_SYNC_MSGS 1
//#define DEBUG_SYNC_SDL_EVENTS 1

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(sync_message, "avsync")
#else
#define sync_message(loglevel, fmt...) message(loglevel, "avsync", fmt)
#endif



/*
 * Sync thread states.  General state machine looks like:
 * INIT -> WAIT_SYNC -> WAIT_AUDIO -> PLAYING -> DONE -> EXIT
 * PAUSE is entered when a PAUSE command is sent.  PAUSE exits into
 * WAIT_SYNC.  EXIT is entered when a QUIT command is received.
 */
enum {
  SYNC_STATE_INIT = 0,
  SYNC_STATE_WAIT_SYNC = 1,
  SYNC_STATE_WAIT_AUDIO = 2,
  SYNC_STATE_PLAYING = 3,
  SYNC_STATE_PAUSED = 4,
  SYNC_STATE_AUDIO_RESYNC = 5,
  SYNC_STATE_DONE = 6,
  SYNC_STATE_EXIT = 7,
};

#ifdef DEBUG_SYNC_STATE
const char *sync_state[] = {
  "Init",
  "Wait Sync",
  "Wait Audio",
  "Playing",
  "Paused",
  "Audio Resync",
  "Done",
  "Exit"
};
#endif
/*
 * process_sdl_events - process the sdl event queue.  This will allow the
 * video window to capture things like close, key strokes.
 */
void CPlayerSession::process_sdl_events (void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event) == 1) {
    switch (event.type) {
    case SDL_QUIT:
      m_master_msg_queue->send_message(MSG_RECEIVED_QUIT,
				       NULL, 
				       0,
				       m_master_msg_queue_sem);
#ifdef DEBUG_SYNC_SDL_EVENTS
      sync_message(LOG_DEBUG, "Quit event detected");
#endif
      break;
    case SDL_KEYDOWN:
#ifdef DEBUG_SYNC_SDL_EVENTS
      sync_message(LOG_DEBUG, "Pressed %x %s", 
		   event.key.keysym.mod, SDL_GetKeyName(event.key.keysym.sym));
#endif
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
	if (m_video_sync &&
	    m_video_sync->get_fullscreen() != 0) {
	  m_video_sync->set_fullscreen(0);
	  m_video_sync->do_video_resize();
	}
	break;
      case SDLK_RETURN:
	if ((event.key.keysym.mod & (KMOD_ALT | KMOD_META)) != 0) {
	  // alt-enter - full screen
	  if (m_video_sync &&
	      m_video_sync->get_fullscreen() == 0) {
	    m_video_sync->set_fullscreen(1);
	    m_video_sync->do_video_resize();
	  }
	}
	break;
      default:
	break;
      }
      sdl_event_msg_t msg;
      msg.mod = event.key.keysym.mod;
      msg.sym = event.key.keysym.sym;
      m_master_msg_queue->send_message(MSG_SDL_KEY_EVENT,
				       (unsigned char *)&msg,
				       sizeof(msg),
				       m_master_msg_queue_sem);
      break;
    default:
      break;
    }
  }
}

/*
 * process_msg_queue - receive messages for the sync task.  Most are
 * state changes.
 */
int CPlayerSession::process_msg_queue (int state) 
{
  CMsg *newmsg;
  while ((newmsg = m_sync_thread_msg_queue.get_message()) != NULL) {
#ifdef DEBUG_SYNC_MSGS
    sync_message(LOG_DEBUG, "Sync thread msg %d", newmsg->get_value());
#endif
    switch (newmsg->get_value()) {
    case MSG_PAUSE_SESSION:
      state = SYNC_STATE_PAUSED;
      break;
    case MSG_START_SESSION:
      state = SYNC_STATE_WAIT_SYNC;
      break;
    case MSG_STOP_THREAD:
      state = SYNC_STATE_EXIT;
      break;
    case MSG_SYNC_RESIZE_SCREEN:
      if (m_video_sync) {
	m_video_sync->set_screen_size(m_screen_scale);
	m_video_sync->set_fullscreen(m_fullscreen);
	m_video_sync->do_video_resize(m_pixel_width, m_pixel_height, m_max_width, m_max_height);
      }
      break;
    default:
      sync_message(LOG_ERR, "Sync thread received message %d", 
		   newmsg->get_value());
      break;
    }
    delete newmsg;
    newmsg = NULL;
  }
  return (state);
}

/***************************************************************************
 * Sync thread state handlers
 ***************************************************************************/

/*
 * sync_thread_init - wait until all the sync routines are initialized.
 */
int CPlayerSession::sync_thread_init (void)
{
  int ret = 1;

  uint media_count = 0, media_initialized = 0;

  for (CPlayerMedia *mptr = m_my_media;
       mptr != NULL && ret >= 0;
       mptr = mptr->get_next()) {
    if (mptr->is_video()) {
      // initialize the video size from the player session information
      media_count++;
      m_video_sync->set_screen_size(m_screen_scale);
      m_video_sync->set_fullscreen(m_fullscreen);
      ret = m_video_sync->initialize_video(m_session_name,
					   m_screen_pos_x,
					   m_screen_pos_y);
      if (ret > 0) media_initialized++;
    } else {
      media_count++;
      ret = m_audio_sync->initialize_audio(m_video_sync != NULL);
      if (ret > 0) media_initialized++;
    }
  }
  if (media_count > 0 && media_count == media_initialized) {
    return (SYNC_STATE_WAIT_SYNC); 
  } 

  if (media_count > 0) {
    m_init_tries_made++;
    if (m_init_tries_made > 50) {
      sync_message(LOG_CRIT, "One media is not initializing; it might not be receiving correctly");
      ret = -1;
    }
  }
  if (ret == -1) {
    sync_message(LOG_CRIT, "Fatal error while initializing hardware");
    if (m_video_sync != NULL) {
      m_video_sync->flush_sync_buffers();
    }
    if (m_audio_sync != NULL) {
      m_audio_sync->flush_sync_buffers();
    }
    m_master_msg_queue->send_message(MSG_RECEIVED_QUIT, 
				     NULL, 
				     0, 
				     m_master_msg_queue_sem);
    m_hardware_error = 1;
    return (SYNC_STATE_DONE);
  }
	

  CMsg *newmsg;

  newmsg = m_sync_thread_msg_queue.get_message();
  if (newmsg != NULL) {
    int value = newmsg->get_value();
    delete newmsg;

    if (value == MSG_STOP_THREAD) {
      return (SYNC_STATE_EXIT);
    }
    if (value == MSG_PAUSE_SESSION) {
      m_sync_pause_done = 1;
    }
  }

  SDL_Delay(100);
	
  return (SYNC_STATE_INIT);
}

/*
 * sync_thread_wait_sync - wait until all decoding threads have put
 * data into the sync classes.
 */
int CPlayerSession::sync_thread_wait_sync (void)
{
  int state;

  state = process_msg_queue(SYNC_STATE_WAIT_SYNC);
  if (state == SYNC_STATE_WAIT_SYNC) {
      
      // We're not synced.  See if the video is ready, and if the audio
      // is ready.  Then start them going...
      int vsynced = 1, asynced = 1;
      uint64_t astart = 0, vstart = 0;

      if (m_video_sync) {
	vsynced = m_video_sync->is_video_ready(vstart);
      } 

      if (m_audio_sync) {
	asynced = m_audio_sync->is_audio_ready(astart); 
      }
      if (vsynced == 1 && asynced == 1) {
	/*
	 * Audio and video are synced. 
	 */
	if (m_audio_sync) {
	  /* 
	   * If we have audio, we use that for syncing.  Start it up
	   */
	  m_first_time_played = astart;
	  m_current_time = astart;
	  sync_message(LOG_DEBUG, "Astart is "U64, astart);
	  if (m_video_sync) 
	    sync_message(LOG_DEBUG, "Vstart is "U64, vstart);
	  m_waiting_for_audio = 1;
	  state = SYNC_STATE_WAIT_AUDIO;
	  m_audio_sync->play_audio();
	} else 	if (m_video_sync) {
	  /*
	   * Video only - set up the start time based on the video time
	   * returned
	   */
	  m_first_time_played = vstart;
	  m_current_time = vstart;
	  m_waiting_for_audio = 0;
	  m_start = get_time_of_day();
	  m_start -= m_current_time;
	  state = SYNC_STATE_PLAYING;
	}
	sync_message(LOG_DEBUG, 
		     "Resynced at time "U64 " "U64, m_current_time, vstart);
      } else {
	SDL_Delay(10);
      }
    }
  return (state);
}

/*
 * sync_thread_wait_audio - wait until the audio thread starts and signals
 * us.
 */
int CPlayerSession::sync_thread_wait_audio (void)
{
  int state;

  state = process_msg_queue(SYNC_STATE_WAIT_AUDIO);
  if (state == SYNC_STATE_WAIT_AUDIO) {
    if (m_waiting_for_audio != 0) {
      SDL_SemWaitTimeout(m_sync_sem, 10);
    } else {
      // make sure we set the current time
      get_current_time();
      sync_message(LOG_DEBUG, "Current time is "U64, m_current_time);
      return (SYNC_STATE_PLAYING);
    }
  }
  return (state);
}

/*
 * sync_thread_playing - do the work of displaying the video and making
 * sure we're in sync.
 */
int CPlayerSession::sync_thread_playing (void) 
{
  int state;
  uint64_t audio_resync_time = 0;
  int64_t wait_audio_time;
  bool video_have_next_time = false;
  uint64_t video_next_time;
  bool have_audio_eof = false;
  bool need_audio_restart = false;
  bool need_audio_resync = false;
  bool have_video_eof = false;

  state = process_msg_queue(SYNC_STATE_PLAYING);
  if (state == SYNC_STATE_PLAYING) {
    get_current_time();
    if (m_audio_sync) {
      need_audio_resync = m_audio_sync->check_audio_sync(m_current_time, 
							 audio_resync_time,
							 wait_audio_time,
							 have_audio_eof,
							 need_audio_restart);
      if (need_audio_resync) {
	if (m_video_sync) {
	  // wait for video to play out
	  return SYNC_STATE_AUDIO_RESYNC;
	} else {
	  // skip right to the audio
	  return SYNC_STATE_WAIT_SYNC;
	}
      }
    }
    if (m_video_sync) {
      video_have_next_time = m_video_sync->play_video_at(m_current_time, 
						 false,
						 video_next_time,
						 have_video_eof);
    }

    int delay = 9;
    bool have_delay = false;

    if (m_video_sync && m_audio_sync) {
      if (have_video_eof && have_audio_eof) {
	return (SYNC_STATE_DONE);
      }
      if (video_have_next_time || wait_audio_time > 0) {
	have_delay = true;
	if (video_have_next_time) {
	  int64_t video_wait_time = video_next_time - m_current_time;
	  delay = (int)MIN(wait_audio_time, video_wait_time);
	} else {
	  delay = wait_audio_time;
	}
      }
    } else if (m_video_sync) {
      if (have_video_eof) {
	return (SYNC_STATE_DONE);
      } 
      if (video_have_next_time) {
	have_delay = true;
	delay = (int)(video_next_time - m_current_time);
      }
    } else {
      // audio only
      if (have_audio_eof) {
	return (SYNC_STATE_DONE);
      }
      if (audio_resync_time != 0) {
	have_delay = true;
	delay = (int)(audio_resync_time - m_current_time);
      }
    }
      
    if (have_delay) {
      if (delay >= 9) {
	delay = 9;
	SDL_SemWaitTimeout(m_sync_sem, delay);
      }
    } else {
      SDL_SemWaitTimeout(m_sync_sem, 10);
    }
  }
  return (state);
}

/*
 * sync_thread_pause - wait for messages to continue or finish
 */
int CPlayerSession::sync_thread_paused (void)
{
  int state;
  state = process_msg_queue(SYNC_STATE_PAUSED);
  if (state == SYNC_STATE_PAUSED) {
    SDL_SemWaitTimeout(m_sync_sem, 10);
  }
  return (state);
}

/*
 * sync_thread_pause - wait for messages to exit, pretty much.
 */
int CPlayerSession::sync_thread_done (void)
{
  int state;
  state = process_msg_queue(SYNC_STATE_DONE);
  if (state == SYNC_STATE_DONE) {
    SDL_SemWaitTimeout(m_sync_sem, 10);
  } 
  return (state);
}

int CPlayerSession::sync_thread_audio_resync (void) 
{
  int state;
  uint64_t audio_resync_time = 0;
  int64_t wait_audio_time;
  bool video_have_next_time = false;
  uint64_t video_next_time;
  bool have_audio_eof = false;
  bool need_audio_resync = false;
  bool have_video_eof = false;
  bool need_audio_restart = false;

  state = process_msg_queue(SYNC_STATE_AUDIO_RESYNC);
  if (state == SYNC_STATE_AUDIO_RESYNC) {
    get_current_time();
    need_audio_resync = m_audio_sync->check_audio_sync(m_current_time, 
						       audio_resync_time,
						       wait_audio_time,
						       have_audio_eof,
						       need_audio_restart);
    if (need_audio_restart) {
      // this occurs when the time is in the past, but we're continuing
      // video
      return SYNC_STATE_WAIT_SYNC;
    }
    if (need_audio_resync == false) {
      // video continued, so, restart audio
      sync_message(LOG_ERR, "resync but no audio resync");
      return SYNC_STATE_PLAYING;
    }
    video_have_next_time = m_video_sync->play_video_at(m_current_time, 
						       true,
						       video_next_time,
						       have_video_eof);
    sync_message(LOG_DEBUG, "audio resync - "U64" have_next %d "U64,
		 m_current_time, video_have_next_time, 
		 video_next_time);
    sync_message(LOG_DEBUG, "audio time "U64, audio_resync_time);

    if (have_audio_eof && have_video_eof) {
      return SYNC_STATE_DONE;
    }

    if (video_have_next_time == false) {
      SDL_SemWaitTimeout(m_sync_sem, 10);
    } else {
      int64_t diff = video_next_time - audio_resync_time;
      if (diff >= TO_D64(-5000) && diff <= TO_D64(5000)) {
	return SYNC_STATE_WAIT_SYNC;
      }
      diff = video_next_time - m_current_time;
      if (diff < 0 || diff > TO_D64(1000)) {
	// drop this frame
	return SYNC_STATE_WAIT_SYNC;
#if 0
	m_video_sync->drop_next_frame();
	return state;
#endif
      }
      if (diff > 9) {
	diff = 9;
      }
      if (diff >= 9) {
	int delay = (int)diff;
	SDL_SemWaitTimeout(m_sync_sem, delay);
      }
    }
  }

  return state;
}
/*
 * sync_thread - call the correct handler, and wait for the state changes.
 * Each handler should only return the new state...
 */  
int CPlayerSession::sync_thread (int state)
{
  int newstate = 0;
  process_sdl_events();
  switch (state) {
  case SYNC_STATE_INIT:
    m_session_state = SESSION_BUFFERING;
    newstate = sync_thread_init();
    break;
  case SYNC_STATE_WAIT_SYNC:
    newstate = sync_thread_wait_sync();
    break;
  case SYNC_STATE_WAIT_AUDIO:
    newstate = sync_thread_wait_audio();
    break;
  case SYNC_STATE_PLAYING:
    newstate = sync_thread_playing();
    break;
  case SYNC_STATE_PAUSED:
    newstate = sync_thread_paused();
    break;
  case SYNC_STATE_AUDIO_RESYNC:
    newstate = sync_thread_audio_resync();
    break;
  case SYNC_STATE_DONE:
    newstate = sync_thread_done();
    break;
  }
#ifdef DEBUG_SYNC_STATE
  if (state != newstate)
    sync_message(LOG_INFO, "sync changed state %s to %s", 
		 sync_state[state], sync_state[newstate]);
#endif
  if (state != newstate) {
    state = newstate;
    switch (state) {
    case SYNC_STATE_WAIT_SYNC:
      m_session_state = SESSION_BUFFERING;
      break;
    case SYNC_STATE_WAIT_AUDIO:
    case SYNC_STATE_AUDIO_RESYNC:
      break;
    case SYNC_STATE_PLAYING:
      m_session_state = SESSION_PLAYING;
      break;
    case SYNC_STATE_PAUSED:
      if (m_video_sync != NULL) 
	m_video_sync->flush_sync_buffers();
      if (m_audio_sync != NULL) 
	m_audio_sync->flush_sync_buffers();
      m_session_state = SESSION_PAUSED;
      m_sync_pause_done = 1;
      break;
    case SYNC_STATE_DONE:
      if (m_video_sync && m_video_sync->get_fullscreen() != 0) {
	m_video_sync->set_fullscreen(0);
	m_video_sync->do_video_resize();
      }
      m_master_msg_queue->send_message(MSG_SESSION_FINISHED, 
				       NULL, 
				       0, 
				       m_master_msg_queue_sem);
      m_session_state = SESSION_DONE;
      break;
    case SYNC_STATE_EXIT:
      if (m_video_sync != NULL) 
	m_video_sync->flush_sync_buffers();
      if (m_audio_sync != NULL) 
	m_audio_sync->flush_sync_buffers();
      break;
    }
  }
  return (state);
}

int c_sync_thread (void *data)
{
  CPlayerSession *p;
  int state = SYNC_STATE_INIT;
  p = (CPlayerSession *)data;
  do {
   state = p->sync_thread(state);
  } while (state != SYNC_STATE_EXIT);
  return (0);
}

void CPlayerSession::display_status (void)
{
  sync_message(LOG_DEBUG, "*******************************************************************");
  sync_message(LOG_DEBUG, "current time "U64, m_current_time);
  CPlayerMedia *p = m_my_media;
  while (p != NULL) {
    p->display_status();
    p = p->get_next();
  }
}
/* end sync.cpp */
