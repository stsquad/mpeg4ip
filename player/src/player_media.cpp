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
 */
/*
 * player_media.cpp - handle generic information about a stream
 */
#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"
#include <rtp/memory.h>
#include "rtp_bytestream.h"
#include "our_config_file.h"
#include "media_utils.h"
#include "ip_port.h"
#include "codec_plugin.h"
#include "audio.h"
#include "text.h"
#include <time.h>
#include "player_rtsp.h"
#include "codec_plugin_private.h"
//#define DROP_PAKS 1
/*
 * c routines for callbacks
 */
static int c_recv_thread (void *data)
{
  CPlayerMedia *media;

  media = (CPlayerMedia *)data;
  return (media->recv_thread());
}


static int c_decode_thread (void *data)
{
  CPlayerMedia *media;
  media = (CPlayerMedia *)data;
  return (media->decode_thread());
}

static void c_rtp_packet_callback (void *data, 
				   unsigned char interleaved, 
				   struct rtp_packet *pak)
{
  ((CPlayerMedia *)data)->rtp_receive_packet(interleaved, pak);
}

static int c_init_rtp_tcp (void *data)
{
  ((CPlayerMedia *)data)->rtp_init(true);
  return 0;
}

static int c_rtp_start (void *data)
{
  ((CPlayerMedia *)data)->rtp_start();
  return 0;
}
static int c_rtp_periodic (void *data)
{
  ((CPlayerMedia *)data)->rtp_periodic();
  return 0;
}


CPlayerMedia::CPlayerMedia (CPlayerSession *p, 
			    session_sync_types_t sync_type)
{
  m_plugin = NULL;
  m_plugin_data = NULL;
  m_next = NULL;
  m_parent = p;
  m_media_info = NULL;
  m_media_fmt = NULL;
  m_our_port = 0;
  m_ports = NULL;
  m_server_port = 0;
  m_source_addr = NULL;
  m_recv_thread = NULL;
  m_rtptime_tickpersec = 0;
  m_rtsp_base_seq_received = 0;
  m_rtsp_base_ts_received = 0;

  m_head = NULL;
  m_rtp_queue_len = 0;

  m_rtp_ssrc_set = FALSE;

  m_rtp_session = NULL;
  m_srtp_session = NULL;
  m_rtsp_session = NULL;
  m_decode_thread_waiting = 0;
  m_sync_time_set = FALSE;
  m_decode_thread = NULL;
  m_decode_thread_sem = NULL;
  set_timed_sync(NULL);
  set_audio_sync(NULL);
  m_paused = false;
  m_byte_stream = NULL;
  m_rtp_byte_stream = NULL;
  m_video_info = NULL;
  m_audio_info = NULL;
  m_user_data = NULL;
  m_rtcp_received = 0;
  m_streaming = false;
  m_stream_ondemand = 0;
  m_rtp_use_rtsp = false;
  m_media_type = NULL;
  m_is_audio = sync_type == AUDIO_SYNC;
  m_sync_type = sync_type;
  switch (sync_type) {
  case VIDEO_SYNC:
    set_video_sync(create_video_sync(m_parent));
    break;
  case AUDIO_SYNC:
    set_audio_sync(create_audio_sync(m_parent));
    break;
  case TIMED_TEXT_SYNC:
    set_timed_sync(create_text_sync(m_parent));
    break;
  }
  m_play_start_time = 0.0;
}

CPlayerMedia::~CPlayerMedia()
{
  rtsp_decode_t *rtsp_decode;

  media_message(LOG_DEBUG, "closing down media %d", m_is_audio);
  if (m_rtsp_session) {
    // If control is aggregate, m_rtsp_session will be freed by
    // CPlayerSession
    if (m_parent->session_control_is_aggregate() == 0) {
      rtsp_send_teardown(m_rtsp_session, NULL, &rtsp_decode);
      free_decode_response(rtsp_decode);
    }
    m_rtsp_session = NULL;
  }
  
  if (m_recv_thread) {
    m_rtp_msg_queue.send_message(MSG_STOP_THREAD);
    SDL_WaitThread(m_recv_thread, NULL);
    m_recv_thread = NULL;
  }

  if (m_decode_thread) {
    m_decode_msg_queue.send_message(MSG_STOP_THREAD, 
				    NULL, 
				    0, 
				    m_decode_thread_sem);
    SDL_WaitThread(m_decode_thread, NULL);
    m_decode_thread = NULL;
  }


    
  if (m_source_addr != NULL) free(m_source_addr);
  m_next = NULL;
  m_parent = NULL;

  if (m_ports) {
    delete m_ports;
    m_ports = NULL;
  }
  if (m_rtp_byte_stream) {
    double diff;
    diff = difftime(time(NULL), m_start_time);
    media_message(LOG_INFO, "Media %s", m_media_info->media);
    
    media_message(LOG_INFO, "Time: %g seconds", diff);
#if 0
    double div;
    player_debug_message("Packets received: %u", m_rtp_packet_received);
    player_debug_message("Payload received: "U64" bytes", m_rtp_data_received);
    div = m_rtp_packet_received / diff;
    player_debug_message("Packets per sec : %g", div);
    div = UINT64_TO_DOUBLE(m_rtp_data_received);
	div *= 8.0;
	div /= diff;
    media_message(LOG_INFO, "Bits per sec   : %g", div);
#endif
			 
  }
  if (m_byte_stream) {
    delete m_byte_stream;
    m_byte_stream = NULL;
    m_rtp_byte_stream = NULL;
  }
  if (m_video_info) {
    free(m_video_info);
    m_video_info = NULL;
  }
  if (m_audio_info) {
    free(m_audio_info);
    m_audio_info = NULL;
  }

  if (m_user_data) {
    free((void *)m_user_data);
    m_user_data = NULL;
  }
  if (m_decode_thread_sem) {
    SDL_DestroySemaphore(m_decode_thread_sem);
    m_decode_thread_sem = NULL;
  }
  CHECK_AND_FREE(m_media_type);
}

void CPlayerMedia::clear_rtp_packets (void)
{
  if (m_head != NULL) {
    m_tail->rtp_next = NULL;
    while (m_head != NULL) {
      rtp_packet *p;
      p = m_head;
      m_head = m_head->rtp_next;
      p->rtp_next = p->rtp_prev = NULL;
      xfree(p);
    }
  }
  m_tail = NULL;
  m_rtp_queue_len = 0;
}

int CPlayerMedia::create_common (const char *media_type)
{
  m_media_type = strdup(media_type);
  m_parent->add_media(this);

  m_decode_thread_sem = SDL_CreateSemaphore(0);
  m_decode_thread = SDL_CreateThread(c_decode_thread, this);
  if (m_decode_thread_sem == NULL || m_decode_thread == NULL) {
    const char *outmedia;
    if (m_media_info == NULL) {
      outmedia = m_is_audio ? "audio" : "video";
    } else outmedia = m_media_info->media;

    m_parent->set_message("Couldn't start media thread for %s", 
			  outmedia);
    media_message(LOG_ERR, "Failed to create decode thread for media %s",
		  outmedia);
    return (-1);
  }
  return 0;
}
/*
 * CPlayerMedia::create - create when we've already got a
 * bytestream
 */
int CPlayerMedia::create_media (const char *media_type, 
				COurInByteStream *b, 
				bool streaming)
{
  m_byte_stream = b;
  m_streaming = streaming;
  return create_common(media_type);
}

/*
 * CPlayerMedia::create_streaming - create a streaming media session,
 * including setting up rtsp session, rtp and rtp bytestream
 */
int CPlayerMedia::create_streaming (media_desc_t *sdp_media,
				    int ondemand,
				    int use_rtsp,
				    int media_number_in_session,
				    struct rtp *rtp_session)
{
  char buffer[80];
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  
  m_streaming = true;
  if (sdp_media == NULL) {
    m_parent->set_message("Internal media error - sdp is NULL");
    return(-1);
  }

  if (strncasecmp(sdp_media->proto, "RTP", strlen("RTP")) != 0) {
    m_parent->set_message("Media %s doesn't use RTP", sdp_media->media);
    media_message(LOG_ERR, "%s doesn't use RTP", sdp_media->media);
    return (-1);
  }
#if 0
  if (strncasecmp(sdp_media->proto, "RTP/SVP", strlen("RTP/SVP")) == 0) {
    m_parent->set_message("Media %s uses SRTP and it is not installed", 
			  sdp_media->media);
    media_message(LOG_ERR, "SRTP required for media %s but not installed", 
		  sdp_media->media);
#endif
  if (sdp_media->fmt_list == NULL) {
    m_parent->set_message("Media %s doesn't have any usuable formats",
			  sdp_media->media);
    media_message(LOG_ERR, "%s doesn't have any formats", 
		  sdp_media->media);
    return (-1);
  }

  m_media_info = sdp_media;
  m_stream_ondemand = ondemand;
  m_rtp_session = rtp_session;

  if (ondemand != 0) {
    /*
     * Get 2 consecutive IP ports.  If we don't have this, don't even
     * bother
     */
    if (use_rtsp == 0) {
      if (m_rtp_session != NULL) {
	m_our_port = rtp_get_rx_port(m_rtp_session);
      } else {
	m_ports = new C2ConsecIpPort(m_parent->get_unused_ip_port_ptr());
	if (m_ports == NULL || !m_ports->valid()) {
	  m_parent->set_message("Could not find any valid IP ports");
	  media_message(LOG_ERR, "Couldn't get valid IP ports");
	  return (-1);
	}
	m_our_port = m_ports->first_port();
      }

      /*
       * Send RTSP setup message - first create the transport string for that
       * message
       */
      create_rtsp_transport_from_sdp(m_parent->get_sdp_info(),
				     m_media_info,
				     m_our_port,
				     buffer,
				     sizeof(buffer));
    } else {
      m_rtp_use_rtsp = true;
      m_rtp_media_number_in_session = media_number_in_session;
      snprintf(buffer, sizeof(buffer), "RTP/AVP/TCP;unicast;interleaved=%d-%d",
	       media_number_in_session * 2, (media_number_in_session * 2) + 1);
    }
    memset(&cmd, 0, sizeof(rtsp_command_t));
    cmd.transport = buffer;
    int err = 
      rtsp_send_setup(m_parent->get_rtsp_client(),
		      m_media_info->control_string,
		      &cmd,
		      &m_rtsp_session,
		      &decode,
		      m_parent->session_control_is_aggregate());
    if (err != 0) {
      m_parent->set_message("Couldn't set up session %s", 
			    m_media_info->control_string);
      media_message(LOG_ERR, "Can't create session %s - error code %d", 
		    m_media_info->media, err);
      if (decode != NULL)
	free_decode_response(decode);
      return (-1);
    }
    cmd.transport = NULL;
    media_message(LOG_INFO, "Transport returned is %s", decode->transport);

    /*
     * process the transport they sent.  They need to send port numbers, 
     * addresses, rtptime information, that sort of thing
     */
    if (m_source_addr == NULL) {
      m_source_addr = rtsp_get_server_ip_address_string(m_rtsp_session);
      media_message(LOG_INFO, "setting default source address from rtsp %s", m_source_addr);
    }

    if (process_rtsp_transport(decode->transport) != 0) {
      m_parent->set_message("Couldn't process transport information in RTSP response: %s", 
			    decode->transport);
      free_decode_response(decode);
      return(-1);
    }
    free_decode_response(decode);
  } else {
    m_server_port = m_our_port = m_media_info->port;
  }
  connect_desc_t *cptr;
  cptr = get_connect_desc_from_media(m_media_info);
  if (cptr == NULL) {
    m_parent->set_message("Server did not return address");
    return (-1);
  }

  //
  // okay - here we want to check that the server port is set up, and
  // go ahead and init rtp, and the associated task
  //
  m_start_time = time(NULL);

  if (create_common(sdp_media->media) < 0) {
    return -1;
  }

  if (ondemand == 0 || use_rtsp == 0) {
    m_rtp_inited = 0;
    m_recv_thread = SDL_CreateThread(c_recv_thread, this);
    if (m_recv_thread == NULL) {
      m_parent->set_message("Couldn't create media %s RTP recv thread",
	       m_media_info->media);
      media_message(LOG_ERR, "%s", m_parent->get_message());
      return (-1);
    }
    while (m_rtp_inited == 0) {
      SDL_Delay(10);
    }
    if (m_rtp_session == NULL) {
      m_parent->set_message("Could not start RTP - check debug log");
      media_message(LOG_ERR, "%s", m_parent->get_message());
      return (-1);
    }
  } else {
    int ret;
    ret = rtsp_thread_set_process_rtp_callback(m_parent->get_rtsp_client(),
					       c_rtp_packet_callback,
					       c_rtp_periodic,
					       m_rtp_media_number_in_session,
					       this);
    if (ret < 0) {
      m_parent->set_message("Can't setup TCP/RTP callback");
      return -1;
    }
    ret = rtsp_thread_perform_callback(m_parent->get_rtsp_client(),
				       c_init_rtp_tcp,
				       this);
    if (ret < 0) {
      m_parent->set_message("Can't init RTP in RTSP thread");
      return -1;
    }
  }
  if (m_rtp_session == NULL) {
    m_parent->set_message("Couldn't create RTP session for media %s",
	     m_media_info->media);
    media_message(LOG_ERR, "%s", m_parent->get_message());
    return (-1);
  }
  return (0);
}

int CPlayerMedia::create_video_plugin (const codec_plugin_t *p,
				       const char *stream_type,
				       const char *compressor, 
				       int type, 
				       int profile, 
				       format_list_t *sdp_media,
				       video_info_t *video,
				       const uint8_t *user_data,
				       uint32_t userdata_size)
{
  if (p == NULL) {
    p = check_for_video_codec(stream_type, 
			      compressor, 
			      sdp_media, 
			      type, 
			      profile, 
			      user_data, 
			      userdata_size, 
			      &config);
    if (p == NULL) return -1;
  } 

  m_plugin = p;
  m_video_info = video;
  m_plugin_data = (p->vc_create)(stream_type,
				 compressor, 
				 type,
				 profile, sdp_media,
				 video,
				 user_data,
				 userdata_size,
				 get_video_vft(),
				 m_videoSync);
  if (m_plugin_data == NULL) 
    return -1;

  if (user_data != NULL) 
    set_user_data(user_data, userdata_size);
  return 0;
}

void CPlayerMedia::set_plugin_data (const codec_plugin_t *p, 
				    codec_data_t *d, 
				    video_vft_t *v, 
				    audio_vft_t *a)
{
  m_plugin = p;
  m_plugin_data = d;
  d->ifptr = m_sync;
  if (is_audio() == false) {
    //d->ifptr = m_videoSync;
    d->v.video_vft = v;
  } else {
    //d->ifptr = m_audioSync;
    d->v.audio_vft = a;
  }
    
}

int CPlayerMedia::get_plugin_status (char *buffer, uint32_t buflen)
{
  if (m_plugin == NULL) return -1;

  if (m_plugin->c_print_status == NULL) return -1;

  return ((m_plugin->c_print_status)(m_plugin_data, buffer, buflen));
}

int CPlayerMedia::create_audio_plugin (const codec_plugin_t *p,
				       const char *stream_type,
				       const char *compressor, 
				       int type, 
				       int profile,
				       format_list_t *sdp_media,
				       audio_info_t *audio,
				       const uint8_t *user_data,
				       uint32_t userdata_size)
{
  if (p == NULL) {
    p = check_for_audio_codec(stream_type, compressor, sdp_media, 
			      type, profile, user_data, userdata_size, 
			      &config);
    if (p == NULL) return -1;
  }
  m_audio_info = audio;
  m_plugin = p;
  m_plugin_data = (p->ac_create)(stream_type,
				 compressor,
				 type, 
				 profile, 
				 sdp_media,
				 audio,
				 user_data,
				 userdata_size,
				 get_audio_vft(),
				 m_sync);
  if (m_plugin_data == NULL) return -1;

  if (user_data != NULL)
    set_user_data(user_data, userdata_size);
  return 0;
}

int CPlayerMedia::create_text_plugin (const codec_plugin_t *p,
				      const char *stream_type,
				      const char *compressor, 
				      format_list_t *sdp_media,
				      const uint8_t *user_data,
				      uint32_t userdata_size)
{
  if (p == NULL) {
    p = check_for_text_codec(stream_type, compressor, sdp_media, 
			     user_data, userdata_size, 
			     &config);
    if (p == NULL) return -1;
  }
  m_plugin = p;
  m_plugin_data = (p->tc_create)(stream_type,
				 compressor,
				 sdp_media,
				 user_data,
				 userdata_size,
				 get_text_vft(),
				 m_sync);
  if (m_plugin_data == NULL) return -1;

  if (user_data != NULL)
    set_user_data(user_data, userdata_size);
  return 0;
}

/*
 * CPlayerMedia::do_play - get play command
 */
int CPlayerMedia::do_play (double start_time_offset)
{

  if (m_streaming) {
    m_paused = false;
    if (m_stream_ondemand != 0) {
      /*
       * We're streaming - send the RTSP play command
       */
      if (m_parent->session_control_is_aggregate() == 0) {
	char buffer[80];
	rtsp_command_t cmd;
	rtsp_decode_t *decode;
	range_desc_t *range;
	memset(&cmd, 0, sizeof(rtsp_command_t));

	// only do range if we're not paused
	range = get_range_from_media(m_media_info);
	if (range != NULL) {
	  if (start_time_offset < range->range_start || 
	      start_time_offset > range->range_end) 
	    start_time_offset = range->range_start;
	  // need to check for smpte
	  sprintf(buffer, "npt=%g-%g", start_time_offset, range->range_end);
	  cmd.range = buffer;
	}

	if (rtsp_send_play(m_rtsp_session, &cmd, &decode) != 0) {
	  media_message(LOG_ERR, "RTSP play command failed");
	  m_parent->set_message("RTSP Play Error %s-%s", 
				decode->retcode,
				decode->retresp != NULL ? 
				decode->retresp : "");
	  free_decode_response(decode);
	  return (-1);
	}

	/*
	 * process the return information
	 */
	int ret = process_rtsp_rtpinfo(decode->rtp_info, m_parent, this);
	if (ret < 0) {
	  media_message(LOG_ERR, "rtsp rtpinfo failed");
	  free_decode_response(decode);
	  m_parent->set_message("RTSP aggregate RtpInfo response failure");
	  return (-1);
	}
	free_decode_response(decode);
      }
      if (m_source_addr == NULL) {
	// get the ip address of the server from the rtsp stack
	m_source_addr = rtsp_get_server_ip_address_string(m_rtsp_session);
	media_message(LOG_INFO, "Setting source address from rtsp - %s", 
		      m_source_addr);
      }
      // ASDF - probably need to do some stuff here for no rtpinfo...
      /*
       * set the various play times, and send a message to the recv task
       * that it needs to start
       */
      m_play_start_time = start_time_offset;
    }
    if (m_byte_stream != NULL) {
      m_byte_stream->play((uint64_t)(start_time_offset * 1000.0));
    }
    if (m_rtp_use_rtsp) {
      rtsp_thread_perform_callback(m_parent->get_rtsp_client(),
				   c_rtp_start, 
				   this);
    } 
  } else {
    /*
     * File (or other) playback.
     */
    if (m_paused == false || start_time_offset == 0.0) {
      m_byte_stream->reset();
    }
    m_byte_stream->play((uint64_t)(start_time_offset * 1000.0));
    m_play_start_time = start_time_offset;
    m_paused = false;
    start_decoding();
  }
  return (0);
}

/*
 * CPlayerMedia::do_pause - stop what we're doing
 */
int CPlayerMedia::do_pause (void)
{

  if (m_streaming) {
    if (m_stream_ondemand != 0) {
      /*
     * streaming - send RTSP pause
     */
      if (m_parent->session_control_is_aggregate() == 0) {
	rtsp_command_t cmd;
	rtsp_decode_t *decode;
	memset(&cmd, 0, sizeof(rtsp_command_t));

	if (rtsp_send_pause(m_rtsp_session, &cmd, &decode) != 0) {
	  media_message(LOG_ERR, "RTSP play command failed");
	  free_decode_response(decode);
	  return (-1);
	}
	free_decode_response(decode);
      }
    }
    if (m_recv_thread != NULL) {
      m_rtp_msg_queue.send_message(MSG_PAUSE_SESSION);
    }
  }

  if (m_byte_stream != NULL) 
    m_byte_stream->pause();
  /*
   * Pause the various threads
   */
  m_decode_msg_queue.send_message(MSG_PAUSE_SESSION, 
				  m_decode_thread_sem);
  m_paused = true;
  return (0);
}

double CPlayerMedia::get_max_playtime (void) 
{
  if (m_byte_stream) {
    return (m_byte_stream->get_max_playtime());
  }
  return (0.0);
}

/***************************************************************************
 * Transport and RTP-Info RTSP header line parsing.
 ***************************************************************************/
#define TTYPE(a,b) {a, sizeof(a), b}

static char *rtpinfo_parse_ssrc (char *transport, CPlayerMedia *m, int &end)
{
  uint32_t ssrc;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_hex(transport, &ssrc);
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport == ',') {
      end = 1;
    } else if (*transport != ';') {
      return (NULL);
    }
    transport++;
  }
  m->set_rtp_ssrc(ssrc);
  return (transport);
}

static char *rtpinfo_parse_seq (char *rtpinfo, CPlayerMedia *m, int &endofurl)
{
  uint32_t seq;
  if (*rtpinfo != '=') {
    return (NULL);
  }
  rtpinfo++;
  ADV_SPACE(rtpinfo);
  rtpinfo = convert_number(rtpinfo, &seq);
  ADV_SPACE(rtpinfo);
  if (*rtpinfo != '\0') {
    if (*rtpinfo == ',') {
      endofurl = 1;
    } else if (*rtpinfo != ';') {
      return (NULL);
    }
    rtpinfo++;
  }
  m->set_rtp_base_seq(seq);
  return (rtpinfo);
}

static char *rtpinfo_parse_rtptime (char *rtpinfo, 
				    CPlayerMedia *m, 
				    int &endofurl)
{
  uint32_t rtptime;
  int neg = 0;
  if (*rtpinfo != '=') {
    return (NULL);
  }
  rtpinfo++;
  ADV_SPACE(rtpinfo);
  if (*rtpinfo == '-') {
    neg = 1;
    rtpinfo++;
    ADV_SPACE(rtpinfo);
  }
  rtpinfo = convert_number(rtpinfo, &rtptime);
  ADV_SPACE(rtpinfo);
  if (*rtpinfo != '\0') {
    if (*rtpinfo == ',') {
      endofurl = 1;
    } else if (*rtpinfo != ';') {
      return (NULL);
    }
    rtpinfo++;
  }
  if (neg != 0) {
    player_error_message("Warning - negative time returned in rtpinfo");
    rtptime = 0 - rtptime;
  }
  m->set_rtp_base_ts(rtptime);
  return (rtpinfo);
}

int CPlayerMedia::process_rtsp_transport (char *transport)
{
  rtsp_transport_parse_t parse;
  memset(&parse, 0, sizeof(parse));
  parse.client_port = get_our_port();
  parse.interleave_port = get_rtp_media_number() * 2;
  parse.use_interleaved = m_rtp_use_rtsp;

  int ret = ::process_rtsp_transport(&parse, transport, m_media_info->proto);

  if (ret >= 0) {
    set_server_port(parse.server_port);
    if (parse.have_ssrc != 0) {
      set_rtp_ssrc(parse.ssrc);
    }
  }
  return ret;
}

static struct {
  const char *name;
  uint32_t namelen;
  char *(*routine)(char *transport, CPlayerMedia *, int &end_for_url);
} rtpinfo_types[] = 
{
  TTYPE("seq", rtpinfo_parse_seq),
  TTYPE("rtptime", rtpinfo_parse_rtptime),
  TTYPE("ssrc", rtpinfo_parse_ssrc),
  {NULL, 0, NULL},
};

int process_rtsp_rtpinfo (char *rtpinfo, 
			  CPlayerSession *session,
			  CPlayerMedia *media)
{
  int ix;
  CPlayerMedia *newmedia;
  if (rtpinfo == NULL) 
    return (0);

  do {
    int no_mimes = 0;
    ADV_SPACE(rtpinfo);
    if (strncasecmp(rtpinfo, "url", strlen("url")) != 0) {
      media_message(LOG_ERR, "Url not found");
      return (-1);
    }
    rtpinfo += strlen("url");
    ADV_SPACE(rtpinfo);
    if (*rtpinfo != '=') {
      media_message(LOG_ERR, "Can't find = after url");
      return (-1);
    }
    rtpinfo++;
    ADV_SPACE(rtpinfo);
    char *url = rtpinfo;
    while (*rtpinfo != '\0' && *rtpinfo != ';' && *rtpinfo != ',') {
      rtpinfo++;
    }
    if (*rtpinfo == '\0') {
      no_mimes = 1;
    } else {
      if (*rtpinfo == ',') {
	no_mimes = 1;
      }
      *rtpinfo++ = '\0';
    }
    char *temp = url;
    newmedia = session->rtsp_url_to_media(url);
    if (newmedia == NULL) {
      media_message(LOG_ERR, "Can't find media from %s", url);
      return -1;
    } else if (media != NULL && media != newmedia) {
      media_message(LOG_ERR, "Url in rtpinfo does not match media %s", url);
      return -1;
    }
    if (temp != url) 
      free(url);

    if (no_mimes == 0) {
    int endofurl = 0;
    do {
      ADV_SPACE(rtpinfo);
      for (ix = 0; rtpinfo_types[ix].name != NULL; ix++) {
	if (strncasecmp(rtpinfo,
			rtpinfo_types[ix].name, 
			rtpinfo_types[ix].namelen - 1) == 0) {
	  rtpinfo += rtpinfo_types[ix].namelen - 1;
	  ADV_SPACE(rtpinfo);
	  rtpinfo = (rtpinfo_types[ix].routine)(rtpinfo, newmedia, endofurl);
	  break;
	}
      }
      if (rtpinfo_types[ix].name == NULL) {
#if 1
	media_message(LOG_INFO, "Unknown mime-type in RtpInfo - skipping %s", 
			     rtpinfo);
#endif
	while (*rtpinfo != ';' && *rtpinfo != '\0') rtpinfo++;
	if (*rtpinfo != '\0') rtpinfo++;
      }
    } while (endofurl == 0 && rtpinfo != NULL && *rtpinfo != '\0');
    } 
    newmedia = NULL;
  } while (rtpinfo != NULL && *rtpinfo != '\0');

  if (rtpinfo == NULL) {
    return (-1);
  }

  return (1);
}

void CPlayerMedia::display_status (void) 
{
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->display_status();
  }
  if (m_sync != NULL) {
    m_sync->display_status();
  }
  media_message(LOG_DEBUG, "%s decode waiting %d", m_is_audio ? "audio" : "video", 
		m_decode_thread_waiting);
}
