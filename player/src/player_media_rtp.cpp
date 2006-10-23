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
 * Copyright (C) Cisco Systems Inc. 2003.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"
#include <rtp/memory.h>
#include "rtp_bytestream.h"
#include "our_config_file.h"
#include "rtp_plugin.h"
#include "media_utils.h"
#include "rfc3119_bytestream.h"
#include "mpeg3_rtp_bytestream.h"
#include "rtp_bytestream_plugin.h"
#include "liblibsrtp.h"

#include "codec_plugin_private.h"
//#define DROP_PAKS 1
static void c_recv_callback (struct rtp *session, rtp_event *e)
{
  CPlayerMedia *m = (CPlayerMedia *)rtp_get_recv_userdata(session);
  m->recv_callback(session, e);
}

static int c_rtcp_send_packet (void *ud, uint8_t *buffer, uint32_t buflen)
{
  return ((CPlayerMedia *)ud)->rtcp_send_packet(buffer, buflen);
}


int CPlayerMedia::rtp_receive_packet (unsigned char interleaved, 
				      struct rtp_packet *pak)
{
  int ret;
  if ((interleaved & 1) == 0) {
    ret = rtp_process_recv_data(m_rtp_session, 0, pak);
    if (ret < 0) {
      xfree(pak);
    }
  } else {

    rtp_process_ctrl(m_rtp_session, pak->packet_start, pak->packet_length);
    xfree(pak);
    ret = 0;
  }
  return ret;
}

/*
 * rtp_peridic - called from receive task; generates RTCP when needed; also
 * checks for end of stream - if so, it kicks the decoder task, to finish off
 */
void CPlayerMedia::rtp_periodic (void)
{
  if (m_rtp_session_from_outside == false) {
    rtp_send_ctrl(m_rtp_session, 
		  m_rtp_byte_stream != NULL ? 
		  m_rtp_byte_stream->get_last_rtp_timestamp() : 0, 
		  NULL);
    rtp_update(m_rtp_session);
  }
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->rtp_periodic();
    if (m_rtp_byte_stream->eof()) {
      bytestream_primed();
    }
  }
}

/*
 * rtp_check_payload - this is called after receiving packets, but before a
 * m_rtp_byte_stream is selected.
 */
void CPlayerMedia::rtp_check_payload (void)
{
  // note wmay - 3/16/2005 - need to push in text here.
  // we'll only every allow 1 type to be read, so we'll be able to
  // set up the interface directly.
  bool try_bytestream_buffering = false;

  if (get_sync_type() == TIMED_TEXT_SYNC) {
    determine_payload_type_from_rtp();
    // set the buffer time to 0, so we can pass buffering immediately
    m_rtp_byte_stream->set_rtp_buffer_time(0);
    try_bytestream_buffering = true;
  } else {
    if (m_head != NULL) {
      /*
       * Make sure that the payload type is the same
       */
      if (m_head->rtp_pak_pt == m_tail->rtp_pak_pt) {
	// we either want only 1 possible protocol, or at least
	// 10 packets of the same consecutively.  10 is arbitrary.
	if (m_media_info->fmt_list->next == NULL || 
	    m_rtp_queue_len > 10) { 
	  if (determine_payload_type_from_rtp() == FALSE) {
	    clear_rtp_packets(); 
	  } else {
	    // call this function again so we begin the buffering check
	    // right away - better than a go-to.
	    try_bytestream_buffering = true;
	  }
	}
      } else {
	clear_rtp_packets();
      }
    }
  }
  // if we've set the rtp bytestream, immediately try and see if they are done
  // buffering;  if so, start the decode task moving.
  if (try_bytestream_buffering) {
    if (m_rtp_byte_stream->check_buffering() != 0) {
      m_rtp_buffering = 1;
      start_decoding();
    }
  }
}

void CPlayerMedia::rtp_start (void)
{
  if (m_rtp_ssrc_set == TRUE) {
    rtp_set_my_ssrc(m_rtp_session, m_rtp_ssrc);
  } else {
    // For now - we'll set up not to wait for RTCP validation 
    // before indicating if rtp library should accept.
    rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
    rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
  }
  if (m_rtp_byte_stream != NULL) {
    //m_rtp_byte_stream->reset(); - gets called when pausing
    //    m_rtp_byte_stream->flush_rtp_packets();
  }
  m_rtp_buffering = 0;
}

void CPlayerMedia::rtp_end(void)
{
  if (m_rtp_session != NULL && m_rtp_session_from_outside == false) {
    rtp_send_bye(m_rtp_session);
    rtp_done(m_rtp_session);
  } else {
    rtp_set_rtp_callback(m_rtp_session, NULL, NULL);
  }

  m_rtp_session = NULL;
  if (m_srtp_session != NULL) {
    destroy_srtp(m_srtp_session);
    m_srtp_session = NULL;
  }
}

int CPlayerMedia::rtcp_send_packet (uint8_t *buffer, uint32_t buflen)
{
  if (config.get_config_value(CONFIG_SEND_RTCP_IN_RTP_OVER_RTSP) != 0) {
    return rtsp_thread_send_rtcp(m_parent->get_rtsp_client(),
				 m_rtp_media_number_in_session,
				 buffer, 
				 buflen);
  }
  return buflen;
}
/****************************************************************************
 * RTP receive routines
 ****************************************************************************/
int CPlayerMedia::recv_thread (void)
{
  struct timeval timeout;
  int retcode;
  CMsg *newmsg;
  int recv_thread_stop = 0;

  m_rtp_buffering = 0;
  if (m_ports != NULL) {
    /*
     * We need to free up the ports that we got before RTP tries to set 
     * them up, so we don't have any re-use conflicts.  There is a small
     * window here that they might get used...
     */
    delete m_ports; // free up the port numbers
    m_ports = NULL;
  }

#ifdef _WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  int ret;
 
  wVersionRequested = MAKEWORD( 2, 0 );
  
  ret = WSAStartup( wVersionRequested, &wsaData );
  if ( ret != 0 ) {
    abort();
  }
#endif
  rtp_init(false);
  if (m_rtp_session != NULL) {
    rtp_start();
  }

  
  while (recv_thread_stop == 0) {
    if ((newmsg = m_rtp_msg_queue.get_message()) != NULL) {
      //player_debug_message("recv thread message %d", newmsg->get_value());
      switch (newmsg->get_value()) {
      case MSG_STOP_THREAD:
	recv_thread_stop = 1;
	break;
      case MSG_PAUSE_SESSION:
	// Indicate that we need to restart the session.
	// But keep going...
	rtp_start();
	break;
      }
      delete newmsg;
      newmsg = NULL;
    }
    if (recv_thread_stop == 1) {
      continue;
    }
    if (m_rtp_session == NULL) {
      SDL_Delay(50); 
    } else {
      if (m_rtp_session_from_outside) {
	SDL_Delay(500);
      } else {
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	retcode = rtp_recv(m_rtp_session, &timeout, 0);
      }
      //      player_debug_message("rtp_recv return %d", retcode);
      // Run rtp periodic after each packet received or idle time.
      if (m_paused == false || m_stream_ondemand != 0)
	rtp_periodic();
    }
    
  }
  /*
   * When we're done, send a bye, close up rtp, and go home
   */
  rtp_end();
  return (0);
}

#ifdef DROP_PAKS
static int dropcount = 0;
#endif

/*
 * CPlayerMedia::recv_callback - callback from RTP with valid data
 */
void CPlayerMedia::recv_callback (struct rtp *session, rtp_event *e)
{
  if (e == NULL) return;
  /*
   * If we're paused, just dump the packet.  Multicast case
   */
  if (m_paused) {
    if (e->type == RX_RTP) {
      media_message(LOG_DEBUG, "%s dropping pak", get_name());
      xfree(e->data);
      return;
    }
  }
#if DROP_PAKS
    if (e->type == RX_RTP && dropcount >= 50) {
      xfree((rtp_packet *)e->data);
      dropcount = 0;
      return;
    } else { 
      dropcount++;
    }
#endif
  if (m_rtp_byte_stream != NULL) {
    if ((m_rtp_byte_stream->recv_callback(session, e) > 0) &&
	(e->type == RX_RTP)) {
      // indicates they are done buffering.
      if (m_rtp_buffering == 0) {
	m_rtp_buffering = 1;
	start_decoding();
      } else {
	// we're not buffering, but the decode thread might be waiting; if so, 
	// tweak it if we have a complete frame.
	if (m_decode_thread_waiting) {
	  if (m_rtp_byte_stream->check_rtp_frame_complete_for_payload_type()) {
	    bytestream_primed();
	  }
	}
      }
    }
    return;
  }
  // we only get here if there is not a valid rtp bytestream yet.
  switch (e->type) {
  case RX_RTP:
    /* regular rtp packet - add it to the queue */
    rtp_packet *rpak;
      
    rpak = (rtp_packet *)e->data;
    if (rpak->rtp_data_len == 0) {
      xfree(rpak);
    } else {
      rpak->pd.rtp_pd_timestamp = get_time_of_day();
      rpak->pd.rtp_pd_have_timestamp = true;
      add_rtp_packet_to_queue(rpak, &m_head, &m_tail, 
			      get_name());
      m_rtp_queue_len++;
      rtp_check_payload();
    }
    break;
  case RX_SR:
    rtcp_sr *srpak;
    srpak = (rtcp_sr *)e->data;

    m_rtcp_ntp_frac = srpak->ntp_frac;
    m_rtcp_ntp_sec = srpak->ntp_sec;
    m_rtcp_rtp_ts = srpak->rtp_ts;
    m_rtcp_received = 1;
    break;
  case RX_APP:
    free(e->data);
    break;
  default:
  case RX_RR:
  case RX_SDES:
  case RX_BYE:
  case SOURCE_CREATED:
  case SOURCE_DELETED:
  case RX_RR_EMPTY:
  case RX_RTCP_START:
  case RX_RTCP_FINISH:
  case RR_TIMEOUT:
#if 0
    media_message(LOG_DEBUG, "Thread %u - Callback from rtp with %d %p", 
		  SDL_ThreadID(),e->type, e->data);
#endif
    break;
  }
}

/*
 * determine_payload_type_from_rtp - determine with protocol we're dealing with
 * in the rtp session.  Set various calculations for the sync task, as well...
 */
int CPlayerMedia::determine_payload_type_from_rtp(void)
{
  uint8_t payload_type, temp;
  format_list_t *fmt;
  uint64_t tickpersec;

  if (m_head != NULL) {
    payload_type = m_head->rtp_pak_pt;
  } else {
    payload_type = atoi(m_media_info->fmt_list->fmt);
  }
  fmt = m_media_info->fmt_list;
  while (fmt != NULL) {
    // rtp payloads are all numeric
    temp = atoi(fmt->fmt);
    if (temp == payload_type) {
      m_media_fmt = fmt;
      if (fmt->rtpmap_name != NULL) {
	tickpersec = fmt->rtpmap_clock_rate;
      } else {
	if (payload_type >= 96) {
	  media_message(LOG_ERR, "Media %s, rtp payload type of %u, no rtp map",
			m_media_info->media, payload_type);
	  return (FALSE);
	} else {
	  // generic payload type.  between 0 and 23 are audio - most
	  // are 8000
	  // all video (above 25) are 90000
	  tickpersec = 90000;
	  // this will handle the >= 0 case as well.
	  if (payload_type <= 23) {
	    tickpersec = 8000;
	    if (payload_type == 6) {
	      tickpersec = 16000;
	    } else if (payload_type == 10 || payload_type == 11) {
	      tickpersec = 44100;
	    } else if (payload_type == 14) 
	      tickpersec = 90000;
	  }
	}
      }

      create_rtp_byte_stream(payload_type,
			     tickpersec,
			     fmt);
      uint64_t start_time = (uint64_t)(m_play_start_time * 1000.0);
      m_rtp_byte_stream->play(start_time);
      m_byte_stream = m_rtp_byte_stream;
      if (is_audio()) {
	m_rtp_byte_stream->set_sync(m_parent);
      } else {
	m_parent->synchronize_rtp_bytestreams(NULL);
      }
#if 1
      media_message(LOG_DEBUG, "media %s - rtp tps %u ntp per rtp ",
			   m_media_info->media,
			   m_rtptime_tickpersec);
#endif

      return (TRUE);
    }
    fmt = fmt->next;
  }
  media_message(LOG_ERR, "Payload type %d not in format list for media %s", 
		payload_type, get_name());
  return (FALSE);
}

/*
 * set up rtptime
 */
void CPlayerMedia::set_rtp_base_ts (uint32_t time)
{
  m_rtsp_base_ts_received = 1;
  m_rtp_base_ts = time;
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->set_rtp_base_ts(time);
  }
}

void CPlayerMedia::set_rtp_base_seq (uint16_t seq)
{
  m_rtsp_base_seq_received = 1; 
  m_rtp_base_seq = seq;
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->set_rtp_base_seq(seq);
  }
}

void CPlayerMedia::rtp_init (bool do_tcp) 
{
  rtp_stream_params_t rsp;
  connect_desc_t *cptr;
  double bw;

  if (m_rtp_session == NULL) {
    m_rtp_session_from_outside = false;
    rtp_default_params(&rsp);

    if (find_rtcp_bandwidth_from_media(m_media_info, &bw) < 0) {
      bw = 5000.0;
    } 
    rsp.rtcp_bandwidth = bw;
    cptr = get_connect_desc_from_media(m_media_info);
  
    rsp.rtp_addr = m_source_addr == NULL ? cptr->conn_addr : m_source_addr;
    rsp.rtp_rx_port = m_our_port;
    rsp.rtp_tx_port = m_server_port;
    rsp.rtp_ttl = cptr == NULL ? 1 : cptr->ttl;
    rsp.rtp_callback = c_recv_callback;
    if (do_tcp) {
      rsp.dont_init_sockets = 1;
      rsp.rtcp_send_packet = c_rtcp_send_packet;
    } else {
      // udp.  See if we have a seperate rtcp field
      if (m_media_info->rtcp_connect.used) {
	rsp.rtcp_rx_port = m_media_info->rtcp_port;
	rsp.rtcp_addr = m_media_info->rtcp_connect.conn_addr;
	rsp.rtcp_ttl = m_media_info->rtcp_connect.ttl;
      }
      if (config.get_config_string(CONFIG_MULTICAST_RX_IF) != NULL) {
	struct in_addr if_addr;
	if (getIpAddressFromInterface(config.get_config_string(CONFIG_MULTICAST_RX_IF),
				      &if_addr) >= 0) {
	  rsp.physical_interface_addr = inet_ntoa(if_addr);
	}
      }
    }

    rsp.recv_userdata = this;
    m_rtp_session = rtp_init_stream(&rsp);

    rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
    rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
    if (strncasecmp(m_media_info->proto, "RTP/SVP", strlen("RTP/SVP")) == 0) {
      srtp_init();
    }
  } else {
    m_rtp_session_from_outside = true;
    rtp_set_rtp_callback(m_rtp_session, c_recv_callback, this);
  }
  m_rtp_inited = 1;

}

void CPlayerMedia::create_rtp_byte_stream (uint8_t rtp_pt,
					   uint64_t tps,
					   format_list_t *fmt)
{
  int codec;
  rtp_check_return_t plugin_ret;
  rtp_plugin_t *rtp_plugin;
  int stream_ondemand;
  rtp_plugin = NULL;
  plugin_ret = check_for_rtp_plugins(fmt, rtp_pt, &rtp_plugin, &config);
  
  stream_ondemand = 0;
  if (m_stream_ondemand == 1 &&
      get_range_from_media(m_media_info) != NULL) {
    // m_stream_ondemand == 1 means we're using RTSP, and having a range 
    // in the SDP means that we have an ondemand presentation; otherwise, we
    // want to treat it like a broadcast session, and use the RTCP.
    stream_ondemand = 1;
  }
  if (plugin_ret != RTP_PLUGIN_NO_MATCH) {
    switch (plugin_ret) {
    case RTP_PLUGIN_MATCH:
      player_debug_message("Starting rtp bytestream %s from plugin", 
			   rtp_plugin->name);
      m_rtp_byte_stream = new CPluginRtpByteStream(rtp_plugin,
						 fmt,
						 rtp_pt,
						 stream_ondemand,
						 tps,
						 &m_head,
						 &m_tail,
						 m_rtsp_base_seq_received,
						 m_rtp_base_seq,
						 m_rtsp_base_ts_received,
						 m_rtp_base_ts,
						 m_rtcp_received,
						 m_rtcp_ntp_frac,
						 m_rtcp_ntp_sec,
						 m_rtcp_rtp_ts);
      return;
    case RTP_PLUGIN_MATCH_USE_VIDEO_DEFAULT:
      // just fall through...
      break; 
    case RTP_PLUGIN_MATCH_USE_AUDIO_DEFAULT:
      m_rtp_byte_stream = 
	new CAudioRtpByteStream(rtp_pt, 
				fmt, 
				stream_ondemand,
				tps,
				&m_head,
				&m_tail,
				m_rtsp_base_seq_received,
				m_rtp_base_seq,
				m_rtsp_base_ts_received,
				m_rtp_base_ts,
				m_rtcp_received,
				m_rtcp_ntp_frac,
				m_rtcp_ntp_sec,
				m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	player_debug_message("Starting generic audio byte stream");
	return;
      }

    case RTP_PLUGIN_NO_MATCH:
    default:
      break;
    }
  } else {
    if (is_audio() == false && (rtp_pt == 32)) {
      codec = VIDEO_MPEG12;
      m_rtp_byte_stream = new CMpeg3RtpByteStream(rtp_pt,
						  fmt, 
						  stream_ondemand,
						  tps,
						  &m_head,
						  &m_tail,
						  m_rtsp_base_seq_received,
						  m_rtp_base_seq,
						  m_rtsp_base_ts_received,
						  m_rtp_base_ts,
						  m_rtcp_received,
						  m_rtcp_ntp_frac,
						  m_rtcp_ntp_sec,
						  m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	return;
      }
    } else {
      if (rtp_pt == 14) {
	codec = MPEG4IP_AUDIO_MP3;
      } else if (rtp_pt <= 23) {
	codec = MPEG4IP_AUDIO_GENERIC;
      }  else {
	if (fmt->rtpmap_name == NULL) return;

	codec = lookup_audio_codec_by_name(fmt->rtpmap_name);
	if (codec < 0) {
	  codec = MPEG4IP_AUDIO_NONE; // fall through everything to generic
	}
      }
      switch (codec) {
      case MPEG4IP_AUDIO_MP3: {
	m_rtp_byte_stream = 
	  new CAudioRtpByteStream(rtp_pt, fmt, 
				  stream_ondemand,
				  tps,
				  &m_head,
				  &m_tail,
				  m_rtsp_base_seq_received,
				  m_rtp_base_seq,
				  m_rtsp_base_ts_received,
				  m_rtp_base_ts,
				  m_rtcp_received,
				  m_rtcp_ntp_frac,
				  m_rtcp_ntp_sec,
				  m_rtcp_rtp_ts);
	if (m_rtp_byte_stream != NULL) {
	  m_rtp_byte_stream->set_skip_on_advance(4);
	  player_debug_message("Starting mp3 2250 audio byte stream");
	  return;
	}
      }
	break;
      case MPEG4IP_AUDIO_MP3_ROBUST:
	m_rtp_byte_stream = 
	  new CRfc3119RtpByteStream(rtp_pt, fmt, 
				    stream_ondemand,
				    tps,
				    &m_head,
				    &m_tail,
				    m_rtsp_base_seq_received,
				    m_rtp_base_seq,
				    m_rtsp_base_ts_received,
				    m_rtp_base_ts,
				    m_rtcp_received,
				    m_rtcp_ntp_frac,
				    m_rtcp_ntp_sec,
				    m_rtcp_rtp_ts);
	if (m_rtp_byte_stream != NULL) {
	  player_debug_message("Starting mpa robust byte stream");
	  return;
	}
	break;
      case MPEG4IP_AUDIO_GENERIC:
	m_rtp_byte_stream = 
	  new CAudioRtpByteStream(rtp_pt, fmt, 
				  stream_ondemand,
				  tps,
				  &m_head,
				  &m_tail,
				  m_rtsp_base_seq_received,
				  m_rtp_base_seq,
				  m_rtsp_base_ts_received,
				  m_rtp_base_ts,
				  m_rtcp_received,
				  m_rtcp_ntp_frac,
				  m_rtcp_ntp_sec,
				  m_rtcp_rtp_ts);
	if (m_rtp_byte_stream != NULL) {
	  player_debug_message("Starting generic audio byte stream");
	  return;
	}
      default:
	break;
      }
    }
  }
  if (m_rtp_byte_stream == NULL) 
    m_rtp_byte_stream = new CRtpByteStream(fmt->media->media,
					   fmt, 
					   rtp_pt,
					   stream_ondemand,
					   tps,
					   &m_head,
					   &m_tail,
					   m_rtsp_base_seq_received,
					   m_rtp_base_seq,
					   m_rtsp_base_ts_received,
					   m_rtp_base_ts,
					   m_rtcp_received,
					   m_rtcp_ntp_frac,
					   m_rtcp_ntp_sec,
					   m_rtcp_rtp_ts);
}

void CPlayerMedia::synchronize_rtp_bytestreams (rtcp_sync_t *sync)
{
  if (is_audio()) {
    player_error_message("Attempt to syncronize audio byte stream");
    return;
  }
  if (m_rtp_byte_stream != NULL) 
    m_rtp_byte_stream->synchronize(sync);
}

int CPlayerMedia::srtp_init (void)
{
  if (m_rtp_session == NULL) return -1;

  const char *crypto;

  crypto = find_unparsed_a_value(m_media_info->unparsed_a_lines, 
				 "a=crypto");
  if (crypto == NULL) {
    media_message(LOG_CRIT, "%s: can't find a=crypto line in sdp for srtp",
		  get_name());
    return -1;
  }

  m_srtp_session = srtp_setup_from_sdp(get_name(), m_rtp_session, crypto);
  if (m_srtp_session == NULL)
    media_message(LOG_ERR, "player_media_rtp.srtp_init: srtp_setup failed");
  
  return 0;
}
    
/* end player_media_rtp.cpp */
