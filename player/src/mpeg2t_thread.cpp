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
 * mpeg2t_thread.c
 */
#include "mpeg2t_private.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>
#include <rtp/net_udp.h>
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "our_config_file.h"
#include "media_utils.h"
#include "codec_plugin_private.h"
#include "mpeg2t_bytestream.h"
#include "player_sdp.h"
#include "ip_port.h"
#include "player_rtsp.h"

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mpeg2t_message, "mpeg2t")
#else
#define mpeg2t_message(loglevel, fmt...) message(loglevel, "mpeg2t", fmt)
#endif

/*
 * mpeg2t_decode_buffer - this is called from the mpeg2t thread.
 */
static uint32_t mpeg2t_decode_buffer (mpeg2t_client_t *info, 
				  uint8_t *buffer, 
				  uint32_t buflen)
{
  mpeg2t_pid_t *pidptr;
  uint32_t buflen_used;

  do {
    pidptr = mpeg2t_process_buffer(info->decoder,
				   buffer, 
				   buflen,
				   &buflen_used);
    if (pidptr != NULL) {
      // have a return - process it
      switch (pidptr->pak_type) {
      case MPEG2T_PAS_PAK:
	break;
      case MPEG2T_PROG_MAP_PAK:
	if (info->pam_recvd_sem != NULL) {
	  SDL_SemPost(info->pam_recvd_sem);
	}
	break;
      case MPEG2T_ES_PAK:
	mpeg2t_stream_t *sptr;
	CPlayerMedia *mptr;
	mpeg2t_es_t *es_pid;
	es_pid = (mpeg2t_es_t *)pidptr;
	sptr = (mpeg2t_stream_t *)mpeg2t_get_userdata(&es_pid->pid);
	if (sptr != NULL) {
	  if (sptr->m_buffering == 0) {
	    // not buffering - if we're saving frames, indicate that to
	    // the bytestream.
	    if (es_pid->save_frames != 0) {
	      mptr = sptr->m_mptr;
	      mptr->bytestream_primed();
	    }
	  } else {
	    // we're buffering.
	    if (sptr->m_have_info == 0) {
	      if (es_pid->info_loaded == 0) {
		// just dump the frame - record the psts
 		mpeg2t_frame_t *fptr;
		do {
		  fptr = mpeg2t_get_es_list_head(es_pid);
		  if (fptr != NULL) {
		    if (fptr->have_ps_ts) {
		      sptr->m_last_psts = fptr->ps_ts;
		      sptr->m_frames_since_last_psts = 0;
		    } else 
		      sptr->m_frames_since_last_psts++;
		    mpeg2t_free_frame(fptr);
		  }
		} while (fptr != NULL);
	      } else {
		// just got the info in pid
		sptr->m_have_info = 1;
		mpeg2t_message(LOG_DEBUG, "%s Info is loading - starting buffering", 
			       sptr->m_is_video ? "video" : "audio");
		if (es_pid->list->have_ps_ts == 0) {
		  mpeg2t_message(LOG_ERR, "have psts is 0 when info loaded");
		}
	      }
	    } else {
	      uint32_t msec;
	      if (sptr->m_is_video) {
		// use the video values for buffering
		double msec_in_list;
		msec_in_list = es_pid->frames_in_list;
		msec_in_list *= 1000.0;
		msec_in_list /= es_pid->frame_rate;
		msec = (int)msec_in_list;
	      } else {
		// use the audio values for buffering
		msec = es_pid->frames_in_list;
		msec *= 1000;
		msec *= es_pid->sample_per_frame;
		msec /= es_pid->sample_freq;
	      }
#if 0
	      mpeg2t_message(LOG_DEBUG, "%s buffer %d", 
			     sptr->m_is_video ? "video" : "audio", msec);
#endif
	      if (msec >= config.get_config_value(CONFIG_RTP_BUFFER_TIME_MSEC)) {
		// yipee - done buffering..
		sptr->m_buffering = 0;
		sptr->m_mptr->start_decoding();
		mpeg2t_message(LOG_DEBUG, "%s done buffering %d",
			       sptr->m_is_video ? "video" : "audio", msec);
	      }
	    }
	  }
	}
	break;
      }
    }
    buffer += buflen_used;
    buflen -= buflen_used;
  } while (buflen >= 188);
  if (buflen > 0) {
    //mpeg2t_message(LOG_DEBUG, "left %d at end", buflen);
  }
  return (buflen);
}

static void mpeg2t_rtp_callback (struct rtp *session, rtp_event *e)
{
  mpeg2t_client_t *info;

  if (e->type == RX_RTP) {
    rtp_packet *rpak;
    rpak = (rtp_packet *)e->data;
    info = (mpeg2t_client_t *)rtp_get_userdata(session);

    info->rtp_seq = rpak->rtp_pak_seq + 1;

    // need to handle "left over" here...
    mpeg2t_decode_buffer((mpeg2t_client_t *)rtp_get_userdata(session), 
			 rpak->rtp_data, 
			 rpak->rtp_data_len);
    xfree(rpak);
  }
}

/*
 * mpeg2t_thread_start_cmd()
 * Handle the start command - create the socket
 */
static int mpeg2t_thread_start_cmd (mpeg2t_client_t *info)
{
  int ret;
  mpeg2t_message(LOG_DEBUG, "Processing start command");
#ifdef _WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
 
  wVersionRequested = MAKEWORD( 2, 0 );
 
  ret = WSAStartup( wVersionRequested, &wsaData );
  if ( ret != 0 ) {
    /* Tell the user that we couldn't find a usable */
    /* WinSock DLL.*/
    return (ret);
  }
#else
  ret = 0;
#endif
  if (info->useRTP == FALSE) {
    if (config.get_config_string(CONFIG_MULTICAST_RX_IF) != NULL) {
      struct in_addr if_addr;

      // configure out the specified interface
      if (getIpAddressFromInterface(config.get_config_string(CONFIG_MULTICAST_RX_IF),
				    &if_addr) >= 0) {
	info->udp = udp_init_if(info->address,
				inet_ntoa(if_addr),
				info->rx_port,
				info->tx_port,
				info->ttl);
      }
    }
    if (info->udp == NULL) {
      info->udp = udp_init(info->address, info->rx_port,
			   info->tx_port, info->ttl);
    }
    if (info->udp == NULL) 
      return -1;
    info->data_socket = udp_fd(info->udp);
  } else {
    info->rtp_session = NULL;
    if (config.get_config_string(CONFIG_MULTICAST_RX_IF) != NULL) {
      struct in_addr if_addr;

      if (getIpAddressFromInterface(config.get_config_string(CONFIG_MULTICAST_RX_IF),
				    &if_addr) >= 0) {
	info->rtp_session = rtp_init_if(info->address,
					inet_ntoa(if_addr),
					info->rx_port,
					info->tx_port,
					info->ttl,
					info->rtcp_bw,
					mpeg2t_rtp_callback,
					(uint8_t *)info,
					0);
      }
    }
    if (info->rtp_session == NULL) {
      info->rtp_session = rtp_init(info->address,
				   info->rx_port,
				   info->tx_port,
				   info->ttl,
				   info->rtcp_bw,
				   mpeg2t_rtp_callback,
				   (uint8_t *)info);
    }
    if (info->rtp_session == NULL) {
      return -1;
    }
    rtp_set_option(info->rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
    rtp_set_option(info->rtp_session, RTP_OPT_PROMISC, TRUE);
    info->data_socket = udp_fd(get_rtp_data_socket(info->rtp_session));
    info->rtcp_socket = udp_fd(get_rtp_rtcp_socket(info->rtp_session));
  }
  return (ret);
}

/*
 * mpeg2t_thread() - mpeg2t thread handler - receives and
 * processes all data
 */
int mpeg2t_thread (void *data)
{
  mpeg2t_client_t *info = (mpeg2t_client_t *)data;
  int ret;
  int continue_thread;
  uint8_t buffer[17000];
  uint32_t buflen_left = 0;
  uint32_t buflen;
  int consec_timeout = 0;

  
  continue_thread = 0;
  //mpeg2t_message(LOG_DEBUG, "thread started");
  mpeg2t_thread_init_thread_info(info);
  while (continue_thread == 0) {
    //mpeg2t_message(LOG_DEBUG, "thread waiting");
    ret = mpeg2t_thread_wait_for_event(info);
    if (ret <= 0) {
      if (ret < 0) {
	//mpeg2t_message(LOG_ERR, "MPEG2T loop error %d errno %d", ret, errno);
      } else {
	consec_timeout++;
	if (info->m_have_rtsp && consec_timeout > 10) {
	  // this could be better... Maybe detect the final time vs the
	  // start time.
	  mpeg2t_stream_t *sptr;
	  sptr = info->stream;
	  while (sptr != NULL) {
	    sptr->m_have_eof = 1;
	    sptr = sptr->next_stream;
	  }
	}
      }
      continue;
    }
    consec_timeout = 0;
    /*
     * See if the communications socket for IPC has any data
     */
    //mpeg2t_message(LOG_DEBUG, "Thread checking control");
    ret = mpeg2t_thread_has_control_message(info);
    
    if (ret) {
      mpeg2t_msg_type_t msg_type;
      int read;
      /*
       * Yes - read the message type.
       */
      read = mpeg2t_thread_get_control_message(info, &msg_type);
      if (read == sizeof(msg_type)) {
	// received message
	//mpeg2t_message(LOG_DEBUG, "Comm socket msg %d", msg_type);
	switch (msg_type) {
	case MPEG2T_MSG_QUIT:
	  continue_thread = 1;
	  break;
	case MPEG2T_MSG_START:
	  ret = mpeg2t_thread_start_cmd(info);
	  mpeg2t_thread_ipc_respond(info, ret);
	  break;
	default:
	  mpeg2t_message(LOG_ERR, "Unknown message %d received", msg_type);
	  break;
	}
      }
    }

    if (info->useRTP) {
      if (mpeg2t_thread_has_receive_data(info)) {
	rtp_recv_data(info->rtp_session, 0);
      }
      if (mpeg2t_thread_has_rtcp_data(info)) {
	buflen = udp_recv(get_rtp_rtcp_socket(info->rtp_session), 
			  buffer, sizeof(buffer));
	rtp_process_ctrl(info->rtp_session, buffer, buflen);
      }
      rtp_send_ctrl(info->rtp_session, 0, NULL);
      rtp_update(info->rtp_session);
    } else {
      if (mpeg2t_thread_has_receive_data(info)) {
	if (info->udp != NULL) {
	  //mpeg2t_message(LOG_DEBUG, "receiving udp data");
	  // we may have leftover data in the buffer if some people send
	  // non-transport stream packet aligned udp packets.
	  buflen = udp_recv(info->udp, buffer + buflen_left, sizeof(buffer) - buflen_left);
	  buflen += buflen_left;
	  buflen_left = mpeg2t_decode_buffer(info, buffer, buflen);
	  if (buflen_left > 0) {
	    memmove(buffer, buffer + buflen - buflen_left, buflen_left);
	  }
	}
      }
    }
  } // end while continue_thread

  SDL_Delay(10);
  /*
   * Okay - we've gotten a quit - we're done
   */
  if (info->useRTP) {
    rtp_send_bye(info->rtp_session);
    rtp_done(info->rtp_session);
    info->rtp_session = NULL;
  } else {
    if (info->udp != NULL) {
      udp_exit(info->udp);
      info->udp = NULL;
    }
  }

  mpeg2t_close_thread(info);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}


/*
 * mpeg2t_create_client
 * create threaded mpeg2t session
 */
static mpeg2t_client_t *mpeg2t_create_client (const char *address,
					      in_port_t rx_port,
					      in_port_t tx_port,
					      int use_rtp,
					      double rtcp_bw, 
					      int ttl,
					      char *errmsg,
					      uint32_t errmsg_len,
					      rtsp_client_t *client = NULL,
					      char *rtsp_url = NULL)
{
  mpeg2t_client_t *info;
  int ret;
  mpeg2t_msg_type_t msg;
  mpeg2t_msg_resp_t resp;
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  int err;

  info = MALLOC_STRUCTURE(mpeg2t_client_t);
  if (info == NULL) return (NULL);
  memset(info, 0, sizeof(mpeg2t_client_t));
  info->m_rtsp_client = client;
  if (client != NULL) {
    info->m_have_rtsp = 1;
  } else {
    info->m_have_rtsp = 0;
  }

  info->m_rtsp_url = rtsp_url;
  info->address = strdup(address);
  info->rx_port = rx_port;
  info->tx_port = tx_port;
  info->useRTP = use_rtp;
  info->rtcp_bw = rtcp_bw;
  info->ttl = ttl;
  info->recv_timeout = 100;
  info->decoder = create_mpeg2_transport();
  if (info->decoder == NULL) {
    mpeg2t_delete_client(info);
    return (NULL);
  }
  info->decoder->save_frames_at_start = 1;
  info->pam_recvd_sem = SDL_CreateSemaphore(0);
  info->msg_mutex = SDL_CreateMutex();
  // start the thread, which starts the data being received, so we
  // can look at the program maps
  if (mpeg2t_create_thread(info) != 0) {
    mpeg2t_delete_client(info);
    return (NULL);
  }
  SDL_Delay(100);
  msg = MPEG2T_MSG_START;
  ret = mpeg2t_thread_ipc_send_wait(info,
				  (unsigned char *)&msg,
				  sizeof(msg),
				  &resp);
  if (ret < 0 || resp < 0) {
    mpeg2t_delete_client(info);
    snprintf(errmsg, errmsg_len, "Couldn't create client - error %d", resp);
    return NULL;
  }
  if (info->m_rtsp_client) {
    // issue play, so we've got the thread ready to receive data, 
    // and we can look for the program maps
    memset(&cmd, 0, sizeof(rtsp_command_t));
    decode = NULL;
    cmd.scale=1.0;
    cmd.range="0-";
    err = rtsp_send_aggregate_play(info->m_rtsp_client, 
				   info->m_rtsp_url, 
				   &cmd, 
				   &decode);
    if (err != RTSP_RESPONSE_GOOD) {
      snprintf(errmsg, errmsg_len, "RTSP play error %s %s", 
	       decode->retcode, decode->retresp);
      free_decode_response(decode);
      memset(&cmd, 0, sizeof(rtsp_command_t));
      decode = NULL;
      rtsp_send_aggregate_teardown(info->m_rtsp_client, 
				   info->m_rtsp_url, 
				   &cmd, 
				   &decode);
      free_rtsp_client(info->m_rtsp_client);
      mpeg2t_delete_client(info);
      return NULL;
    }
    free_decode_response(decode);
  }
  
  // Wait until we get the program maps (should probably be PMAP_WAIT), 
  // so we can determine audio/video
  int max = config.get_config_value(CONFIG_MPEG2T_PAM_WAIT_SECS);
  do {
    ret = SDL_SemWaitTimeout(info->pam_recvd_sem, 1000);
    if (ret == SDL_MUTEX_TIMEDOUT) {
      max--;
      mpeg2t_message(LOG_DEBUG, "timeout - still left %d", max);
    }
  } while (ret == SDL_MUTEX_TIMEDOUT && max >= 0);

  if (ret == SDL_MUTEX_TIMEDOUT) {
    if (info->decoder->program_maps_recvd != 0) {
      mpeg2t_message(LOG_INFO, "Program count received %d doesn't match program count %d", 
		     info->decoder->program_maps_recvd, 
		     info->decoder->program_count);
    } else {
      if (info->m_rtsp_client) {
	memset(&cmd, 0, sizeof(rtsp_command_t));
	decode = NULL;
	rtsp_send_aggregate_teardown(info->m_rtsp_client, 
				     info->m_rtsp_url, 
				     &cmd, 
				     &decode);
      free_rtsp_client(info->m_rtsp_client);
      }
      snprintf(errmsg, errmsg_len, "Did not receive Transport Stream Program Map in %d seconds", config.get_config_value(CONFIG_MPEG2T_PAM_WAIT_SECS));
      mpeg2t_delete_client(info);
      return NULL;
    }
  }
  return (info);
}

void mpeg2t_delete_client (mpeg2t_client_t *info)
{
  mpeg2t_stream_t *p;

  mpeg2t_thread_close(info);

  CHECK_AND_FREE(info->address);
  delete_mpeg2t_transport(info->decoder);
  info->decoder = NULL;
  SDL_DestroyMutex(info->msg_mutex);
  SDL_DestroySemaphore(info->pam_recvd_sem);
  p = info->stream;
  while (p != NULL) {
    info->stream = p->next_stream;
    free(p);
    p = info->stream;
  }
  free(info);
}

static void close_mpeg2t_client (void *data)
{
  mpeg2t_delete_client((mpeg2t_client_t *)data);
}

static int mpeg2t_create_video(mpeg2t_client_t *info,
			       CPlayerSession *psptr, 
			       video_query_t *vq, 
			       int video_offset,
			       char *errmsg, 
			       uint32_t errlen,
			       int &sdesc)
{
  int ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int created = 0;

  for (ix = 0; ix < video_offset; ix++) {
    mpeg2t_pid_t *pidptr;
    mpeg2t_es_t *es_pid;
    pidptr = mpeg2t_lookup_pid(info->decoder,vq[ix].track_id);
    if (pidptr->pak_type != MPEG2T_ES_PAK) {
      mpeg2t_message(LOG_CRIT, "mpeg2t video type is not es pak");
      exit(1);
    }
    es_pid = (mpeg2t_es_t *)pidptr;
    if (vq[ix].enabled != 0 && created == 0) {
      created = 1;
      mptr = new CPlayerMedia(psptr);
      if (mptr == NULL) {
	return (-1);
      }
      video_info_t *vinfo;
      vinfo = MALLOC_STRUCTURE(video_info_t);
      vinfo->height = vq[ix].h;
      vinfo->width = vq[ix].w;
      plugin = check_for_video_codec("MPEG2 TRANSPORT",
				     NULL,
				     vq[ix].type,
				     vq[ix].profile,
				     vq[ix].config, 
				     vq[ix].config_len);

      int ret = mptr->create_video_plugin(plugin, 
					  "MPEG2 TRANSPORT",
					  vq[ix].type,
					  vq[ix].profile,
					  NULL, // sdp info
					  vinfo, // video info
					  vq[ix].config,
					  vq[ix].config_len);

      if (ret < 0) {
	mpeg2t_message(LOG_ERR, "Failed to create plugin data");
	snprintf(errmsg, errlen, "Failed to start plugin");
	delete mptr;
	return -1;
      }
      mpeg2t_stream_t *stream;
      stream = MALLOC_STRUCTURE(mpeg2t_stream_t);
      stream->m_parent = info;
      stream->m_mptr = mptr;
      stream->m_is_video = 1;
      stream->m_buffering = 1;
      stream->m_frames_since_last_psts = 0;
      stream->m_last_psts = 0;
      stream->m_have_info = 0;
      stream->m_have_eof = 0;
      stream->next_stream = info->stream;
      info->stream = stream;
      mpeg2t_set_userdata(&es_pid->pid, stream);

      CMpeg2tVideoByteStream *vbyte;
      vbyte = new CMpeg2tVideoByteStream(es_pid, info->m_have_rtsp);
      if (vbyte == NULL) {
	mpeg2t_message(LOG_CRIT, "failed to create byte stream");
	delete mptr;
	free(stream);
	return (-1);
      }
      ret = mptr->create(vbyte, TRUE, errmsg, errlen, 1);
      if (ret != 0) {
	mpeg2t_message(LOG_CRIT, "failed to create from file");
	free(stream);
	return (-1);
      }
      if (es_pid->info_loaded) {
	char buffer[80];
	if (mpeg2t_write_stream_info(es_pid, buffer, 80) >= 0) {
	  psptr->set_session_desc(sdesc, buffer);
	  sdesc++;
	}
      }
      mpeg2t_start_saving_frames(es_pid);
    }  else {
      mpeg2t_stop_saving_frames(es_pid);
    }
  }
  return created;
}

static int mpeg2t_create_audio (mpeg2t_client_t *info,
				CPlayerSession *psptr, 
				audio_query_t *aq, 
				int audio_offset,
				char *errmsg, 
				uint32_t errlen,
				int &sdesc)
{
  int ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int created = 0;

  for (ix = 0; ix < audio_offset; ix++) {
    mpeg2t_pid_t *pidptr;
    mpeg2t_es_t *es_pid;
    pidptr = mpeg2t_lookup_pid(info->decoder,aq[ix].track_id);
    if (pidptr->pak_type != MPEG2T_ES_PAK) {
      mpeg2t_message(LOG_CRIT, "mpeg2t video type is not es pak");
      exit(1);
    }
    es_pid = (mpeg2t_es_t *)pidptr;
    if (aq[ix].enabled != 0 && created == 0) {
      mptr = new CPlayerMedia(psptr);
      if (mptr == NULL) {
	return (-1);
      }
      audio_info_t *ainfo;
      ainfo = MALLOC_STRUCTURE(audio_info_t);
      ainfo->freq = aq[ix].sampling_freq;
      ainfo->chans = aq[ix].chans;
      ainfo->bitspersample = 0;
      plugin = check_for_audio_codec("MPEG2 TRANSPORT",
				     NULL,
				     aq[ix].type,
				     aq[ix].profile,
				     aq[ix].config, 
				     aq[ix].config_len);

      int ret = mptr->create_audio_plugin(plugin, 
					  "MPEG2 TRANSPORT",
					  aq[ix].type,
					  aq[ix].profile,
					  NULL, // sdp info
					  ainfo, // video info
					  aq[ix].config,
					  aq[ix].config_len);

      if (ret < 0) {
	mpeg2t_message(LOG_ERR, "Failed to create plugin data");
	snprintf(errmsg, errlen, "Failed to start plugin");
	delete mptr;
	return -1;
      }
      mpeg2t_stream_t *stream;
      stream = MALLOC_STRUCTURE(mpeg2t_stream_t);
      stream->m_parent = info;
      stream->m_mptr = mptr;
      stream->m_is_video = 0;
      stream->m_buffering = 1;
      stream->m_frames_since_last_psts = 0;
      stream->m_last_psts = 0;
      stream->m_have_info = 0;
      stream->next_stream = info->stream;
      info->stream = stream;

      mpeg2t_set_userdata(&es_pid->pid, stream);

      created = 1;
      CMpeg2tAudioByteStream *abyte;
      abyte = new CMpeg2tAudioByteStream(es_pid, info->m_have_rtsp);
      if (abyte == NULL) {
	mpeg2t_message(LOG_CRIT, "failed to create byte stream");
	delete mptr;
	free(stream);
	return (-1);
      }
      ret = mptr->create(abyte, FALSE, errmsg, errlen, 1);
      if (ret != 0) {
	mpeg2t_message(LOG_CRIT, "failed to create from file");
	free(stream);
	return (-1);
      }
      if (es_pid->info_loaded) {
	char buffer[80];
	if (mpeg2t_write_stream_info(es_pid, buffer, 80) >= 0) {
	  psptr->set_session_desc(sdesc, buffer);
	  sdesc++;
	}
      }
      mpeg2t_start_saving_frames(es_pid);
    } else {
      mpeg2t_stop_saving_frames(es_pid);
    }
  }
  return created;
}

mpeg2t_client_t *mpeg2t_start_rtsp (CPlayerSession *psptr, 
				    const char *orig_name, 
				    char *errmsg, 
				    uint32_t errlen)
{
  char *rtsp_url;
  rtsp_client_t *rptr;
  int err;
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  int rtsp_resp;
  int have_duration = 0;
  uint64_t duration = 0;
  mpeg2t_client_t *mp2t;

  rtsp_url = strdup(orig_name + 2);
  rtsp_url[0] = 'r';
  rtsp_url[1] = 't';
  rtsp_url[2] = 's';
  rtsp_url[3] = 'p';
  // That's to change mpeg2t:// to rtsp://
  rptr = rtsp_create_client(rtsp_url, &err);
  if (rptr == NULL) {
    snprintf(errmsg, errlen, "Couldn't create rtsp client %d", err);
    free(rtsp_url);
    return NULL;
  }
  memset(&cmd, 0, sizeof(rtsp_command_t));
  // don't set accept
  rtsp_resp = rtsp_send_describe(rptr, &cmd, &decode);
  if (rtsp_resp == RTSP_RESPONSE_GOOD) {
    // good response - see what they returned.
    if (decode->content_type != NULL) {
      if (strncasecmp(decode->content_type, 
		      "application/sdp", 
		      strlen("application/sdp")) == 0) {
	free_decode_response(decode);
	free(rtsp_url);
	snprintf(errmsg, errlen, "Returned content type is SDP - please use rtsp://");
	return NULL;
      } else if (strncasecmp(decode->content_type, 
			     "application/x-rtsp-mh",
			     strlen("application/x-rtsp-mh")) == 0) {
	// 
	if (decode->body) {
	  const char *dptr = strcasestr(decode->body, "Duration");
	  if (dptr != NULL) {
	    dptr += strlen("duration");
	    ADV_SPACE(dptr);
	    if (*dptr == '=') {
	      dptr++;
	      ADV_SPACE(dptr);
	      char *endptr;
	      duration = strtoull(dptr, &endptr, 10);
	      duration = (duration + 999) / M_64; // convert usec to msec
	      have_duration = endptr != dptr;
	    }
	  }
	}
      }
    } 
  } else {
    // not good, but try anyway for setup.
  }
  if (decode != NULL) free_decode_response(decode);
  // we're done with describe - we might have something, we might not 
  // lets go with a SETUP.
  memset(&cmd, 0, sizeof(rtsp_command_t));
  decode = NULL;
  char buffer[2048];
  char *ouraddr = get_host_ip_address();
  C2ConsecIpPort *port = new C2ConsecIpPort(psptr->get_unused_ip_port_ptr());

  snprintf(buffer, sizeof(buffer), "RAW/RAW/UDP;unicast;destination=%s;client_port=%u",
	   ouraddr, port->first_port());
  cmd.transport = buffer;
  rtsp_session_t *session;

  err = rtsp_send_setup(rptr, rtsp_url, &cmd, &session, &decode, 1);
  free(ouraddr);

  if (err != RTSP_RESPONSE_GOOD) {
    if (decode != NULL) {
      snprintf(errmsg, errlen, "RTSP error %s %s",
	       decode->retcode, decode->retresp);
    } else {
      snprintf(errmsg, errlen, "RTSP setup error %d", err);
    }
    free_rtsp_client(rptr);
    free_decode_response(decode);
    free(rtsp_url);
    return NULL;
  }
  // we have a good setup - we'll need to parse the transport for the 
  // ports to set up, then create the client.
  rtsp_transport_parse_t parse;
  memset(&parse, 0, sizeof(parse));
  parse.client_port = port->first_port();
  delete port;
  err = process_rtsp_transport(&parse, decode->transport, "RAW/RAW/UDP");
  if (err < 0) {
    snprintf(errmsg, errlen, "error in rtsp transport \"%s\"", decode->transport);
    free_rtsp_client(rptr);
    free_decode_response(decode);
    free(rtsp_url);
    return NULL;
  }
  free_decode_response(decode);

  
  mp2t = mpeg2t_create_client(parse.source,
			      parse.client_port,
			      parse.server_port, 
			      0, 
			      0.0,
			      0, 
			      errmsg, 
			      errlen,
			      rptr,
			      rtsp_url);

  // now, we'll start playing, and capturingr until we get enough to set
  // up the decoders - we'll save everything for a bit
  if (mp2t != NULL) {
    mp2t->m_have_end_time = have_duration;
    mp2t->m_end_time = duration;
  }
  return mp2t;
}

int create_mpeg2t_session (CPlayerSession *psptr,
			   const char *orig_name, 
			   session_desc_t *sdp,
			   char *errmsg, 
			   uint32_t errlen, 
			   int have_audio_driver,
			   control_callback_vft_t *cc_vft)
{
  const char *colon, *slash, *name;
  char *addr, *port;
  uint32_t addrlen, portlen;
  mpeg2t_client_t *mp2t;
  in_port_t rxport;
  if (orig_name != NULL) {
    name = orig_name + strlen("mpeg2t://");
    colon = strchr(name, ':');
    slash = strchr(name, '/');

    if (slash == NULL) {
      slash = name + strlen(name);
      if (colon == NULL || slash == NULL || colon > slash) {
	snprintf(errmsg, errlen, "Misformed mpeg2 url %s", orig_name);
	return -1;
      }
  
      addrlen = colon - name;
      portlen = slash - colon - 1;
      addr = (char *)malloc(1 + addrlen);
      port = (char *)malloc(1 + portlen);
      memcpy(addr, name, addrlen);
      addr[addrlen] = '\0';
      memcpy(port, colon + 1, portlen);
      port[portlen] = '\0';
      char *eport;
      rxport = strtoul(port, &eport, 10);
      if (eport == NULL || *eport != '\0') {
	snprintf(errmsg, errlen, "Illegal port number in url %s", orig_name);
	free(addr);
	free(port);
	return -1;
      }
      free(port);
  

      mp2t = mpeg2t_create_client(addr, rxport, 0, 0, 0.0, 0, errmsg, errlen);
      free(addr);
    } else {
       // we have a rtsp like url.
      mp2t = mpeg2t_start_rtsp(psptr, orig_name, errmsg, errlen);
      if (mp2t == NULL)
	return -1;
    }
   } else if (sdp != NULL) {
    // from SDP
    connect_desc_t *cptr;
    double bw;
    cptr = get_connect_desc_from_media(sdp->media);
    if (find_rtcp_bandwidth_from_media(sdp->media, &bw) < 0) {
      bw = 5000.0;
    }
    mp2t = mpeg2t_create_client(cptr->conn_addr,
				sdp->media->port,
				0,
				1, // use rtp
				bw,
				cptr->ttl,
				errmsg, 
				errlen);
  } else {
    return -1;
  }

  if (mp2t == NULL) {
    return -1;
  }
  psptr->set_media_close_callback(close_mpeg2t_client, mp2t);
  // Okay - we need to gather together information about pids and
  // lists, then call the audio/video query vectors.
  int audio_count, video_count;
  int audio_info_count, video_info_count;
  int passes;
  mpeg2t_pid_t *pid_ptr;
  mpeg2t_es_t *es_pid;
  mp2t->decoder->save_frames_at_start = 0;
  pid_ptr = &mp2t->decoder->pas.pid;
  audio_count = video_count = 0;
  audio_info_count = video_info_count = 0;
  passes = 0;

  do {
  SDL_LockMutex(mp2t->decoder->pid_mutex);
  while (pid_ptr != NULL) {
    switch (pid_ptr->pak_type) {
    case MPEG2T_PAS_PAK:
    case MPEG2T_PROG_MAP_PAK:
      break;
    case MPEG2T_ES_PAK:
      es_pid = (mpeg2t_es_t *)pid_ptr;
      switch (es_pid->stream_type) {
      case MPEG2T_ST_MPEG_VIDEO:
      case MPEG2T_ST_MPEG4_VIDEO:
      case MPEG2T_ST_11172_VIDEO:
	video_count++;
	if (es_pid->info_loaded) video_info_count++;
	break;
      case MPEG2T_ST_11172_AUDIO:
      case MPEG2T_ST_MPEG_AUDIO:
	audio_count++;
	if (es_pid->info_loaded) audio_info_count++;
	break;
      case MPEG2T_ST_MPEG_AUDIO_6_A:
      case MPEG2T_ST_MPEG_AUDIO_6_B:
      case MPEG2T_ST_MPEG_AUDIO_6_C:
      case MPEG2T_ST_MPEG_AUDIO_6_D:
      case MPEG2T_ST_MPEG2_AAC:
      default:
	mpeg2t_message(LOG_INFO, "PID %x - Unknown/unused stream type %d",
		       pid_ptr->pid, es_pid->stream_type);
	break;
      }
      break;
    }
    pid_ptr = pid_ptr->next_pid;
  }
  SDL_UnlockMutex(mp2t->decoder->pid_mutex);
  passes++;
  if (audio_count != audio_info_count ||
      video_info_count != video_count) {
    SDL_Delay(1 * 1000);
  }
  } while (audio_info_count != audio_count && video_info_count != video_info_count);
  video_query_t *vq;
  audio_query_t *aq;
  if (video_count > 0) {
    vq = (video_query_t *)malloc(sizeof(video_query_t) * video_count);
    memset(vq, 0, sizeof(video_query_t) * video_count);
  } else {
    vq = NULL;
  }
  if (audio_count > 0) {
    aq = (audio_query_t *)malloc(sizeof(audio_query_t) * audio_count);
    memset(aq, 0, sizeof(audio_query_t) * audio_count);
  } else {
    aq = NULL;
  }

  int vid_cnt = 0, aud_cnt = 0;
  codec_plugin_t *plugin;

  pid_ptr = &mp2t->decoder->pas.pid;
  SDL_LockMutex(mp2t->decoder->pid_mutex);
  while (pid_ptr != NULL) {
    switch (pid_ptr->pak_type) {
    case MPEG2T_PAS_PAK:
    case MPEG2T_PROG_MAP_PAK:
      break;
    case MPEG2T_ES_PAK:
      es_pid = (mpeg2t_es_t *)pid_ptr;
      switch (es_pid->stream_type) {
      case MPEG2T_ST_11172_VIDEO:
      case MPEG2T_ST_MPEG_VIDEO:
      case MPEG2T_ST_MPEG4_VIDEO:
	if (vid_cnt < video_count) {
	  plugin = check_for_video_codec("MPEG2 TRANSPORT",
					 NULL,
					 es_pid->stream_type,
					 -1,
					 es_pid->es_data,
					 es_pid->es_info_len);
	  if (plugin == NULL) {
	    snprintf(errmsg, errlen, 
		     "Can't find plugin for stream type %d",
		     es_pid->stream_type);
	    mpeg2t_message(LOG_ERR, errmsg);
	  } else {
	    vq[vid_cnt].track_id = pid_ptr->pid;
	    vq[vid_cnt].compressor = "MPEG2 TRANSPORT";
	    vq[vid_cnt].type = es_pid->stream_type;
	    vq[vid_cnt].profile = -1;
	    vq[vid_cnt].fptr = NULL;
	    if (es_pid->info_loaded != 0) {
	      vq[vid_cnt].h = es_pid->h;
	      vq[vid_cnt].w = es_pid->w;
	      vq[vid_cnt].frame_rate = es_pid->frame_rate;
	      mpeg2t_message(LOG_DEBUG, "video stream h %d w %d fr %g bitr %g", 
			     es_pid->h, es_pid->w, es_pid->frame_rate, 
			     es_pid->bitrate);
	    } else {
	      vq[vid_cnt].h = -1;
	      vq[vid_cnt].w = -1;
	      vq[vid_cnt].frame_rate = 0.0;
	    }
	    vq[vid_cnt].config = es_pid->es_data;
	    vq[vid_cnt].config_len = es_pid->es_info_len;
	    vq[vid_cnt].enabled = 0;
	    vq[vid_cnt].reference = NULL;
	    vid_cnt++;
	  }
	}
	break;
      case MPEG2T_ST_MPEG_AUDIO:
      case MPEG2T_ST_11172_AUDIO:
	if (aud_cnt < audio_count) {
	  plugin = check_for_audio_codec("MPEG2 TRANSPORT",
					 NULL,
					 es_pid->stream_type,
					 -1,
					 es_pid->es_data,
					 es_pid->es_info_len);
	  if (plugin == NULL) {
	    snprintf(errmsg, errlen, 
		     "Can't find plugin for stream type %d",
		     es_pid->stream_type);
	    mpeg2t_message(LOG_ERR, errmsg);
	  } else {
	    aq[aud_cnt].track_id = pid_ptr->pid;
	    aq[aud_cnt].compressor = "MPEG2 TRANSPORT";
	    aq[aud_cnt].type = es_pid->stream_type;
	    aq[aud_cnt].profile = -1;
	    aq[aud_cnt].fptr = NULL;
	    aq[aud_cnt].config = es_pid->es_data;
	    aq[aud_cnt].config_len = es_pid->es_info_len;
	    if (es_pid->info_loaded != 0) {
	      aq[aud_cnt].chans = es_pid->audio_chans;
	      aq[aud_cnt].sampling_freq = es_pid->sample_freq;
	      mpeg2t_message(LOG_DEBUG, "audio stream chans %d sf %d bitrate %g", 
			     es_pid->audio_chans, es_pid->sample_freq, es_pid->bitrate / 1000.0);
	    } else {
	      aq[aud_cnt].chans = -1;
	      aq[aud_cnt].sampling_freq = -1;
	    }
	    aq[aud_cnt].enabled = 0;
	    aq[aud_cnt].reference = NULL;
	    aud_cnt++;
	  }
	}
	break;
      case MPEG2T_ST_MPEG_AUDIO_6_A:
      case MPEG2T_ST_MPEG_AUDIO_6_B:
      case MPEG2T_ST_MPEG_AUDIO_6_C:
      case MPEG2T_ST_MPEG_AUDIO_6_D:
      case MPEG2T_ST_MPEG2_AAC:
      default:
	mpeg2t_message(LOG_INFO, "PID %x - Unknown/unused stream type %d",
		       es_pid->stream_type, pid_ptr->pid);
	break;
      }
      break;
    }
    pid_ptr = pid_ptr->next_pid;
  }
  SDL_UnlockMutex(mp2t->decoder->pid_mutex);
  if (cc_vft && cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_count, vq, audio_count, aq);
  } else {
    if (video_count > 0) {
      vq[0].enabled = 1;
    }
    if (audio_count > 0) {
      aq[0].enabled = 1;
    }
  }
  int total_enabled = 0;

  psptr->set_session_desc(0, "MPEG2 Transport Stream");

  int sdesc = 1;
  // create video media
  int ret =  mpeg2t_create_video(mp2t, psptr, vq, video_count, 
			  errmsg, errlen, sdesc);
  if (ret < 0) {
    free(aq);
    free(vq);
    return -1;
  }
  total_enabled += ret;
  // create audio media
  ret = mpeg2t_create_audio(mp2t, psptr, aq, 
			    audio_count, errmsg, errlen, sdesc);
  if (ret < 0) {
    free(aq);
    free(vq);
    return -1;
  }
  total_enabled += ret;

  free(aq);
  free(vq);
  if (total_enabled != 0) {
    // This is to get the m_streaming bit set, so we can do the
    // elapsed time correctly
    if (mp2t->m_rtsp_client) {
      psptr->create_streaming_ondemand_other(mp2t->m_rtsp_client,
					     mp2t->m_rtsp_url,
					     mp2t->m_have_end_time,
					     mp2t->m_end_time,
					     1,
					     0);
    } else {
					     
      psptr->create_streaming_broadcast(NULL, errmsg, errlen);
    }
    return 0;
  }
  return -1;
}  

// NOTE TO SELF - need to add a mechanism that indicates that 
// we're paused.  We'll need to keep reading the RTP packets, but
// will want to flush them.
// Options are to add a do_pause to the bytestream.
// or have a pause registration in either player media or playersession
