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
#include "codec/mp3/mp3_rtp_bytestream.h"
#include "rtp_bytestream_plugin.h"

#include "codec_plugin_private.h"

static void c_recv_callback (struct rtp *session, rtp_event *e)
{
  CPlayerMedia *m = (CPlayerMedia *)rtp_get_userdata(session);
  m->recv_callback(session, e);
}

static int c_rtcp_send_packet (void *ud, uint8_t *buffer, int buflen)
{
  return ((CPlayerMedia *)ud)->rtcp_send_packet(buffer, buflen);
}


int CPlayerMedia::rtp_receive_packet (unsigned char interleaved, 
				      struct rtp_packet *pak, 
				      int len)
{
  int ret;
  if ((interleaved & 1) == 0) {
    ret = rtp_process_recv_data(m_rtp_session, 0, pak, len);
    if (ret < 0) {
      xfree(pak);
    }
  } else {
    uint8_t *pakbuf = (uint8_t *)pak;
    pakbuf += sizeof(rtp_packet_data);
	    
    rtp_process_ctrl(m_rtp_session, pakbuf, len);
    xfree(pak);
    ret = 0;
  }
  return ret;
}

void CPlayerMedia::rtp_periodic (void)
{
  rtp_send_ctrl(m_rtp_session, 
		m_rtp_byte_stream != NULL ? 
		m_rtp_byte_stream->get_last_rtp_timestamp() : 0, 
		NULL);
  rtp_update(m_rtp_session);
  if (m_rtp_byte_stream != NULL) {
    int ret = m_rtp_byte_stream->recv_task(m_decode_thread_waiting);
    if (ret > 0) {
      if (m_rtp_buffering == 0) {
	m_rtp_buffering = 1;
	start_decoding();
      } else {
	bytestream_primed();
      }
    }
    return;
  }
  if (m_head != NULL) {
    /*
     * Make sure that the payload type is the same
     */
    if (m_head->rtp_pak_pt == m_tail->rtp_pak_pt) {
      if (m_rtp_queue_len > 10) { // 10 packets consecutive proto same
	if (determine_payload_type_from_rtp() == FALSE) {
	  clear_rtp_packets(); 
	}
      }
    } else {
      clear_rtp_packets();
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
    m_rtp_byte_stream->flush_rtp_packets();
  }
  m_rtp_buffering = 0;
}

void CPlayerMedia::rtp_end(void)
{
  if (m_rtp_session != NULL) {
    rtp_send_bye(m_rtp_session);
    rtp_done(m_rtp_session);
  }
  m_rtp_session = NULL;
}

int CPlayerMedia::rtcp_send_packet (uint8_t *buffer, int buflen)
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
  connect_desc_t *cptr;
  cptr = get_connect_desc_from_media(m_media_info);


  m_rtp_buffering = 0;
  if (m_stream_ondemand != 0) {
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
  double bw;

  if (find_rtcp_bandwidth_from_media(m_media_info, &bw) < 0) {
    bw = 5000.0;
  } else {
    media_message(LOG_DEBUG, "Using bw from sdp %g", bw);
  }
  m_rtp_session = rtp_init(m_source_addr == NULL ? 
			   cptr->conn_addr : m_source_addr,
			   m_our_port,
			   m_server_port,
			   cptr == NULL ? 1 : cptr->ttl, // need ttl here
			   bw, // rtcp bandwidth ?
			   c_recv_callback,
			   (uint8_t *)this);
  if (m_rtp_session != NULL) {
    rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
    rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
    rtp_start();
  }
  m_rtp_inited = 1;
  
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
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;
      retcode = rtp_recv(m_rtp_session, &timeout, 0);
      //      player_debug_message("rtp_recv return %d", retcode);
      // Run rtp periodic after each packet received or idle time.
      if (m_paused == 0 || m_stream_ondemand != 0)
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
  if (m_paused != 0) {
    if (e->type == RX_RTP) {
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
    m_rtp_byte_stream->recv_callback(session, e);
    return;
  }
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
      add_rtp_packet_to_queue(rpak, &m_head, &m_tail, m_is_video ? "video" : "audio");
      m_rtp_queue_len++;
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
  uint8_t payload_type = m_head->rtp_pak_pt, temp;
  format_list_t *fmt;
  uint64_t tickpersec;

  fmt = m_media_info->fmt;
  while (fmt != NULL) {
    // rtp payloads are all numeric
    temp = atoi(fmt->fmt);
    if (temp == payload_type) {
      m_media_fmt = fmt;
      if (fmt->rtpmap != NULL) {
	tickpersec = fmt->rtpmap->clock_rate;
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
      m_rtp_byte_stream->play((uint64_t)(m_play_start_time * 1000.0));
      m_byte_stream = m_rtp_byte_stream;
      if (!is_video()) {
	m_rtp_byte_stream->set_sync(m_parent);
      } else {
	m_parent->syncronize_rtp_bytestreams(NULL);
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
		payload_type, m_is_video ? "video" : "audio");
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

void CPlayerMedia::rtp_init_tcp (void) 
{
  connect_desc_t *cptr;
  double bw;

  if (find_rtcp_bandwidth_from_media(m_media_info, &bw) < 0) {
    bw = 5000.0;
  } 
  cptr = get_connect_desc_from_media(m_media_info);
  m_rtp_session = rtp_init_extern_net(m_source_addr == NULL ? 
				      cptr->conn_addr : m_source_addr,
				      m_our_port,
				      m_server_port,
				      cptr->ttl,
				      bw, // rtcp bandwidth ?
				      c_recv_callback,
				      c_rtcp_send_packet,
				      (uint8_t *)this);
  rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
  rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
  m_rtp_inited = 1;

}

void CPlayerMedia::create_rtp_byte_stream (uint8_t rtp_pt,
					   uint64_t tps,
					   format_list_t *fmt)
{
  int codec;
  rtp_check_return_t plugin_ret;
  rtp_plugin_t *rtp_plugin;

  rtp_plugin = NULL;
  plugin_ret = check_for_rtp_plugins(fmt, rtp_pt, &rtp_plugin);

  if (plugin_ret != RTP_PLUGIN_NO_MATCH) {
    switch (plugin_ret) {
    case RTP_PLUGIN_MATCH:
      player_debug_message("Starting rtp bytestream %s from plugin", 
			   rtp_plugin->name);
      m_rtp_byte_stream = new CPluginRtpByteStream(rtp_plugin,
						 fmt,
						 rtp_pt,
						 m_stream_ondemand,
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
				m_stream_ondemand,
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
  } else {
    if (is_video() && (rtp_pt == 32)) {
      codec = VIDEO_MPEG12;
      m_rtp_byte_stream = new CMpeg3RtpByteStream(rtp_pt,
						  fmt, 
						m_stream_ondemand,
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
      if (fmt->rtpmap == NULL) return;

      codec = lookup_audio_codec_by_name(fmt->rtpmap->encode_name);
      if (codec < 0) {
	codec = MPEG4IP_AUDIO_NONE; // fall through everything to generic
      }
    }
    switch (codec) {
    case MPEG4IP_AUDIO_MP3: {
      m_rtp_byte_stream = 
	new CAudioRtpByteStream(rtp_pt, fmt, 
				m_stream_ondemand,
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
				m_stream_ondemand,
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
				m_stream_ondemand,
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
  m_rtp_byte_stream = new CRtpByteStream(fmt->media->media,
					 fmt, 
					 rtp_pt,
					 m_stream_ondemand,
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
}

void CPlayerMedia::syncronize_rtp_bytestreams (rtcp_sync_t *sync)
{
  if (!is_video()) {
    player_error_message("Attempt to syncronize audio byte stream");
    return;
  }
  if (m_rtp_byte_stream != NULL) 
    m_rtp_byte_stream->syncronize(sync);
}
/* end player_media_rtp.cpp */
