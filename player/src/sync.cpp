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
#include "player_media.h"
#include <SDL.h>
#include <SDL_thread.h>
#include "player_util.h"
#include "audio.h"
#include "video.h"
//#define DEBUG_SYNC_STATE 1
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
  SYNC_STATE_DONE = 5,
  SYNC_STATE_EXIT = 6,
};

#ifdef DEBUG_SYNC_STATE
const char *sync_state[] = {
  "Init",
  "Wait Sync",
  "Wait Audio",
  "Playing",
  "Paused",
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
	if ((event.key.keysym.mod & (KMOD_LALT | KMOD_RALT)) != 0) {
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
	m_video_sync->do_video_resize();
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
	
  if (ret == 1) {
    return (SYNC_STATE_WAIT_SYNC); 
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
      uint64_t astart, vstart;

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
	  sync_message(LOG_DEBUG, "Astart is %llu", astart);
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
		     "Resynced at time "LLU " "LLU, m_current_time, vstart);
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
      sync_message(LOG_DEBUG, "Current time is %llu", m_current_time);
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
  int64_t video_status = 0;
  int have_audio_eof = 0, have_video_eof = 0;

  state = process_msg_queue(SYNC_STATE_PLAYING);
  if (state == SYNC_STATE_PLAYING) {
    get_current_time();
    if (m_audio_sync) {
      audio_resync_time = m_audio_sync->check_audio_sync(m_current_time, 
							 have_audio_eof);
    }
    if (m_video_sync) {
      video_status = m_video_sync->play_video_at(m_current_time, 
						 have_video_eof);
    }

    int delay = 9;
    int wait_for_signal = 0;

    if (m_video_sync && m_audio_sync) {
      if (have_video_eof && have_audio_eof) {
	return (SYNC_STATE_DONE);
      }
      if (video_status > 0 || audio_resync_time != 0) {
	if (audio_resync_time != 0) {
	  int64_t diff = audio_resync_time - m_current_time;
	  delay = (int)min(diff, video_status);
	} else {
	  if (video_status < 9) {
	    delay = 0;
	  }
	}
	if (delay < 9) {
	  wait_for_signal = 0;
	} else {
	  wait_for_signal = 1;
	}
      } else {
	wait_for_signal = 0;
      }
    } else if (m_video_sync) {
      if (have_video_eof == 1) {
	return (SYNC_STATE_DONE);
      } 
      if (video_status >= 9) {
	wait_for_signal = 1;
	delay = (int)video_status;
      } else {
	wait_for_signal = 0;
      }
    } else {
      // audio only
      if (have_audio_eof == 1) {
	return (SYNC_STATE_DONE);
      }
      if (audio_resync_time != 0) {
	if (audio_resync_time - m_current_time > 10) {
	  wait_for_signal = 1;
	  delay = (int)(audio_resync_time - m_current_time);
	} else {
	  wait_for_signal = 0;
	}
      } else {
	wait_for_signal = 1;
      }
    }
    //player_debug_message("w %d d %d", wait_for_signal, delay);
    if (wait_for_signal) {
      if (delay > 9) {
	delay = 9;
      }
      //player_debug_message("sw %d", delay);

      SDL_SemWaitTimeout(m_sync_sem, delay);
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

/* end sync.cpp */
