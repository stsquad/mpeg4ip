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
 * player_media.cpp - handle generic information about a stream
 * Things to do - pull all rtp info out of this, push it into rtp bytestream
 * class.
 */
#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>

/*
 * c routines for callbacks
 */
static int c_recv_thread (void *data)
{
  CPlayerMedia *media;

  media = (CPlayerMedia *)data;
  return (media->recv_thread());
}

static void c_recv_callback (struct rtp *session, rtp_event *e)
{
  CPlayerMedia *m = (CPlayerMedia *)rtp_get_userdata(session);
  m->recv_callback(session, e);
}

static int c_decode_thread (void *data)
{
  CPlayerMedia *media;
  media = (CPlayerMedia *)data;
  return (media->decode_thread());
}

CPlayerMedia::CPlayerMedia ()
{
  m_next = NULL;
  m_parent = NULL;
  m_media_info = NULL;
  m_media_fmt = NULL;
  m_our_port = 0;
  m_ports = NULL;
  m_server_port = 0;
  m_source_addr = NULL;
  m_recv_thread = NULL;
  m_rtp_packet_received = 0;
  m_rtp_data_received = 0;

  m_rtp_proto = 0;

  m_rtp_packet_mutex = NULL;
  m_head = NULL;
  m_rtp_queue_len = 0;
  m_rtp_queue_len_max = 512;

  m_rtp_ssrc_set = FALSE;
  m_rtp_init_seq_set = FALSE;
  m_rtp_rtptime = 0xffffffff;
  
  m_rtsp_session = NULL;
  m_decode_thread_waiting = 0;
  m_sync_time_set = FALSE;
  m_decode_thread = NULL;
  m_decode_thread_sem = NULL;
  m_video_sync = NULL;
  m_audio_sync = NULL;
  m_paused = 0;
  m_byte_stream = NULL;
  m_rtp_byte_stream = NULL;
  m_video_info = NULL;
  m_audio_info = NULL;
  m_codec_type = NULL;
  m_user_data = NULL;
}

CPlayerMedia::~CPlayerMedia()
{
  rtsp_decode_t *rtsp_decode;

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

  if (m_rtsp_session) {
    rtsp_send_teardown(m_rtsp_session, NULL, &rtsp_decode);
    m_rtsp_session = NULL;
  }
    
  if (m_source_addr) free(m_source_addr);
  m_next = NULL;
  m_parent = NULL;

  if (m_rtp_packet_mutex) {
    SDL_DestroyMutex(m_rtp_packet_mutex);
    m_rtp_packet_mutex = NULL;
  }
  if (m_decode_thread_sem) {
    SDL_DestroySemaphore(m_decode_thread_sem);
    m_decode_thread_sem = NULL;
  }
  if (m_ports) {
    delete m_ports;
    m_ports = NULL;
  }
  if (m_rtp_byte_stream) {
    double diff, div;
    diff = difftime(time(NULL), m_start_time);
    player_debug_message("Media %s", m_media_info->media);
    
    player_debug_message("Time: %g seconds", diff);
    player_debug_message("Packets received: %u", m_rtp_packet_received);
    player_debug_message("Payload received: %llu bytes", m_rtp_data_received);
    div = m_rtp_packet_received / diff;
    player_debug_message("Packets per sec : %g", div);
#ifdef _WINDOWS
    div = (int64_t)m_rtp_data_received;
#else
	div = m_rtp_data_received;
#endif
	div *= 8.0;
	div /= diff;
    player_debug_message("Bits per sec   : %g", div);
			 
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
  if (m_codec_type) {
    free((void *)m_codec_type);
    m_codec_type = NULL;
  }

  if (m_user_data) {
    free((void *)m_user_data);
    m_user_data = NULL;
  }
}
/*
 * CPlayerMedia::create_from_file - create when we've already got a
 * bytestream
 */
int CPlayerMedia::create_from_file (CPlayerSession *p, 
				    COurInByteStream *b, 
				    int is_video)
{
  m_parent = p;
  m_parent->add_media(this);
  m_is_video = is_video;
  m_byte_stream = b;
  m_decode_thread_sem = SDL_CreateSemaphore(0);
  m_decode_thread = SDL_CreateThread(c_decode_thread, this);
  if (m_decode_thread == NULL) {
    player_error_message("Failed to create decode thread for media %s",
			 m_media_info->media);
    return (-1);
  }

  return (0);
}

/*
 * CPlayerMedia::create_streaming - create a streaming media session,
 * including setting up rtsp session, rtp and rtp bytestream
 */
int CPlayerMedia::create_streaming (CPlayerSession *psptr,
				    media_desc_t *sdp_media,
				    const char **errmsg,
				    int ondemand)
{
  char buffer[80];
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  
  if (psptr == NULL || sdp_media == NULL) {
    *errmsg = "Internal media error";
    return(-1);
  }

  if (strncasecmp(sdp_media->proto, "RTP", strlen("RTP")) != 0) {
    *errmsg = "Media doesn't use RTP";
    player_error_message("Media %s doesn't use RTP", sdp_media->media);
    return (-1);
  }
  if (sdp_media->fmt == NULL) {
    *errmsg = "Media doesn't have any usuable formats";
    player_error_message("Media %s doesn't have any formats", 
			 sdp_media->media);
    return (-1);
  }

  m_parent = psptr;
  m_media_info = sdp_media;
  m_is_video = strcmp(sdp_media->media, "video") == 0;
  m_stream_ondemand = ondemand;
  if (ondemand != 0) {
    /*
     * Get 2 consecutive IP ports.  If we don't have this, don't even
     * bother
     */
    m_ports = new C2ConsecIpPort(&global_invalid_ports);
    if (m_ports == NULL || !m_ports->valid()) {
      *errmsg = "Could not find any valid IP ports";
      player_error_message("Couldn't get valid IP ports");
      return (-1);
    }
    m_our_port = m_ports->first_port();

    /*
     * Send RTSP setup message - first create the transport string for that
     * message
     */
    memset(&cmd, 0, sizeof(rtsp_command_t));
    create_rtsp_transport_from_sdp(m_parent->get_sdp_info(),
				   m_media_info,
				   m_our_port,
				   buffer,
				   sizeof(buffer));
  
    cmd.transport = buffer;
    if (rtsp_send_setup(m_parent->get_rtsp_client(),
			m_media_info->control_string,
			&cmd,
			&m_rtsp_session,
			&decode) != 0) {
      *errmsg = "Couldn't set up media session";
      player_error_message("Can't create session %s", m_media_info->media);
      if (decode != NULL)
	free_decode_response(decode);
      return (-1);
    }
    cmd.transport = NULL;
    player_error_message("Transport returned is %s", decode->transport);

    /*
     * process the transport they sent.  They need to send port numbers, 
     * addresses, rtptime information, that sort of thing
     */
    if (process_rtsp_transport(decode->transport) != 0) {
      *errmsg = "Couldn't process transport information";
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
    *errmsg = "Server did not return address";
    return (-1);
  }

  //
  // okay - here we want to check that the server port is set up, and
  // go ahead and init rtp, and the associated task
  //
  m_parent->add_media(this);
  if (ondemand != 0) {
    /*
     * We need to free up the ports that we got before RTP tries to set 
     * them up, so we don't have any re-use conflicts.  There is a small
     * window here that they might get used...
     */
    delete m_ports; // free up the port numbers
    m_ports = NULL;
  }
  m_rtp_session = rtp_init(m_source_addr == NULL ? 
			   cptr->conn_addr : m_source_addr,
			   m_our_port,
			   m_server_port,
			   cptr->ttl, // need ttl here
			   5000.0, // rtcp bandwidth ?
			   c_recv_callback,
			   this);
  m_start_time = time(NULL);
  if (m_rtp_session == NULL) {
    *errmsg = "Couldn't create RTP session";
    player_error_message("Failed to create RTP for media %s", 
			 m_media_info->media);
    return (-1);
  }
  rtp_setopt(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);

  /*
   * Create the various mutexes, semaphores, threads, etc
   */
  m_rtp_packet_mutex = SDL_CreateMutex();
  m_decode_thread_sem = SDL_CreateSemaphore(0);
  if (m_rtp_packet_mutex == NULL) {
    *errmsg = "Couldn't create mutex";
    player_error_message("Failed to create mutex for media %s",
			 m_media_info->media);
    return (-1);
  }

  m_decode_thread = SDL_CreateThread(c_decode_thread, this);
  if (m_decode_thread == NULL) {
    *errmsg = "Couldn't create media thread";
    player_error_message("Failed to create decode thread for media %s",
			 m_media_info->media);
    return (-1);
  }

  m_recv_thread = SDL_CreateThread(c_recv_thread, this);
  if (m_recv_thread == NULL) {
    *errmsg = "Couldn't create media thread";
    player_error_message("Failed to create thread for media %s RTP recv",
			 m_media_info->media);
    return (-1);
  }
  /*
   * create the rtp bytestream
   */
  m_rtp_byte_stream = new CInByteStreamRtp(this);
  m_byte_stream = m_rtp_byte_stream;
  return (0);
}

/*
 * CPlayerMedia::do_play - get play command
 */
int CPlayerMedia::do_play (double start_time_offset)
{
  char buffer[80];
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  range_desc_t *range;

  if (m_rtp_byte_stream != NULL) {
    if (m_stream_ondemand != 0) {
      /*
       * We're streaming - send the RTSP play command
       */
      memset(&cmd, 0, sizeof(rtsp_command_t));

      // only do range if we're not paused
      range = get_range_from_media(m_media_info);
      if (range == NULL) {
	return (-1);
      }
      if (start_time_offset < range->range_start || 
	  start_time_offset > range->range_end) 
	start_time_offset = range->range_start;
      // need to check for smpte
      sprintf(buffer, "npt=%g-%g", start_time_offset, range->range_end);
      cmd.range = buffer;

      if (rtsp_send_play(m_rtsp_session, &cmd, &decode) != 0) {
	player_error_message("RTSP play command failed");
	free_decode_response(decode);
	return (-1);
      }

      /*
       * process the return information
       */
      if (process_rtsp_rtpinfo(decode->rtp_info) != 0) {
	free_decode_response(decode);
	return (-1);
      }
      /*
       * set the various play times, and send a message to the recv task
       * that it needs to start
       */
      m_play_start_time = start_time_offset;
      m_byte_stream->set_start_time((uint64_t)(m_play_start_time * 1000.0));
      free_decode_response(decode);
    }
    m_paused = 0;
    m_rtp_msg_queue.send_message(MSG_START_SESSION);
  } else {
    /*
     * File (or other) playback.
     */
    if (m_paused == 0 || start_time_offset == 0.0) {
      m_byte_stream->reset();
    }
    m_byte_stream->set_start_time((uint64_t)(start_time_offset * 1000.0));
    m_play_start_time = start_time_offset;
    m_paused = 0;
    m_decode_msg_queue.send_message(MSG_START_DECODING, 
				    NULL, 
				    0, 
				    m_decode_thread_sem);
  }
  return (0);
}

/*
 * CPlayerMedia::do_pause - stop what we're doing
 */
int CPlayerMedia::do_pause (void)
{
  rtsp_command_t cmd;
  rtsp_decode_t *decode;

  if (m_rtp_byte_stream != NULL) {
    /*
     * streaming - send RTSP pause
     */
    memset(&cmd, 0, sizeof(rtsp_command_t));

    if (rtsp_send_pause(m_rtsp_session, &cmd, &decode) != 0) {
      player_error_message("RTSP play command failed");
      free_decode_response(decode);
      return (-1);
    }
    free_decode_response(decode);
  }
  
  /*
   * Pause the various threads
   */
  m_rtp_msg_queue.send_message(MSG_PAUSE_SESSION);
  m_decode_msg_queue.send_message(MSG_PAUSE_SESSION, 
				  NULL, 
				  0, 
				  m_decode_thread_sem);
  m_paused = 1;
  return (0);
}

/***************************************************************************
 * Transport and RTP-Info RTSP header line parsing.
 ***************************************************************************/
#define ADV_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}

#define TTYPE(a,b) {a, sizeof(a), b}

static char *transport_parse_unicast (char *transport, CPlayerMedia *m)
{
  ADV_SPACE(transport);
  if (*transport == '\0') return (transport);

  if (*transport != ';')
    return (NULL);
  transport++;
  ADV_SPACE(transport);
  return (transport);
}

static char *transport_parse_multicast (char *transport, CPlayerMedia *m)
{
  player_error_message("Received multicast indication during SETUP");
  return (NULL);
}

static char *convert_number (char *transport, uint32_t &value)
{
  value = 0;
  while (isdigit(*transport)) {
    value *= 10;
    value += *transport - '0';
    transport++;
  }
  return (transport);
}

static char *transport_parse_client_port (char *transport, CPlayerMedia *m)
{
  uint32_t port;
  uint16_t our_port, our_port_max;
  if (*transport++ != '=') {
    return (NULL);
  }
  ADV_SPACE(transport);
  transport = convert_number(transport, port);
  ADV_SPACE(transport);
  our_port = m->get_our_port();
  our_port_max = our_port + 1;

  if (port != our_port) {
    player_error_message("Returned client port %u doesn't match sent %u",
			 port, our_port);
    return (NULL);
  }
  if (*transport == ';') {
    transport++;
    return (transport);
  }
  if (*transport == '\0') {
    return (transport);
  }
  if (*transport != '-') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, port);
  if ((port < our_port) || 
      (port > our_port_max)) {
    player_error_message("Illegal client to port %u, range %u to %u",
			 port, our_port, our_port_max);
    return (NULL);
  }
  ADV_SPACE(transport);
  if (*transport == ';') {
    transport++;
  }
  return(transport);
}

static char *transport_parse_server_port (char *transport, CPlayerMedia *m)
{
  uint32_t fromport, toport;

  if (*transport++ != '=') {
    return (NULL);
  }
  ADV_SPACE(transport);
  transport = convert_number(transport, fromport);
  ADV_SPACE(transport);

  m->set_server_port(fromport);

  if (*transport == ';') {
    transport++;
    return (transport);
  }
  if (*transport == '\0') {
    return (transport);
  }
  if (*transport != '-') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, toport);
  if (toport < fromport || toport > fromport + 1) {
    player_error_message("Illegal server to port %u, from is %u",
			 toport, fromport);
    return (NULL);
  }
  ADV_SPACE(transport);
  if (*transport == ';') {
    transport++;
  }
  return(transport);
}

static char *transport_parse_source (char *transport, CPlayerMedia *m)
{
  char *ptr, *newone;
  size_t addrlen;

  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  ptr = transport;
  while (*transport != ';' && *transport != '\0') transport++;
  addrlen = transport - ptr;
  if (addrlen == 0) {
    return (NULL);
  }
  newone = (char *)malloc(addrlen + 1);
  if (newone == NULL) {
    player_error_message("Can't alloc memory for transport source");
    return (NULL);
  }
  strncpy(newone, ptr, addrlen);
  newone[addrlen] = '\0';
  m->set_source_addr(newone);
  if (*transport == ';') transport++;
  return (transport);
}

static char *transport_parse_ssrc (char *transport, CPlayerMedia *m)
{
  uint32_t ssrc;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, ssrc);
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport != ';') {
      return (NULL);
    }
    transport++;
  }
  m->set_rtp_ssrc(ssrc);
  return (transport);
}

static char *rtpinfo_parse_seq (char *rtpinfo, CPlayerMedia *m)
{
  uint32_t seq;
  if (*rtpinfo != '=') {
    return (NULL);
  }
  rtpinfo++;
  ADV_SPACE(rtpinfo);
  rtpinfo = convert_number(rtpinfo, seq);
  ADV_SPACE(rtpinfo);
  if (*rtpinfo != '\0') {
    if (*rtpinfo != ';') {
      return (NULL);
    }
    rtpinfo++;
  }
  m->set_rtp_init_seq(seq);
  return (rtpinfo);
}

static char *rtpinfo_parse_rtptime (char *rtpinfo, CPlayerMedia *m)
{
  uint32_t rtptime;
  if (*rtpinfo != '=') {
    return (NULL);
  }
  rtpinfo++;
  ADV_SPACE(rtpinfo);
  rtpinfo = convert_number(rtpinfo, rtptime);
  ADV_SPACE(rtpinfo);
  if (*rtpinfo != '\0') {
    if (*rtpinfo != ';') {
      return (NULL);
    }
    rtpinfo++;
  }
  m->set_rtp_rtptime(rtptime);
  return (rtpinfo);
}
struct {
  const char *name;
  size_t namelen;
  char *(*routine)(char *transport, CPlayerMedia *);
} transport_types[] = 
{
  TTYPE("unicast", transport_parse_unicast),
  TTYPE("multicast", transport_parse_multicast),
  TTYPE("client_port", transport_parse_client_port),
  TTYPE("server_port", transport_parse_server_port),
  TTYPE("source", transport_parse_source),
  TTYPE("ssrc", transport_parse_ssrc),
  {NULL, 0, NULL},
}, rtpinfo_types[] = 
{
  TTYPE("seq", rtpinfo_parse_seq),
  TTYPE("rtptime", rtpinfo_parse_rtptime),
  TTYPE("ssrc", transport_parse_ssrc),
  {NULL, 0, NULL},
};

int CPlayerMedia::process_rtsp_transport (char *transport)
{
  size_t protolen;
  int ix;

  if (transport == NULL) 
    return (-1);

  protolen = strlen(m_media_info->proto);
  
  if (strncasecmp(transport, m_media_info->proto, protolen) != 0) {
    player_error_message("transport %s doesn't match %s", transport, 
			 m_media_info->proto);
    return (-1);
  }
  transport += protolen;
  if (*transport == '/') {
    transport++;
    if (strncasecmp(transport, "UDP", strlen("UDP")) != 0) {
      player_error_message("Transport is not UDP");
      return (-1);
    }
    transport += strlen("UDP");
  }
  if (*transport != ';') {
    return (-1);
  }
  transport++;
  do {
    ADV_SPACE(transport);
    for (ix = 0; transport_types[ix].name != NULL; ix++) {
      if (strncasecmp(transport, 
		      transport_types[ix].name, 
		      transport_types[ix].namelen - 1) == 0) {
	transport += transport_types[ix].namelen - 1;
	ADV_SPACE(transport);
	transport = (transport_types[ix].routine)(transport, this);
	break;
      }
    }
    if (transport_types[ix].name == NULL) {
      player_error_message("Illegal name in transport - skipping %s", 
			   transport);
      while (*transport != ';' && *transport != '\0') transport++;
      if (*transport != '\0') transport++;
    }
  } while (transport != NULL && *transport != '\0');

  if (transport == NULL) {
    return (-1);
  }
  return (0);
}

int CPlayerMedia::process_rtsp_rtpinfo (char *rtpinfo)
{
  int ix;

  if (rtpinfo == NULL) 
    return (-1);

  do {
    ADV_SPACE(rtpinfo);
    for (ix = 0; rtpinfo_types[ix].name != NULL; ix++) {
      if (strncasecmp(rtpinfo,
		      rtpinfo_types[ix].name, 
		      rtpinfo_types[ix].namelen - 1) == 0) {
	rtpinfo += rtpinfo_types[ix].namelen - 1;
	ADV_SPACE(rtpinfo);
	rtpinfo = (rtpinfo_types[ix].routine)(rtpinfo, this);
	break;
      }
    }
    if (rtpinfo_types[ix].name == NULL) {
#if 0
      player_debug_message("Illegal name in RtpInfo - skipping %s", 
			   rtpinfo);
#endif
      while (*rtpinfo != ';' && *rtpinfo != '\0') rtpinfo++;
      if (*rtpinfo != '\0') rtpinfo++;
    }
  } while (rtpinfo != NULL && *rtpinfo != '\0');

  if (rtpinfo == NULL) {
    return (-1);
  }
  return (0);
}

/****************************************************************************
 * RTP receive routines
 ****************************************************************************/
int CPlayerMedia::recv_thread (void)
{
  struct timeval timeout;
  int retcode;
  int buffering = 0;
  CMsg *newmsg;
  int recv_thread_stop = 0;
  int receiving = 0;

  while (recv_thread_stop == 0) {
    /*
     * See if we need to check for a state change - this will allow
     * changes to the rtp info, if required.
     */
    while ((newmsg = m_rtp_msg_queue.get_message()) != NULL) {
      switch (newmsg->get_value()) {
      case MSG_STOP_THREAD:
	recv_thread_stop = 1;
	continue;
      case MSG_START_SESSION:
	if (m_rtp_ssrc_set == TRUE) {
	  rtp_set_my_ssrc(m_rtp_session, m_rtp_ssrc);
	} else {
	  // For now - we'll set up not to wait for RTCP validation 
	  // before indicating if rtp library should accept.
	  rtp_setopt(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
	}
	flush_rtp_packets();
	buffering = 0;
	receiving = 1;
	break;
      case MSG_PAUSE_SESSION:
	break;
      }
      delete newmsg;
    }

    while (receiving == 1 && recv_thread_stop == 0) {
      if ((newmsg = m_rtp_msg_queue.get_message()) != NULL) {
	//player_debug_message("recv thread message %d", newmsg->get_value());
	switch (newmsg->get_value()) {
	case MSG_STOP_THREAD:
	  recv_thread_stop = 1;
	  break;
	case MSG_START_SESSION:
	  player_debug_message("Got play when playing");
	  break;
	case MSG_PAUSE_SESSION:
	  receiving = 0;
	  break;
	}
	delete newmsg;
	newmsg = NULL;
      }
      if (receiving == 0 || recv_thread_stop == 1) {
	continue;
      }
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;
      retcode = rtp_recv(m_rtp_session, &timeout, 0);
      rtp_send_ctrl(m_rtp_session, m_rtptime_last, NULL);
      rtp_update(m_rtp_session);
      /*
       * We need to make sure we have some buffering.  We'll buffer
       * about 2 seconds worth, then let the decode task know to go...
       */
      if (buffering == 0) {
	uint32_t head_ts, tail_ts;
	if (m_head != NULL) {
	  /*
	   * Make sure that the proto is the same
	   */
	  if (m_rtp_proto == 0) {
	    if (m_head->pt == m_tail->pt) {
	      if (m_rtp_queue_len > 10) { // 10 packets consecutive proto same
		if (determine_proto_from_rtp() == FALSE)
		  flush_rtp_packets();
	      }
	    } else {
	      flush_rtp_packets();
	    }
	  } else {
	    /*
	     * Protocol the same.  Make sure we have at least 2 seconds of
	     * good data
	     */
	    if (m_tail->m) {
	      head_ts = m_head->ts;
	      tail_ts = m_tail->ts;
	      double calc;
	      calc = (tail_ts - head_ts);
	      calc /= m_rtptime_tickpersec;
#if 0
	      player_debug_message("head %u tail %u diff %u div %u in queue %u - time diff is %g", head_ts, tail_ts, tail_ts - head_ts, m_rtptime_tickpersec, m_rtp_queue_len, calc);
#endif
	      if (calc > 2.0) {
		buffering = 1;
#if 1
		player_debug_message("buffering complete - head %u tail %u %g - count %u", 
				     head_ts, tail_ts, calc, m_rtp_queue_len);
#endif

		m_decode_msg_queue.send_message(MSG_START_DECODING, 
						NULL, 
						0, 
						m_decode_thread_sem);
	      }
	    }
	  }
	}
      } else if (m_decode_thread_waiting != 0) {
	/*
	 * We're good with buffering - but the decode thread might have
	 * caught up, and will be waiting.  Post a message to kickstart
	 * it
	 */
	if (m_tail->m == 1) {
	  m_decode_thread_waiting = 0;
	  SDL_SemPost(m_decode_thread_sem);
	}
      }
    }
  }
  /*
   * When we're done, send a bye, close up rtp, and go home
   */
  rtp_send_bye(m_rtp_session);
  rtp_done(m_rtp_session);
  m_rtp_session = NULL;
  return (0);
}

int count = 0, cnt2 = 0;
/*
 * CPlayerMedia::recv_callback - callback from RTP with valid data
 */
void CPlayerMedia::recv_callback (struct rtp *session, rtp_event *e)
{
  if (e == NULL) return;
  switch (e->type) {
  case RX_RTP:
    /* regular rtp packet - add it to the queue */
    rtp_packet *rpak;
    m_rtp_packet_received++;
    rpak = (rtp_packet *)e->data;
    m_rtp_data_received += rpak->data_len;
    m_rtptime_last = rpak->ts;
    count++;
#if 0
    if (count > 20) {
      cnt2++;
      if (cnt2 > 5) {
	count = 0;
	cnt2 = 0;
      }
      xfree(rpak);
      player_debug_message("Dropped packet");
    } else
#endif
      if (rpak->data_len == 0) {
	xfree(rpak);
      } else 
    add_packet_to_queue(rpak);
    break;
  case RX_SR:
#if 0
    rtcp_sr *srpak;

    //This is close enough for now.  It gives an error in the .01 and .001
    // decimal place.			   
    srpak = (rtcp_sr *)e->data;

    player_debug_message("ntp time %u %u rtp %u", 
			 srpak->ntp_sec, srpak->ntp_frac, srpak->rtp_ts);
    double temp, rem;
    // Convert fractional part to microseconds.
    temp = srpak->ntp_frac;
    temp /= (1LL << 32);
    temp += srpak->ntp_sec;

    rem = (srpak->rtp_ts - m_rtp_rtptime );
    rem /= m_rtptime_tickpersec;
    player_debug_message("ntp conversion is %f", temp - rem);
#endif
    break;

  default:
#if 0
    player_debug_message("Thread %u - Callback from rtp with %d %x", 
			 SDL_ThreadID(),e->type, e->data);
#endif
    break;
  }
}


/*
 * add_packet_to_queue - adds the rtp packet to the queue - this is 
 * an ordered link list base on rpak->seq
 */
int CPlayerMedia::add_packet_to_queue (rtp_packet *pak)
{
  rtp_packet *q;
  int inserted = TRUE;
  int16_t diff;
#if 0
    player_debug_message("CBThread %u - m %u pt %u seq %x ts %u len %d", 
			 SDL_ThreadID(),
			 pak->m, pak->pt, pak->seq, pak->ts, 
			 
			 pak->data_len);
#endif
  if (SDL_mutexP(m_rtp_packet_mutex) == -1) {
    player_error_message("SDL Lock mutex failure in recv thread");
    return (-1);
  }
  
  if (m_head == NULL) {
    m_head = m_tail = pak;
  } else if (m_head == m_tail) {
    m_head->next = pak;
    m_head->prev = pak;
    pak->next = pak->prev = m_head;
    diff = pak->seq - m_head->seq;
    if (diff > 0) {
      m_tail = pak;
    } else {
      m_head = pak;
    }
  } else if (((m_head->seq < m_tail->seq) &&
	      ((pak->seq > m_tail->seq) || pak->seq < m_head->seq)) ||
	     ((m_head->seq > m_tail->seq) &&
	      ((pak->seq > m_tail->seq && pak->seq < m_head->seq)))) {
    // insert between tail and head
    // Maybe move head - probably move tail.
    m_tail->next = pak;
    pak->prev = m_tail;
    m_head->prev = pak;
    pak->next = m_head;
    diff = m_head->seq - pak->seq;
    if (diff > 0 && diff < 4) {
      // between tail and head, and really close to the head - move the
      // head pointer
      m_head = pak;
    } else {
      // otherwise, just insert at end
      m_tail = pak;
    }
  } else {
    // insert in middle
    // Loop through until we find where it should fit
    q = m_head;
    do {
      // check for duplicates
      if (pak->seq == q->seq || pak->seq == q->next->seq) {
	// dup seq number
	inserted = FALSE;
	break;
      }
      // Okay - this is disgusting, but works.  The first part (before the
      // or) will see if pak->seq is between q and q->next, assuming that the 
      // sequence number for q and q->next are ascending.
      //
      // 2nd part of the if is the converse case (q->next->seq is smaller
      // than q->seq).  In that case, we need to make sure that pak->seq is
      // either larger than q->seq or less than q->next->seq
      if (((q->next->seq > q->seq) &&
	   (q->seq < pak->seq && pak->seq < q->next->seq)) ||
	  ((q->next->seq < q->seq) &&
	   (pak->seq < q->next->seq || pak->seq > q->seq))) {
	q->next->prev = pak;
	pak->next = q->next;
	q->next = pak;
	pak->prev = q;
	break;
      }
      q = q->next;
    } while (q != m_tail);
    if (q == m_tail) {
      inserted = FALSE;
      player_error_message("Couldn't insert %u between %u and %u", 
			   pak->seq, m_head->seq, m_tail->seq);
    }
  }
  if (inserted == TRUE)
    m_rtp_queue_len++;
  if (SDL_mutexV(m_rtp_packet_mutex) == -1) {
    player_error_message("SDL unlock mutex failure in recv thread");
    return (-1);
  }
  if (inserted == FALSE) {
    player_error_message("Couldn't insert pak");
    xfree(pak);
  }
  return (0);
}

/*
 * determine_proto_from_rtp - determine with protocol we're dealing with
 * in the rtp session.  Set various calculations for the sync task, as well...
 */
int CPlayerMedia::determine_proto_from_rtp(void)
{
  char proto = m_head->pt, temp;
  format_list_t *fmt;

  fmt = m_media_info->fmt;
  while (fmt != NULL) {
    temp = atoi(fmt->fmt);
    if (temp == proto) {
      m_media_fmt = fmt;
      m_rtp_proto = proto;
      if (m_rtp_proto >= 96) {
	if (fmt->rtpmap == NULL) {
	  player_error_message("Media %s, rtp proto of %u, no rtp map",
			       m_media_info->media, m_rtp_proto);
	  return (FALSE);
	}
	m_rtptime_tickpersec = fmt->rtpmap->clock_rate;
#if 0
	player_debug_message("media %s - rtp proto %u - clock rate %u",
			     m_media_info->media, m_rtp_proto, 
			     m_rtptime_tickpersec);
#endif
	set_codec_type(fmt->rtpmap->encode_name);
      } else {
	m_rtptime_tickpersec = 90000;
      }

      m_rtptime_ntptickperrtptick = 1;
	  m_rtptime_ntptickperrtptick <<= 32;
	  m_rtptime_ntptickperrtptick /= m_rtptime_tickpersec;
      m_rtp_byte_stream->set_rtp_config(m_rtptime_tickpersec);
#if 0
      player_debug_message("media %s - rtp tps %u ntp per rtp %llu",
			   m_media_info->media,
			   m_rtptime_tickpersec, 
			   m_rtptime_ntptickperrtptick);
#endif

      return (TRUE);
    }
    fmt = fmt->next;
  }
  return (FALSE);
}

void CPlayerMedia::flush_rtp_packets (void) 
{
  SDL_mutexP(m_rtp_packet_mutex);
  if (m_head != NULL) {
    m_tail->next = NULL;
    while (m_head != NULL) {
      rtp_packet *p;
      p = m_head;
      m_head = m_head->next;
      p->next = p->prev = NULL;
      xfree(p);
    }
  }
  m_rtp_queue_len = 0;
  SDL_mutexV(m_rtp_packet_mutex);
}

/*
 * set up rtptime
 */
void CPlayerMedia::set_rtp_rtptime (uint32_t time)
{
  m_rtp_rtptime = time;
  m_rtp_byte_stream->set_rtp_rtptime(time);
  player_debug_message("setting rtptime to %u", time);
};


/* end player_media.cpp */
