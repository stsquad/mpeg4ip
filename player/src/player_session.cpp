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
#include "audio.h"
#include "video.h"

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
  m_content_base = NULL;
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
  m_fullscreen = 0;
  m_session_control_is_aggregate = 0;
  for (int ix = 0; ix < SESSION_DESC_COUNT; ix++) {
    m_session_desc[ix] = NULL;
  }
}

CPlayerSession::~CPlayerSession ()
{
#ifndef _WINDOWS
  if (m_sync_thread) {
    send_sync_thread_a_message(MSG_STOP_THREAD);
    SDL_WaitThread(m_sync_thread, NULL);
    m_sync_thread = NULL;
  }
#endif

  while (m_my_media != NULL) {
    CPlayerMedia *p;
    p = m_my_media;
    m_my_media = p->get_next();
    delete p;
  }

  if (session_control_is_aggregate()) {
    rtsp_command_t cmd;
    rtsp_decode_t *decode;
    memset(&cmd, 0, sizeof(rtsp_command_t));
    rtsp_send_aggregate_teardown(m_rtsp_client,
				 m_sdp_info->control_string,
				 &cmd,
				 &decode);
    free_decode_response(decode);
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
  if (m_content_base) {
    free((void *) m_content_base);
    m_content_base = NULL;
  }
  for (int ix = 0; ix < SESSION_DESC_COUNT; ix++) {
    if (m_session_desc[ix] != NULL) 
      free((void *)m_session_desc[ix]);
    m_session_desc[ix] = NULL;
  }
  SDL_Quit();
}

int CPlayerSession::create_streaming_broadcast (session_desc_t *sdp,
						const char **ermsg)
{
  session_set_seekable(0);
  m_sdp_info = sdp;
  m_range = get_range_from_sdp(m_sdp_info);
  m_streaming = 1;
  return (0);
}
/*
 * create_streaming - create a session for streaming.  Create an
 * RTSP session with the server, get the SDP information from it.
 */
int CPlayerSession::create_streaming_ondemand (const char *url, 
					       const char **errmsg,
					       int use_tcp)
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
  if (use_tcp != 0) {
#ifndef _WIN32
    m_rtsp_client = rtsp_create_client_for_rtp_tcp(url, &err);
#else
	m_rtsp_client = NULL;
#endif
  } else {
    m_rtsp_client = rtsp_create_client(url, &err);
  }
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
    int retval;
    if (decode != NULL) {
    retval = (((decode->retcode[0] - '0') * 100) +
	    ((decode->retcode[1] - '0') * 10) +
	    (decode->retcode[2] - '0'));
    } else {
      retval = -1;
    }
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
  if (m_sdp_info->control_string != NULL) {
    set_session_control(1);
  }
  /*
   * Make sure we can use the urls in the sdp info
   */
  m_content_base = strdup(decode->content_base);
  convert_relative_urls_to_absolute(m_sdp_info,
				    m_content_base);

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
      m_video_sync = new CVideoSync(this);
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
    player_debug_message("Restarting at " LLU ", %g", m_current_time, start_time);
  } else {
    /*
     * We might have been paused, but we're told to seek
     */
    // Indicate what time we're starting at for sync task.
    m_play_start_time = (uint64_t)(start_time * 1000.0);
  }
  m_paused = 0;

  send_sync_thread_a_message(MSG_START_SESSION);
  // If we're doing aggregate rtsp, send the play command...

  if (session_control_is_aggregate()) {
    char buffer[80];
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    memset(&cmd, 0, sizeof(rtsp_command_t));
    if (m_range != NULL) {
      sprintf(buffer, "npt=%g-%g", start_time, m_range->range_end);
      cmd.range = buffer;
    }
    if (rtsp_send_aggregate_play(m_rtsp_client,
				 m_sdp_info->control_string,
				 &cmd,
				 &decode) != 0) {
      player_debug_message("RTSP aggregate play command failed");
      free_decode_response(decode);
      return (-1);
    }
    int ret = process_rtsp_rtpinfo(decode->rtp_info, this, NULL);
    free_decode_response(decode);
    if (ret < 0) {
      player_debug_message("rtsp aggregate rtpinfo failed");
      return (-1);
    }
  }

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
  if (session_control_is_aggregate()) {
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    memset(&cmd, 0, sizeof(rtsp_command_t));
    if (rtsp_send_aggregate_pause(m_rtsp_client,
				  m_sdp_info->control_string,
				  &cmd,
				  &decode) != 0) {
      player_debug_message("RTSP aggregate pause command failed");
      free_decode_response(decode);
      return (-1);
    }
    free_decode_response(decode);
  }
  p = m_my_media;
  while (p != NULL) {
    ret = p->do_pause();
    if (ret != 0) return (ret);
    p = p->get_next();
  }
  m_sync_pause_done = 0;
  send_sync_thread_a_message(MSG_PAUSE_SESSION);
#ifndef _WINDOWS
  do {
#endif
    SDL_Delay(100);
#ifndef _WINDOWS
  } while (m_sync_pause_done == 0);
#endif
  m_paused = 1;
  return (0);
}
void CPlayerSession::add_media (CPlayerMedia *m) 
{
  CPlayerMedia *p;
  if (m_my_media == NULL) {
    m_my_media = m;
  } else {
    p = m_my_media;
    while (p->get_next() != NULL) {
      if (p == m) return;
      p = p->get_next();
    }
    p->set_next(m);
  }
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

void CPlayerSession::set_screen_size (int scaletimes2, int fullscreen)
{
  m_screen_scale = scaletimes2;
  m_fullscreen = fullscreen;
  if (m_video_sync) {
    m_video_sync->set_screen_size(scaletimes2);
    m_video_sync->set_fullscreen(fullscreen);
    send_sync_thread_a_message(MSG_SYNC_RESIZE_SCREEN);
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

/*
 * Matches a url with the corresponding media. 
 * Return the media, or NULL if no match. 
 */
CPlayerMedia *CPlayerSession::rtsp_url_to_media (const char *url)
{
  CPlayerMedia *p = m_my_media;
  while (p != NULL) {
	rtsp_session_t *session = p->get_rtsp_session();
	if (rtsp_is_url_my_stream(session, url, m_content_base, 
							  m_session_name) == 1) 
	  return p;
    p = p->get_next();
  }
  return (NULL);
}

int CPlayerSession::set_session_desc (int line, const char *desc)
{
  if (line >= SESSION_DESC_COUNT) {
    return -1;
  }
  if (m_session_desc[line] != NULL) free((void *)m_session_desc[line]);
  m_session_desc[line] = strdup(desc);
  if (m_session_desc[line] == NULL) 
    return -1;
  return (0);
}

const char *CPlayerSession::get_session_desc (int line)
{
  return m_session_desc[line];
}
/* end file player_session.cpp */
