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
 * player_session.cpp - describes player session class, which is the
 * main access point for the player
 */
#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"

/*
 * c callback for sync thread
 */
static int c_sync_thread (void *data)
{
  CPlayerSession *p;
  p = (CPlayerSession *)data;
  return (p->sync_thread());
}

CPlayerSession::CPlayerSession (CMsgQueue *master_mq, 
				SDL_sem *master_sem,
				const char *name)
{
  m_sdp_info = NULL;
  m_my_media = NULL;
  m_rtsp_client = NULL;
  m_video_sync = NULL;
  m_audio_sync = NULL;
  m_sync_thread = NULL;
  m_sync_sem = NULL;
  m_range = NULL;
  m_master_msg_queue = master_mq;
  m_master_msg_queue_sem = master_sem;
  m_paused = 0;
  m_streaming = 0;
  m_session_name = strdup(name);
  m_audio_volume = 75;
  m_current_time = 0;
  m_seekable = 0;
  m_session_state = SESSION_PAUSED;
  m_screen_pos_x = 0;
  m_screen_pos_y = 0;
  m_clock_wrapped = -1;
  m_hardware_error = 0;
}

CPlayerSession::~CPlayerSession ()
{
  if (m_sync_thread) {
    m_sync_thread_msg_queue.send_message(MSG_STOP_THREAD, NULL, 0, m_sync_sem);
    SDL_WaitThread(m_sync_thread, NULL);
    m_sync_thread = NULL;
  }

  while (m_my_media != NULL) {
    CPlayerMedia *p;
    p = m_my_media;
    m_my_media = p->get_next();
    delete p;
  }

  if (m_rtsp_client) {
    free_rtsp_client(m_rtsp_client);
    m_rtsp_client = NULL;
  }
  
  if (m_sdp_info) {
    free_session_desc(m_sdp_info);
    m_sdp_info = NULL;
  }

  if (m_sync_sem) {
    SDL_DestroySemaphore(m_sync_sem);
    m_sync_sem = NULL;
  }
  
  if (m_video_sync != NULL) {
    delete m_video_sync;
    m_video_sync = NULL;
  }

  if (m_audio_sync != NULL) {
    delete m_audio_sync;
    m_audio_sync = NULL;
  }
  if (m_session_name) {
    free((void *)m_session_name);
    m_session_name = NULL;
  }
  SDL_Quit();
}

/*
 * create_streaming - create a session for streaming.  Create an
 * RTSP session with the server, get the SDP information from it.
 */
int CPlayerSession::create_streaming (const char *url, const char **errmsg)
{
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  sdp_decode_info_t *sdpdecode;
  int dummy;
  int err;

  // streaming has seek capability (at least on demand)
  session_set_seekable(1);
  player_debug_message("Creating streaming %s", url);
  memset(&cmd, 0, sizeof(rtsp_command_t));

  /*
   * create RTSP session
   */
  m_rtsp_client = rtsp_create_client(url, &err);
  if (m_rtsp_client == NULL) {
    *errmsg = "Failed to create RTSP client";
    player_error_message("Failed to create rtsp client - error %d", err);
    return (err);
  }

  cmd.accept = "application/sdp";

  /*
   * Send the RTSP describe.  This should return SDP information about
   * the session.
   */
  if (rtsp_send_describe(m_rtsp_client, &cmd, &decode) !=
      RTSP_RESPONSE_GOOD) {
    int retval = (((decode->retcode[0] - '0') * 100) +
	    ((decode->retcode[1] - '0') * 10) +
	    (decode->retcode[2] - '0'));
    *errmsg = "RTSP describe error";
    player_error_message("Describe response not good\n");
    free_decode_response(decode);
    return (retval);
  }

  sdpdecode = set_sdp_decode_from_memory(decode->body);
  if (sdpdecode == NULL) {
    *errmsg = "Memory failure";
    player_error_message("Couldn't get sdp decode\n");
    free_decode_response(decode);
    return (-1);
  }

  /*
   * Decode the SDP information into structures we can use.
   */
  err = sdp_decode(sdpdecode, &m_sdp_info, &dummy);
  free(sdpdecode);
  if (err != 0) {
    *errmsg = "Couldn't decode session description";
    player_error_message("Couldn't decode sdp %s", decode->body);
    free_decode_response(decode);
    return (-1);
  }
  if (dummy != 1) {
    *errmsg = "Session description error";
    player_error_message("Incorrect number of sessions in sdp decode %d",
			 dummy);
    free_decode_response(decode);
    return (-1);
  }

  m_range = get_range_from_sdp(m_sdp_info);

  /*
   * Make sure we can use the urls in the sdp info
   */
  convert_relative_urls_to_absolute(m_sdp_info,
				    decode->content_base);

  free_decode_response(decode);
  m_streaming = 1;
  return (0);
}

/*
 * set_up_sync_thread.  Creates the sync thread, and a sync class
 * for each media
 */
void CPlayerSession::set_up_sync_thread(void) 
{
  CPlayerMedia *media;

  media = m_my_media;
  while (media != NULL) {
    if (media->is_video()) {
      m_video_sync = new CVideoSync();
      media->set_video_sync(m_video_sync);
    } else {
      m_audio_sync = new CAudioSync(this, m_audio_volume);
      media->set_audio_sync(m_audio_sync);
    }
    media= media->get_next();
  }
  m_sync_sem = SDL_CreateSemaphore(0);
#ifndef _WINDOWS
  m_sync_thread = SDL_CreateThread(c_sync_thread, this);
#endif
}

/*
 * play_all_media - get all media to play
 */
int CPlayerSession::play_all_media (int start_from_begin, double start_time)
{
  int ret;
  CPlayerMedia *p;

  p = m_my_media;
  m_session_state = SESSION_BUFFERING;
  if (m_paused == 1 && start_time == 0.0 && start_from_begin == FALSE) {
    /*
     * we were paused.  Continue.
     */
    m_play_start_time = m_current_time;
#ifdef _WINDOWS
	start_time = (double)(int64_t)m_current_time;
#else
    start_time = (double) m_current_time;
#endif
    start_time /= 1000.0;
    player_debug_message("Restarting at %llu, %g", m_current_time, start_time);
  } else {
    /*
     * We might have been paused, but we're told to seek
     */
    // Indicate what time we're starting at for sync task.
    m_play_start_time = (uint64_t)(start_time * 1000.0);
  }
  m_paused = 0;

  m_sync_thread_msg_queue.send_message(MSG_START_SESSION, NULL, 0, m_sync_sem);
  while (p != NULL) {
    ret = p->do_play(start_time);
    if (ret != 0) return (ret);
    p = p->get_next();
  }
  return (0);
}

/*
 * pause_all_media - do a spin loop until the sync thread indicates it's
 * paused.
 */
int CPlayerSession::pause_all_media (void) 
{
  int ret;
  CPlayerMedia *p;
  m_session_state = SESSION_PAUSED;
  p = m_my_media;
  while (p != NULL) {
    ret = p->do_pause();
    if (ret != 0) return (ret);
    p = p->get_next();
  }
  m_sync_pause_done = 0;
  m_sync_thread_msg_queue.send_message(MSG_PAUSE_SESSION, NULL, 0, m_sync_sem);
  do {
    SDL_Delay(100);
  } while (m_sync_pause_done == 0);
  m_paused = 1;
  return (0);
}

int CPlayerSession::session_has_audio (void)
{
  CPlayerMedia *p;
  p = m_my_media;
  while (p != NULL) {
    if (p->is_video() == FALSE) {
      return (1);
    }
    p = p->get_next();
  }
  return (0);
}

int CPlayerSession::session_has_video (void)
{
  CPlayerMedia *p;
  p = m_my_media;
  while (p != NULL) {
    if (p->is_video() != FALSE) {
      return (1);
    }
    p = p->get_next();
  }
  return (0);
}

void CPlayerSession::set_audio_volume (int volume)
{
  m_audio_volume = volume;
  if (m_audio_sync) {
    m_audio_sync->set_volume(m_audio_volume);
  }
}

void CPlayerSession::set_screen_location (int x, int y)
{
  m_screen_pos_x = x;
  m_screen_pos_y = y;
}

void CPlayerSession::set_screen_size (int scaletimes2)
{
  m_screen_scale = scaletimes2;
  if (m_video_sync) {
    m_video_sync->set_screen_size(scaletimes2);
    m_sync_thread_msg_queue.send_message(MSG_SYNC_RESIZE_SCREEN, 
					 NULL, 
					 0, 
					 m_sync_sem);
  }
}
double CPlayerSession::get_max_time (void)
{
  if (m_range != NULL) {
    return m_range->range_end;
  }
  CPlayerMedia *p;
  double max = 0.0;
  p = m_my_media;
  while (p != NULL) {
    double temp = p->get_max_playtime();
    if (temp > max) max = temp;
    p = p->get_next();
  }
  return (max);
}

/* end file player_session.cpp */
