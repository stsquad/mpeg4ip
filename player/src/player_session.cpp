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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter @maersk-moller.net
 */
/* 
 * player_session.cpp - describes player session class, which is the
 * main access point for the player
 */
#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"
#include "audio.h"
#include "video.h"
#include "media_utils.h"
#include "our_config_file.h"

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(sync_message, "avsync")
#else
#define sync_message(loglevel, fmt...) message(loglevel, "avsync", fmt)
#endif

CPlayerSession::CPlayerSession (CMsgQueue *master_mq, 
				SDL_sem *master_sem,
				const char *name,
				control_callback_vft_t *cc_vft,
				void *video_persistence,
				double start_time)
{
  m_cc_vft = cc_vft;
  m_start_time_param = start_time;
  m_sdp_info = NULL;
  m_my_media = NULL;
  m_rtsp_client = NULL;
  m_timed_sync_list = NULL;
  m_video_list = NULL;
  m_audio_sync = NULL;
  m_sync_thread = NULL;
  m_sync_sem = NULL;
  m_content_base = NULL;
  m_master_msg_queue = master_mq;
  m_master_msg_queue_sem = master_sem;
  m_paused = false;
  m_streaming = false;
  m_session_name = strdup(name);
  m_audio_volume = 75;
  m_current_time = 0;
  m_seekable = 0;
  m_session_state = SESSION_PAUSED;
  m_screen_pos_x = 0;
  m_screen_pos_y = 0;
  m_hardware_error = 0;
  m_fullscreen = false;
  m_pixel_height = -1;
  m_pixel_width = -1;
  m_session_control_url = NULL;
  for (int ix = 0; ix < SESSION_DESC_COUNT; ix++) {
    m_session_desc[ix] = NULL;
  }
  m_media_close_callback = NULL;
  m_media_close_callback_data = NULL;
  m_streaming_media_set_up = 0;
  m_unused_ports = NULL;
  m_first_time_played = 0;
  m_latency = 0;
  m_have_audio_rtcp_sync = false;
  m_screen_scale = 2;
  m_set_end_time = 0;
  m_dont_send_first_rtsp_play = 0;
  // if we are passed a persistence, it is grabbed...
  m_video_persistence = video_persistence;
  m_grabbed_video_persistence = m_video_persistence != NULL;
  m_init_tries_made_with_media = 0;
  m_init_tries_made_with_no_media = 0;
  m_message[0] = '\0';
  m_stop_processing = SDL_CreateSemaphore(0);
  m_audio_count = m_video_count = m_text_count = 0;
  m_mouse_click_callback = NULL;
  m_audio_rtp_session = m_video_rtp_session = NULL;
}

CPlayerSession::~CPlayerSession ()
{
  int hadthread = 0;
  // indicate that we want to stop the start-up processing
  SDL_SemPost(m_stop_processing);
#ifndef NEED_SDL_VIDEO_IN_MAIN_THREAD
  if (m_sync_thread) {
    send_sync_thread_a_message(MSG_STOP_THREAD);
    SDL_WaitThread(m_sync_thread, NULL);
    m_sync_thread = NULL;
    hadthread = 1;
  }
#else
  send_sync_thread_a_message(MSG_STOP_THREAD);
  hadthread = 1;
#endif

  if (m_streaming_media_set_up != 0 &&
      session_control_is_aggregate()) {
    rtsp_command_t cmd;
    rtsp_decode_t *decode;
    memset(&cmd, 0, sizeof(rtsp_command_t));
    rtsp_send_aggregate_teardown(m_rtsp_client,
				 m_session_control_url,
				 &cmd,
				 &decode);
    free_decode_response(decode);
  }


  if (m_rtsp_client) {
    free_rtsp_client(m_rtsp_client);
    m_rtsp_client = NULL;
  }

  while (m_my_media != NULL) {
    CPlayerMedia *p;
    p = m_my_media;
    m_my_media = p->get_next();
    delete p;
  }  

  if (m_sdp_info) {
    sdp_free_session_desc(m_sdp_info);
    m_sdp_info = NULL;
  }

  if (m_stop_processing != NULL) {
    SDL_DestroySemaphore(m_stop_processing);
    m_stop_processing = NULL;
  }
  if (m_sync_sem) {
    SDL_DestroySemaphore(m_sync_sem);
    m_sync_sem = NULL;
  }

  int quit_sdl = 1;
  if (m_video_list != NULL) {
    // we need to know if we should quit SDL or not.  If the control
    // code has grabbed the persistence, we don't want to quit
    if (m_video_list->grabbed_video_persistence() != 0) {
      quit_sdl = 0;
    }
  }
  CTimedSync *ts;
  while (m_timed_sync_list != NULL) {
    ts = m_timed_sync_list->GetNext();
    delete m_timed_sync_list;
    m_timed_sync_list = ts;
  } 
  if (m_grabbed_video_persistence) {
    sync_message(LOG_DEBUG, "grabbed persist");
    quit_sdl = 0;
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
  
  if (m_media_close_callback != NULL) {
    m_media_close_callback(m_media_close_callback_data);
  }

  while (m_unused_ports != NULL) {
    CIpPort *first;
    first = m_unused_ports;
    m_unused_ports = first->get_next();
    delete first;
  }
  if (hadthread != 0 && quit_sdl != 0) {
    SDL_Quit();
  }
}

// start will basically start the sync thread, which will call
// start_session_work
bool CPlayerSession::start (bool use_thread)
{
  m_sync_sem = SDL_CreateSemaphore(0);
 
#ifdef NEED_SDL_VIDEO_IN_MAIN_THREAD
  use_thread = false;
#endif
  if (use_thread) {
    m_sync_thread = SDL_CreateThread(c_sync_thread, this);
  } else {
    return start_session_work();
  }
  return true;
}

// API from the sync task - this will start the session
// while this is running (except if NEED_SDL_VIDEO_IN_MAIN_THREAD),
// callees from parse_name_for_session can call ShouldStopProcessing
// to see if they should stop what they are doing.
bool CPlayerSession::start_session_work (void)
{
  int ret;
  bool err = false;
  ret = parse_name_for_session(this, 
			       get_session_name(), 
			       m_cc_vft);

  if (ret >= 0) {
    if (play_all_media(TRUE, m_start_time_param) < 0) {
      err = true;
    }
    if (err == false) {
      m_master_msg_queue->send_message(ret > 0 ? MSG_SESSION_WARNING 
				       : MSG_SESSION_STARTED,
				       m_master_msg_queue_sem);
    }
    m_init_time = get_time_of_day();
  } else {
    err = true;
  }
   
  if (err) {
    m_master_msg_queue->send_message(MSG_SESSION_ERROR, 
				     m_master_msg_queue_sem);
    return false;
  }
  return true;
}

int CPlayerSession::create_streaming_broadcast (session_desc_t *sdp)
{
  session_set_seekable(0);
  m_sdp_info = sdp;
  m_streaming = true;
  m_streaming_ondemand = 0;
  m_rtp_over_rtsp = 0;
  return (0);
}
/*
 * create_streaming - create a session for streaming.  Create an
 * RTSP session with the server, get the SDP information from it.
 */
int CPlayerSession::create_streaming_ondemand (const char *url,
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
    m_rtsp_client = rtsp_create_client_for_rtp_tcp(url, &err,
						   config.GetStringValue(CONFIG_RTSP_PROXY_ADDR),
						   config.GetIntegerValue(CONFIG_RTSP_PROXY_PORT));
  } else {
    m_rtsp_client = rtsp_create_client(url, &err,
				       config.GetStringValue(CONFIG_RTSP_PROXY_ADDR),
				       config.GetIntegerValue(CONFIG_RTSP_PROXY_PORT));
  }
  if (m_rtsp_client == NULL) {
    set_message("Failed to create RTSP client");
    player_error_message("Failed to create rtsp client - error %d", err);
    return (err);
  }
  m_rtp_over_rtsp = use_tcp;

  cmd.accept = "application/sdp";

  /*
   * Send the RTSP describe.  This should return SDP information about
   * the session.
   */
  int rtsp_resp;

  rtsp_resp = rtsp_send_describe(m_rtsp_client, &cmd, &decode);
  if (rtsp_resp != RTSP_RESPONSE_GOOD) {
    int retval;
    if (decode != NULL) {
      retval = (((decode->retcode[0] - '0') * 100) +
		((decode->retcode[1] - '0') * 10) +
		(decode->retcode[2] - '0'));
      set_message("RTSP describe error %d %s", retval,
		  decode->retresp != NULL ? decode->retresp : "");
      free_decode_response(decode);
    } else {
      retval = -1;
      set_message("RTSP return invalid %d", rtsp_resp);
    }
    player_error_message("Describe response not good\n");
    return (retval);
  }

  sdpdecode = set_sdp_decode_from_memory(decode->body);
  if (sdpdecode == NULL) {
    set_message("Memory failure");
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
    set_message("Couldn't decode session description %s",
	     decode->body);
    player_error_message("Couldn't decode sdp %s", decode->body);
    free_decode_response(decode);
    return (-1);
  }
  if (dummy != 1) {
    set_message("Incorrect number of sessions in sdp decode %d",
	     dummy);
    player_error_message("%s", get_message());
    free_decode_response(decode);
    return (-1);
  }

  /*
   * Make sure we can use the urls in the sdp info
   */
  if (decode->content_location != NULL) {
    // Note - we may have problems if the content location is not absolute.
    m_content_base = strdup(decode->content_location);
  } else if (decode->content_base != NULL) {
    m_content_base = strdup(decode->content_base);
  } else {
    int urllen = strlen(url);
    if (url[urllen] != '/') {
      char *temp;
      temp = (char *)malloc(urllen + 2);
      strcpy(temp, url);
      strcat(temp, "/");
      m_content_base = temp;
    } else {
      m_content_base = strdup(url);
    }
  }

  convert_relative_urls_to_absolute(m_sdp_info,
				    m_content_base);

  if (m_sdp_info->control_string != NULL) {
    player_debug_message("setting control url to %s", m_sdp_info->control_string);
    set_session_control_url(m_sdp_info->control_string);
  }
  free_decode_response(decode);
  m_streaming = true;
  m_streaming_ondemand = (get_range_from_sdp(m_sdp_info) != NULL);
  return (0);
}

int CPlayerSession::create_streaming_ondemand_other(rtsp_client_t *rtsp_client,
						    const char *control_url,
						    int have_end_time,
						    uint64_t end_time,
						    int dont_send_start_play,
						    int seekable)
{
  session_set_seekable(seekable);
  m_streaming = true;
  m_streaming_ondemand = 1;
  m_rtsp_client = rtsp_client;
  m_set_end_time = have_end_time;
  m_end_time = end_time;
  m_dont_send_first_rtsp_play = dont_send_start_play;
  m_session_control_url = control_url;
  streaming_media_set_up();
  return 0;
}

/*
 * play_all_media - get all media to play
 */
int CPlayerSession::play_all_media (int start_from_begin, 
				    double start_time)
{
  int ret;
  CPlayerMedia *p;

  if (m_set_end_time == 0) {
    range_desc_t *range;
    if (m_sdp_info && m_sdp_info->session_range.have_range != FALSE) {
      range = &m_sdp_info->session_range;
    } else {
      range = NULL;
      p = m_my_media;
      while (range == NULL && p != NULL) {
	media_desc_t *media;
	media = p->get_sdp_media_desc();
	if (media && media->media_range.have_range) {
	  range = &media->media_range;
	}
	p = p->get_next();
      }
    }
    if (range != NULL) {
      m_end_time = (uint64_t)(range->range_end * 1000.0);
      m_set_end_time = 1;
    }
  }
  p = m_my_media;
  m_session_state = SESSION_BUFFERING;
  if (m_paused && start_time == 0.0 && start_from_begin == FALSE) {
    /*
     * we were paused.  Continue.
     */
    m_play_start_time = m_current_time;
    start_time = UINT64_TO_DOUBLE(m_current_time);
    start_time /= 1000.0;
    player_debug_message("Restarting at " U64 ", %g", m_current_time, start_time);
  } else {
    /*
     * We might have been paused, but we're told to seek
     */
    // Indicate what time we're starting at for sync task.
    m_play_start_time = (uint64_t)(start_time * 1000.0);
  }
  m_paused = false;

  send_sync_thread_a_message(MSG_START_SESSION);
  // If we're doing aggregate rtsp, send the play command...

  if (session_control_is_aggregate() &&
      m_dont_send_first_rtsp_play == 0) {
    char buffer[80];
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    memset(&cmd, 0, sizeof(rtsp_command_t));
    if (m_set_end_time != 0) {
      uint64_t stime = (uint64_t)(start_time * 1000.0);
      sprintf(buffer, "npt="U64"."U64"-"U64"."U64, 
	      stime / 1000, stime % 1000, m_end_time / 1000, m_end_time % 1000);
      cmd.range = buffer;
    }
    if (rtsp_send_aggregate_play(m_rtsp_client,
				 m_session_control_url,
				 &cmd,
				 &decode) != 0) {
      set_message("RTSP Aggregate Play Error %s-%s", 
		  decode->retcode,
		  decode->retresp != NULL ? decode->retresp : "");
      player_debug_message("RTSP aggregate play command failed");
      free_decode_response(decode);
      return (-1);
    }
    if (decode->rtp_info == NULL) {
      player_error_message("No rtp info field");
    } else {
      player_debug_message("rtp info is \'%s\'", decode->rtp_info);
    }
    int ret = process_rtsp_rtpinfo(decode->rtp_info, this, NULL);
    free_decode_response(decode);
    if (ret < 0) {
      set_message("RTSP aggregate RtpInfo response failure");
      player_debug_message("rtsp aggregate rtpinfo failed");
      return (-1);
    }
  }
  m_dont_send_first_rtsp_play = 0;

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
				  m_session_control_url,
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
#ifndef NEED_SDL_VIDEO_IN_MAIN_THREAD
  do {
#endif
    SDL_Delay(100);
#ifndef NEED_SDL_VIDEO_IN_MAIN_THREAD
  } while (m_sync_pause_done == 0);
#endif
  m_paused = true;
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
  // set the sync values
  if (m->is_audio()) {
    // add audio sync
    m_audio_count++;
    m_audio_sync = m->get_audio_sync();
    if (m_audio_sync == NULL) {
      player_error_message("add audio media and sync is NULL");
    }
  } else {
    CTimedSync *ts;
    ts = m->get_timed_sync();
    if (m->get_sync_type() == VIDEO_SYNC) {
      m_video_count++;
      if (m_video_list == NULL) {
	m_video_list = m->get_video_sync();
      } else {
	CVideoSync *vs = m->get_video_sync();
	vs->SetNextVideo(m_video_list);
	m_video_list = vs;
      }
    } else {
      m_text_count++;
    }
    ts->SetNext(m_timed_sync_list);
    m_timed_sync_list = ts;
    if (ts == NULL) {
      player_error_message("add timed media and sync is NULL");
    }
  }
}

bool CPlayerSession::session_has_audio (void)
{
  return m_audio_count > 0;
}

bool CPlayerSession::session_has_video (void)
{
  return m_video_count > 0;
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

void CPlayerSession::set_screen_size (int scaletimes2, 
				      bool fullscreen, 
				      int pixel_width, 
				      int pixel_height,
				      int max_width, 
				      int max_height)
{
  m_screen_scale = scaletimes2;
  m_fullscreen = fullscreen;
  m_pixel_width = pixel_width;
  m_pixel_height = pixel_height;
  m_max_width = max_width;
  m_max_height = max_height;
  // Note - wmay - used to set the video sync directly here - now
  // we wait until we're initing or get resize message in sync thread.
  send_sync_thread_a_message(MSG_SYNC_RESIZE_SCREEN);
}

double CPlayerSession::get_max_time (void)
{
  CPlayerMedia *p;
  double max = 0.0;
  p = m_my_media;
  while (p != NULL) {
    double temp = p->get_max_playtime();
    if (temp > max) max = temp;
    p = p->get_next();
  }
  if (max == 0.0 && m_set_end_time) {
    max = (double)
#ifdef _WIN32
		(int64_t)
#endif
		m_end_time;
    max /= 1000.0;
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
/*
 * audio_is_ready - when the audio indicates that it's ready, it will
 * send a latency number, and a play time
 */
void CPlayerSession::audio_is_ready (uint64_t latency, uint64_t time)
{
  m_start = get_time_of_day();
  sync_message(LOG_DEBUG, "Aisready "U64, m_start);
  m_start -= time;
  m_latency = latency;
  sync_message(LOG_DEBUG, "Audio is ready "U64" - latency "U64, time, latency);
  sync_message(LOG_DEBUG, "m_start is "X64, m_start);
  m_waiting_for_audio = 0;
  SDL_SemPost(m_sync_sem);
}

void CPlayerSession::adjust_start_time (int64_t time)
{
  m_start -= time;
#if 0
  sync_message(LOG_INFO, "Adjusting start time "D64 " to " U64, time,
	       get_current_time());
#endif
  SDL_SemPost(m_sync_sem);
}

/*
 * get_current_time.  Gets the time of day, subtracts off the start time
 * to get the current play time.
 */
uint64_t CPlayerSession::get_current_time (void)
{
  uint64_t current_time;

  if (m_waiting_for_audio != 0) {
    return 0;
  }
  current_time = get_time_of_day();
  //if (current_time < m_start) return 0;
  m_current_time = current_time - m_start;
  if (m_current_time >= m_latency) m_current_time -= m_latency;
  return(m_current_time);
}

void CPlayerSession::synchronize_rtp_bytestreams (rtcp_sync_t *sync)
{
  if (sync != NULL) {
    m_audio_rtcp_sync = *sync;
    m_have_audio_rtcp_sync = true;
  } else {
    if (!m_have_audio_rtcp_sync) 
      return;
  }
  CPlayerMedia *mptr = m_my_media;
  while (mptr != NULL) {
    if (mptr->is_audio() == false) {
      mptr->synchronize_rtp_bytestreams(&m_audio_rtcp_sync);
    }
    mptr = mptr->get_next();
  }
}

void *CPlayerSession::grab_video_persistence (void)
{
  if (m_video_list == NULL) {
    m_grabbed_video_persistence = true;
    return m_video_persistence;
  }
  return m_video_list->grab_video_persistence();
}

void CPlayerSession::set_cursor (bool on)
{
  CVideoSync *vs = m_video_list;
  while (vs != NULL) {
    vs->set_cursor(on);
    vs = vs->GetNextVideo();
  }
}
/* end file player_session.cpp */
