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
#include "player_util.h"
#include "our_config_file.h"
#include "media_utils.h"

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mpeg2t_message, "mpeg2t")
#else
#define mpeg2t_message(loglevel, fmt...) message(loglevel, "mpeg2t", fmt)
#endif

/*
 * mpeg2t_decode_buffer - this is called from the mpeg2t thread.
 */
static void mpeg2t_decode_buffer (mpeg2t_client_t *info, 
				  uint8_t *buffer, 
				  int blen)
{
  mpeg2t_pid_t *pidptr;
  uint32_t buflen;
  uint32_t buflen_used;
  buflen = blen;

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
	break;
      }
    }
    buffer += buflen_used;
    buflen -= buflen_used;
  } while (buflen >= 188);
  if (buflen > 0) {
    mpeg2t_message(LOG_ERR, "decode buffer size is not multiple of 188 - %d %d",
		   blen, buflen);
  }
}

static void mpeg2t_rtp_callback (struct rtp *session, rtp_event *e)
{
  if (e->type == RX_RTP) {
    rtp_packet *rpak;
    rpak = (rtp_packet *)e->data;
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
#ifdef _WINDOWS
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
    info->udp = udp_init(info->address, info->rx_port,
			 info->tx_port, info->ttl);
    if (info->udp == NULL) 
      return -1;
    info->data_socket = udp_fd(info->udp);
  } else {
    info->rtp_session = rtp_init(info->address,
				 info->rx_port,
				 info->tx_port,
				 info->ttl,
				 info->rtcp_bw,
				 mpeg2t_rtp_callback,
				 (uint8_t *)info);
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
  uint8_t buffer[RTP_MAX_PACKET_LEN];
  int buflen;

  
  continue_thread = 0;
  mpeg2t_message(LOG_DEBUG, "thread started");
  mpeg2t_thread_init_thread_info(info);
  while (continue_thread == 0) {
    //    mpeg2t_message(LOG_DEBUG, "thread waiting");
    ret = mpeg2t_thread_wait_for_event(info);
    if (ret <= 0) {
      if (ret < 0) {
	//mpeg2t_message(LOG_ERR, "MPEG2T loop error %d errno %d", ret, errno);
      } else {
      }
      continue;
    }

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
	mpeg2t_message(LOG_DEBUG, "Comm socket msg %d", msg_type);
	switch (msg_type) {
	case MPEG2T_MSG_QUIT:
	  continue_thread = 1;
	  break;
	case MPEG2T_MSG_START:
	  ret = mpeg2t_thread_start_cmd(info);
	  mpeg2t_thread_ipc_respond(info, ret);
	  break;
	default:
	  //	  mpeg2t_message(LOG_ERR, "Unknown message %d received", msg_type);
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
			  buffer, RTP_MAX_PACKET_LEN);
	rtp_process_ctrl(info->rtp_session, buffer, buflen);
      }
      rtp_send_ctrl(info->rtp_session, 0, NULL);
      rtp_update(info->rtp_session);
    } else {
      if (mpeg2t_thread_has_receive_data(info)) {
	if (info->udp != NULL) {
	  mpeg2t_message(LOG_DEBUG, "receiving udp data");
	  buflen = udp_recv(info->udp, buffer, RTP_MAX_PACKET_LEN);
	  mpeg2t_decode_buffer(info, buffer, buflen);
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
#ifdef _WINDOWS
  WSACleanup();
#endif
  return 0;
}


/*
 * mpeg2t_create_client_for_rtp_tcp
 * create threaded mpeg2t session
 */
mpeg2t_client_t *mpeg2t_create_client (const char *address,
				       in_port_t rx_port,
				       in_port_t tx_port,
				       int use_rtp,
				       double rtcp_bw, 
				       int ttl,
				       char *errmsg,
				       uint32_t errmsg_len)
{
  mpeg2t_client_t *info;
  int ret;
  mpeg2t_msg_type_t msg;
  mpeg2t_msg_resp_t resp;
#if 0
  if (func == NULL) {
    mpeg2t_message(LOG_CRIT, "Callback is NULL");
    *err = EINVAL;
    return NULL;
  }
#endif
  info = MALLOC_STRUCTURE(mpeg2t_client_t);
  if (info == NULL) return (NULL);
  memset(info, 0, sizeof(mpeg2t_client_t));
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
  info->pam_recvd_sem = SDL_CreateSemaphore(0);
  info->msg_mutex = SDL_CreateMutex();
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
  
  int max = config.get_config_value(CONFIG_MPEG2T_PAM_WAIT_SECS);
  do {
    ret = SDL_SemWaitTimeout(info->pam_recvd_sem, 1000);
    if (ret == SDL_MUTEX_TIMEDOUT) {
      max--;
      mpeg2t_message(LOG_DEBUG, "timeout - still left %d", max);
    }
  } while (ret == SDL_MUTEX_TIMEDOUT && max >= 0);

  if (ret == SDL_MUTEX_TIMEDOUT) {
    snprintf(errmsg, errmsg_len, "Did not receive Transport Stream Program Map in %d seconds", config.get_config_value(CONFIG_MPEG2T_PAM_WAIT_SECS));
    mpeg2t_delete_client(info);
    return NULL;
  }
  return (info);
}

void mpeg2t_delete_client (mpeg2t_client_t *info)
{
  mpeg2t_thread_close(info);
  CHECK_AND_FREE(info, address);
  free(info);
}

static void close_mpeg2t_client (void *data)
{
  mpeg2t_delete_client((mpeg2t_client_t *)data);
}

int create_mpeg2t_session (CPlayerSession *psptr,
			   const char *orig_name, 
			   char *errmsg, 
			   uint32_t errlen, 
			   //int have_audio_driver,
			   control_callback_vft_t *cc_vft)
{
  const char *colon, *slash, *name;
  char *addr, *port;
  uint32_t addrlen, portlen;
  name = orig_name + strlen("mpeg2t://");
  colon = strchr(name, ':');
  slash = strchr(name, '/');

  if (slash == NULL) slash = name + strlen(name);
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
  in_port_t rxport;
  char *eport;
  rxport = strtoul(port, &eport, 10);
  if (eport == NULL || *eport != '\0') {
    snprintf(errmsg, errlen, "Illegal port number in url %s", orig_name);
    free(addr);
    free(port);
   return -1;
  }
  free(port);
  

  mpeg2t_client_t *mp2t;
  mp2t = mpeg2t_create_client(addr, rxport, 0, 0, 0.0, 0, errmsg, errlen);
  free(addr);
  if (mp2t == NULL) {
    return -1;
  }
  psptr->set_media_close_callback(close_mpeg2t_client, mp2t);
  return -1;
}  
