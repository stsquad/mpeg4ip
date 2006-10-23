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
 * rtsp_thread.c
 */
#include "rtsp_private.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>

typedef enum rtsp_rtp_state_t {
  RTP_DATA_UNKNOWN = 0,
  RTP_DATA_START = 1,
  RTP_DATA_CONTINUE = 2,
  RTSP_HEADER_CHECK = 3,
  RTP_HEADER_CHECK = 4,
} rtsp_rtp_state_t;

#ifdef DEBUG_RTP_STATES
static const char *states[] = {
  "DATA UNKNOWN",
  "DATA START",
  "DATA CONTINUE",
  "RTSP HEADER CHECK",
  "RTP HEADER CHECK"
};
#endif

typedef struct rtp_state_t {
  rtp_packet *rtp_ptr;
  rtsp_rtp_state_t state;
  int try_periodic;
  int receiving_rtsp_response;
  unsigned short rtp_len, rtp_len_gotten;
  unsigned char header[3];
} rtp_state_t;

/*
 * rtsp_thread_start_cmd()
 * Handle the start command - create the socket
 */
static int rtsp_thread_start_cmd (rtsp_client_t *info)
{
  int ret;
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
#endif
  ret = rtsp_create_socket(info);
  return (ret);
}

/*
 * rtsp_thread_send_and_get()
 * send a command and get the response
 */
static int rtsp_thread_send_and_get (rtsp_client_t *info)
{
  rtsp_msg_send_and_get_t msg;
  int ret;

  ret = rtsp_thread_ipc_receive(info, (char *)&msg, sizeof(msg));
  
  if (ret != sizeof(msg)) {
    rtsp_debug(LOG_DEBUG, "Send and get - recv %d", ret);
    return -1;
  }
  ret = rtsp_send(info, msg.buffer, msg.buflen);
  rtsp_debug(LOG_DEBUG, "Send and get - send %d", ret);
  return ret;
}

static int rtsp_msg_thread_perform_callback (rtsp_client_t *info)
{
  rtsp_msg_callback_t callback;
  int ret;
  int cbs;

  cbs = sizeof(callback);
  ret = rtsp_thread_ipc_receive(info, (char *)&callback, cbs);
  if (ret != cbs) {
    rtsp_debug(LOG_ERR, "Perform callback msg - recvd %d instead of %u",
	       ret, cbs); 
    return -1;
  }

  return ((callback.func)(callback.ud));
}

static int rtsp_msg_thread_set_rtp_callback (rtsp_client_t *info)
{
  rtsp_msg_rtp_callback_t callback;
  int interleave_num;
  int ret;
  int cbs = sizeof(callback);

  rtsp_debug(LOG_DEBUG, "In rtsp_msg_thread_set_rtp_callback");
  ret = rtsp_thread_ipc_receive(info, (char *)&callback, cbs);
  if (ret != cbs) {
    rtsp_debug(LOG_ERR, "Perform callback msg - recvd %d instead of %u",
	       ret, cbs);
    return -1;
  }

  interleave_num = callback.interleave;
  if (interleave_num >= MAX_RTP_THREAD_SESSIONS) {
    return -1;
  }
  if (info->m_callback[interleave_num].rtp_callback_set != 0) {
    return -2;
  }
  info->m_callback[interleave_num].rtp_callback_set = 1;
  info->m_callback[interleave_num].rtp_callback = callback.callback_func;
  info->m_callback[interleave_num].rtp_periodic = callback.periodic_func;
  info->m_callback[interleave_num].rtp_userdata = callback.ud;
  return 0;
}

int rtsp_thread_send_rtcp (rtsp_client_t *info,
			   int interleave,
			   uint8_t *buffer,
			   uint32_t buflen)
{
  int ret;
  uint8_t buff[4];

  buff[0] = '$';
  buff[1] = (interleave * 2) + 1;
  buff[2] = (buflen >> 8) & 0xff;
  buff[3] = (buflen & 0xff);
  ret = send(info->server_socket, buff, 4, 0);
  ret += send(info->server_socket, buffer, buflen, 0);
  return ret;
}

static void callback_for_rtp_packet (rtsp_client_t *info,
				     unsigned char interleave,
				     rtp_packet *rtp_ptr)
{
  unsigned char  which = interleave;
  interleave /= 2;
  if (info->m_callback[interleave].rtp_callback_set) {
    (info->m_callback[interleave].rtp_callback)(info->m_callback[interleave].rtp_userdata,
						which,
						rtp_ptr);
  } else {
    xfree(rtp_ptr);
  }
}

/****************************************************************************
 *
 * Thread RTP data receive state machine
 *
 ****************************************************************************/
static __inline void change_state (rtp_state_t *state, rtsp_rtp_state_t s)
{
  if (state->state != s) {
    state->state = s;
    //rtsp_debug(LOG_DEBUG, "changing state to %d %s", s,states[s]);
  }
}
static void move_end_of_buffer (rtsp_client_t *info, int bytes)
{
  memmove(info->m_resp_buffer,
	  &info->m_resp_buffer[info->m_offset_on],
	  bytes);
  info->m_offset_on = 0;
  info->m_buffer_len = bytes;
}

static int get_rtp_packet (rtsp_client_t *info, rtp_state_t *state)
{
  int ret;
  if (state->rtp_ptr == NULL) {
    //rtsp_debug(LOG_DEBUG, "PAK %d %d", state->header[0], state->rtp_len);
    state->rtp_ptr =
      (rtp_packet *)xmalloc(state->rtp_len + RTP_PACKET_HEADER_SIZE);
    state->rtp_len_gotten = 0;
  }
  ret = 
   rtsp_recv(info,
	      ((char *)state->rtp_ptr) + RTP_PACKET_HEADER_SIZE + state->rtp_len_gotten,
	      state->rtp_len - state->rtp_len_gotten);
  //rtsp_debug(LOG_DEBUG, "recv %d", ret);
  state->rtp_len_gotten += ret;

  if (state->rtp_len_gotten == state->rtp_len) {
    state->rtp_ptr->packet_start = ((uint8_t *)state->rtp_ptr) + RTP_PACKET_HEADER_SIZE;
    state->rtp_ptr->packet_length = state->rtp_len;
    callback_for_rtp_packet(info,
			    state->header[0],
			    state->rtp_ptr);
    state->rtp_ptr = NULL;
    state->rtp_len = 0;
    state->rtp_len_gotten = 0;
    state->try_periodic = 1;
    change_state(state, RTP_DATA_START);
    return 0;
  } 
  change_state(state, RTP_DATA_CONTINUE);
  return 1;
}

static const char *rtsp_cmp = "rtsp/1.0";
static int rtsp_get_resp (rtsp_client_t *info,
			  rtp_state_t *state)
{
  int ret;

  ret = rtsp_get_response(info);
  if (ret == RTSP_RESPONSE_RECV_ERROR) {
    change_state(state, RTP_DATA_UNKNOWN);
  } else {
    change_state(state, RTP_DATA_START);
    if (state->receiving_rtsp_response != 0) {
      /*
       * Are they waiting ?  If so, pop them the return value
       */
      rtsp_thread_ipc_respond(info, ret);
      state->receiving_rtsp_response = 0;
    }
  }
  return 1;
}
  
static int check_rtsp_resp (rtsp_client_t *info,
			    rtp_state_t *state)
{
  int len, blen, ret;

  len = strlen(rtsp_cmp);

  blen = rtsp_bytes_in_buffer(info);

  if (len > blen) {
    if (info->m_offset_on != 0) {
      move_end_of_buffer(info, blen);
    }
    ret = rtsp_receive_socket(info,
			      info->m_resp_buffer + blen,
			      len - blen,
			      0,
			      0);
    if (ret < 0) return 0;
    info->m_buffer_len += ret;
    blen = rtsp_bytes_in_buffer(info);
  }
  
  if (len <= blen) {
    if (strncasecmp(rtsp_cmp,
		    &info->m_resp_buffer[info->m_offset_on],
		    len) == 0) {
      //rtsp_debug(LOG_DEBUG, "Found resp");
      return (rtsp_get_resp(info, state));
    }

    info->m_offset_on++;
    change_state(state, RTP_DATA_UNKNOWN);
    return 0;
  }
  return -1;
}

static int check_rtp_header (rtsp_client_t *info,
			     rtp_state_t *state)
{
  int ix;
  int blen;

  blen = rtsp_bytes_in_buffer(info);

  if (blen < 3) {
    return -1;
  }

  state->header[0] = info->m_resp_buffer[info->m_offset_on];
  state->header[1] = info->m_resp_buffer[info->m_offset_on + 1];
  state->header[2] = info->m_resp_buffer[info->m_offset_on + 2];

  ix = state->header[0] / 2;
  if ((ix >= MAX_RTP_THREAD_SESSIONS) ||
      !(info->m_callback[ix].rtp_callback_set)) {
    // header failure
    info->m_offset_on++;
    change_state(state, RTP_DATA_UNKNOWN);
    return 0;
  }

  state->rtp_len = (state->header[1] << 8) | state->header[2];
  if (state->rtp_len < 1514) {
    info->m_offset_on += 3; // increment past the header
    return (get_rtp_packet(info, state));
  }
  // length is most likely incorrect
  info->m_offset_on++;
  change_state(state, RTP_DATA_UNKNOWN);
  return 0;
}

static int data_unknown (rtsp_client_t *info,
			 rtp_state_t *state)
{
  int blen;
  int ix;

  blen = rtsp_bytes_in_buffer(info);
  if (info->m_offset_on != 0) {
    move_end_of_buffer(info, blen);
  }
  blen = rtsp_receive_socket(info,
			     info->m_resp_buffer + info->m_offset_on,
			     RECV_BUFF_DEFAULT_LEN - info->m_offset_on,
			     0,
			     0);
  if (blen < 0) return -1;
  info->m_buffer_len += blen;

  blen = rtsp_bytes_in_buffer(info);

  for (ix = 0; ix < blen; ix++) {
    if (info->m_resp_buffer[info->m_offset_on] == '$') {
      info->m_offset_on++;
      change_state(state, RTP_HEADER_CHECK);
      return 0;
    } else if (tolower(info->m_resp_buffer[info->m_offset_on]) == 'r') {
      change_state(state, RTSP_HEADER_CHECK);
      return 0;
    } else {
      info->m_offset_on++;
    }
  }
      
  return -1;
}

/*
 * rtsp_thread() - rtsp thread handler - receives and
 * processes all data
 */
int rtsp_thread (void *data)
{
  rtsp_client_t *info = (rtsp_client_t *)data;
  int continue_thread;
  int ret;
  unsigned int ix;
  int state_cont;
  int bytes;
  int got_rtp_pak;
  rtp_state_t state;


  
  continue_thread = 0;
  memset(&state, sizeof(state), 0);
  state.rtp_ptr = NULL;
  state.state = RTP_DATA_UNKNOWN;
  rtsp_thread_init_thread_info(info);
  
  while (continue_thread == 0) {
	//  rtsp_debug(LOG_DEBUG, "thread waiting");
    ret = rtsp_thread_wait_for_event(info);
    if (ret <= 0) {
      if (ret < 0) {
	//rtsp_debug(LOG_ERR, "RTSP loop error %d errno %d", ret, errno);
      } else {
	if (info->server_socket != -1) {
	  for (ix = 0; ix < MAX_RTP_THREAD_SESSIONS; ix++) {
	    if (info->m_callback[ix].rtp_callback_set &&
		info->m_callback[ix].rtp_periodic != NULL) {
	      (info->m_callback[ix].rtp_periodic)(info->m_callback[ix].rtp_userdata);
	    }
	  }
	}
      }
      continue;
    }

    /*
     * See if the communications socket for IPC has any data
     */
	//rtsp_debug(LOG_DEBUG, "Thread checking control");
    ret = rtsp_thread_has_control_message(info);
    
    if (ret) {
      rtsp_msg_type_t msg_type;
      int read;
      /*
       * Yes - read the message type.
       */
      read = rtsp_thread_get_control_message(info, &msg_type);
      if (read == sizeof(msg_type)) {
	// received message
	//rtsp_debug(LOG_DEBUG, "Comm socket msg %d", msg_type);
	switch (msg_type) {
	case RTSP_MSG_QUIT:
	  continue_thread = 1;
	  break;
	case RTSP_MSG_START:
	  ret = rtsp_thread_start_cmd(info);
	  rtsp_thread_ipc_respond(info, ret);
	  break;
	case RTSP_MSG_SEND_AND_GET:
	  ret = rtsp_thread_send_and_get(info);
	  if (ret < 0) {
	    rtsp_thread_ipc_respond(info, ret);
	  } else {
	    // indicate we're supposed to receive...
	    state.receiving_rtsp_response = 1;
	  }
	  break;
	case RTSP_MSG_PERFORM_CALLBACK:
	  ret = rtsp_msg_thread_perform_callback(info);
	  rtsp_thread_ipc_respond(info, ret);
	  break;
	case RTSP_MSG_SET_RTP_CALLBACK:
	  ret = rtsp_msg_thread_set_rtp_callback(info);
	  rtsp_thread_ipc_respond(info, ret);
	  break;
	default:
		rtsp_debug(LOG_ERR, "Unknown message %d received", msg_type);
	}
      }
    }

    /*
     * See if the data socket has any data
     */
	//rtsp_debug(LOG_DEBUG, "Thread checking socket");
    if (info->server_socket != -1) {
      ret = rtsp_thread_has_receive_data(info);
      if (ret) {
	state_cont = 0;
	while (state_cont == 0) {
	  got_rtp_pak = 0;
	  switch (state.state) {
	  case RTP_DATA_UNKNOWN:
	    state_cont = data_unknown(info, &state);
	    break;
	  case RTP_HEADER_CHECK:
	    got_rtp_pak = 1;
	    state_cont = check_rtp_header(info, &state);
	    break;
	  case RTSP_HEADER_CHECK:
	    state_cont = check_rtsp_resp(info, &state);
	    break;
	  case RTP_DATA_START:
	    /*
	     * At the beginning... Either we're getting a $, or getting
	     * a RTP packet.
	     */
	    bytes = rtsp_bytes_in_buffer(info);
	    if (bytes < 4) {
	      if (bytes != 0 && info->m_offset_on != 0) {
		move_end_of_buffer(info, bytes);
	      }
	      ret = rtsp_receive_socket(info,
					info->m_resp_buffer + bytes,
					4 - bytes,
					0,
					0);
	      if (ret < 0) {
		state_cont = 1;
		break;
	      }
	      bytes += ret;
	      info->m_offset_on = 0;
	      info->m_buffer_len = bytes;
	      if (bytes < 4) {
		state_cont  = 1;
		break;
	      }
	    }
	    // we either have a $ - indicating RTP, or a R (for RTSP response)
	    if (info->m_resp_buffer[info->m_offset_on] == '$') {
	      /*
	       * read the 3 byte header - 1 byte for interleaved channel,
	       * 2 byte length.
	       */
	      info->m_offset_on++;
	
	      ret = rtsp_recv(info, (char *)state.header, 3U);
	      if (ret != 3) continue;
	      state.rtp_len = (state.header[1] << 8) | state.header[2];
	      state_cont = get_rtp_packet(info, &state);
	      got_rtp_pak = 1;
	    } else if (tolower(info->m_resp_buffer[info->m_offset_on]) == 'r') {
	      state_cont = rtsp_get_resp(info, &state);
	    } else {
	      info->m_offset_on++;
	      rtsp_debug(LOG_INFO, "Unknown data %d in rtp stream",
			  info->m_resp_buffer[info->m_offset_on]);
	      change_state(&state, RTP_DATA_UNKNOWN);
	    }
	    break;
	  case RTP_DATA_CONTINUE:
	    state_cont = get_rtp_packet(info, &state);
	    got_rtp_pak = 1;
	    break;
	  }

	  if (got_rtp_pak == 1 && state.try_periodic != 0) {
	    state.try_periodic = 0;
	    ix = state.header[0] / 2;
	    if (info->m_callback[ix].rtp_callback_set &&
		info->m_callback[ix].rtp_periodic != NULL) {
	      (info->m_callback[ix].rtp_periodic)(info->m_callback[ix].rtp_userdata);
	    }
	  }
	}
	
      } // end server_socket has data
    } // end have server socket
  } // end while continue_thread

  SDL_Delay(10);
  /*
   * Okay - we've gotten a quit - we're done
   */
  if (state.rtp_ptr != NULL) {
    xfree(state.rtp_ptr);
  }
  // exiting thread - get rid of the sockets.
  rtsp_thread_close(info);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}




int rtsp_thread_perform_callback (rtsp_client_t *info,
				  rtsp_thread_callback_f func,
				  void *ud)
{
  int ret, callback_ret;
  rtsp_wrap_msg_callback_t callback_body;

  callback_body.msg = RTSP_MSG_PERFORM_CALLBACK;
  callback_body.body.func = func;
  callback_body.body.ud = ud;
  
  ret = rtsp_thread_ipc_send_wait(info,
				  (unsigned char *)&callback_body,
				  sizeof(callback_body),
				  &callback_ret);
  if (ret != sizeof(callback_ret)) {
    return -1;
  }
  return (callback_ret);
}

int rtsp_thread_set_process_rtp_callback (rtsp_client_t *info,
					  process_rtp_callback_f rtp_callback,
					  rtsp_thread_callback_f rtp_periodic,
					  int rtp_interleave,
					  void *ud)
{
  int ret, callback_ret;
  rtsp_wrap_msg_rtp_callback_t callback_body;

  callback_body.msg = RTSP_MSG_SET_RTP_CALLBACK;
  callback_body.body.callback_func = rtp_callback;
  callback_body.body.periodic_func = rtp_periodic;
  callback_body.body.ud = ud;
  callback_body.body.interleave = rtp_interleave;
  
  ret = rtsp_thread_ipc_send_wait(info,
				  (unsigned char *)&callback_body,
				  sizeof(callback_body),
				  &callback_ret);
  if (ret != sizeof(callback_ret)) {
    return -1;
  }
  return (callback_ret);
}  

/*
 * rtsp_create_client_for_rtp_tcp
 * create threaded rtsp session
 */
rtsp_client_t *rtsp_create_client_for_rtp_tcp (const char *url,
					       int *err,
					       const char *proxy_addr,
					       in_port_t proxy_port)
{
  rtsp_client_t *info;
  int ret;
  rtsp_msg_type_t msg;
  rtsp_msg_resp_t resp;
#if 0
  if (func == NULL) {
    rtsp_debug(LOG_CRIT, "Callback is NULL");
    *err = EINVAL;
    return NULL;
  }
#endif
  info = rtsp_create_client_common(url, err, proxy_addr, proxy_port);
  if (info == NULL) return (NULL);
  info->msg_mutex = SDL_CreateMutex();
  if (rtsp_create_thread(info) != 0) {
    free_rtsp_client(info);
    return (NULL);
  }
  msg = RTSP_MSG_START;
  ret = rtsp_thread_ipc_send_wait(info,
				  (unsigned char *)&msg,
				  sizeof(msg),
				  &resp);
  if (ret < 0 || resp < 0) {
    free_rtsp_client(info);
    *err = resp;
    return NULL;
  }
				  
  return (info);
}

