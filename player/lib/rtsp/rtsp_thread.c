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
#include <sys/un.h>
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif

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
 * rtsp_thread_ipc_respond
 * respond with the message given
 */
static int rtsp_thread_ipc_respond (rtsp_client_t *info,
				    const unsigned char *msg,
				    uint32_t len)
{
  size_t ret;
  rtsp_debug(LOG_DEBUG, "Sending resp to thread %d - len %d", msg[0], len);
  ret = send(COMM_SOCKET_THREAD, msg, len, 0);
  return ret == len;
}

/*
 * rtsp_thread_start_cmd()
 * Handle the start command - create the socket
 */
static int rtsp_thread_start_cmd (rtsp_client_t *info)
{
  int ret;
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
#endif
  ret = rtsp_create_socket(info);
  return (0);
}

/*
 * rtsp_thread_send_and_get()
 * send a command and get the response
 */
static int rtsp_thread_send_and_get (rtsp_client_t *info)
{
  rtsp_msg_send_and_get_t msg;
  int ret;

  ret = recv(COMM_SOCKET_THREAD, (char *)&msg, sizeof(msg), 0);
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
  ret = recv(COMM_SOCKET_THREAD, &callback, cbs, 0);
  if (ret != cbs) {
    rtsp_debug(LOG_ERR, "Perform callback msg - recvd %d instead of %d",
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
  ret = recv(COMM_SOCKET_THREAD, &callback, cbs, 0);
  if (ret != cbs) {
    rtsp_debug(LOG_ERR, "Perform callback msg - recvd %d instead of %d",
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
			   char *buffer,
			   int buflen)
{
  int ret;
  char buff[4];

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
				     rtp_packet *rtp_ptr,
				     unsigned short rtp_len)
{
  unsigned char  which = interleave;
  interleave /= 2;
  if (info->m_callback[interleave].rtp_callback_set) {
    (info->m_callback[interleave].rtp_callback)(info->m_callback[interleave].rtp_userdata,
						which,
						rtp_ptr,
						rtp_len);
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
    callback_for_rtp_packet(info,
			    state->header[0],
			    state->rtp_ptr,
			    state->rtp_len);
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
      rtsp_thread_ipc_respond(info,
			      (const unsigned char *)&ret,
			      sizeof(ret));
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
    ret = rtsp_receive_socket(info->server_socket,
			      info->m_resp_buffer + blen,
			      len - blen,
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
  blen = rtsp_receive_socket(info->server_socket,
			     info->m_resp_buffer + info->m_offset_on,
			     RECV_BUFF_DEFAULT_LEN - info->m_offset_on,
			     0);
  if (blen < 0) return 0;
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
static int rtsp_thread (void *data)
{
  rtsp_client_t *info = (rtsp_client_t *)data;
  int continue_thread;
  int ret;
  unsigned int ix;
  int state_cont;
  int bytes;
  int got_rtp_pak;
  rtp_state_t state;


#ifdef HAVE_POLL
  struct pollfd pollit[2];
#else
  int max_fd;
  fd_set read_set;
  struct timeval timeout;
#endif

#ifndef HAVE_SOCKETPAIR
  struct sockaddr_un our_name, his_name;
  int len;
  //rtsp_debug(LOG_DEBUG, "rtsp_thread running");
  COMM_SOCKET_THREAD = socket(AF_UNIX, SOCK_STREAM, 0);

  memset(&our_name, 0, sizeof(our_name));
  our_name.sun_family = AF_UNIX;
  strcpy(our_name.sun_path, info->socket_name);
  ret = bind(COMM_SOCKET_THREAD, (struct sockaddr *)&our_name, sizeof(our_name));
  listen(COMM_SOCKET_THREAD, 1);
  len = sizeof(his_name);
  info->comm_socket_write_to = accept(COMM_SOCKET_THREAD, (struct sockaddr *)&his_name, &len);
#else
  info->comm_socket_write_to = info->comm_socket[0];
#endif
  
  continue_thread = 0;
  memset(&state, sizeof(state), 0);
  state.rtp_ptr = NULL;
  state.state = RTP_DATA_UNKNOWN;
  
  while (continue_thread == 0) {
#ifdef HAVE_POLL
#define SERVER_SOCKET_HAS_DATA ((pollit[1].revents & (POLLIN | POLLPRI)) != 0)
#define COMM_SOCKET_HAS_DATA   ((pollit[0].revents & (POLLIN | POLLPRI)) != 0)
    pollit[0].fd = COMM_SOCKET_THREAD;
    pollit[0].events = POLLIN | POLLPRI;
    pollit[0].revents = 0;
    pollit[1].fd = info->server_socket;
    pollit[1].events = POLLIN | POLLPRI;
    pollit[1].revents = 0;

    ret = poll(pollit, info->server_socket == -1 ? 1 : 2, info->recv_timeout);
    //    rtsp_debug(LOG_DEBUG, "poll ret %d", ret);
#else
#define SERVER_SOCKET_HAS_DATA (FD_ISSET(info->server_socket, &read_set))
#define COMM_SOCKET_HAS_DATA   (FD_ISSET(COMM_SOCKET_THREAD, &read_set))
    FD_ZERO(&read_set);
    max_fd = COMM_SOCKET_THREAD;
    if (info->server_socket != -1) {
      FD_SET(info->server_socket, &read_set);
      max_fd = MAX(info->server_socket, max_fd);
    }
    FD_SET(COMM_SOCKET_THREAD, &read_set);
    timeout.tv_sec = info->recv_timeout / 1000;
    timeout.tv_usec = (info->recv_timeout % 1000) * 1000;
    ret = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
#endif
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
    ret = COMM_SOCKET_HAS_DATA;
    if (ret) {
      rtsp_msg_type_t msg_type;
      int read;
      /*
       * Yes - read the message type.
       */
      read = recv(COMM_SOCKET_THREAD, (char *)&msg_type, sizeof(msg_type), 0);
      if (read == sizeof(msg_type)) {
	// received message
	rtsp_debug(LOG_DEBUG, "Comm socket msg %d", msg_type);
	switch (msg_type) {
	case RTSP_MSG_QUIT:
	  continue_thread = 1;
	  break;
	case RTSP_MSG_START:
	  ret = rtsp_thread_start_cmd(info);
	  rtsp_thread_ipc_respond(info,
				  (const unsigned char *)&ret,
				  sizeof(ret));
	  break;
	case RTSP_MSG_SEND_AND_GET:
	  ret = rtsp_thread_send_and_get(info);
	  if (ret < 0) {
	    rtsp_thread_ipc_respond(info,
				    (const unsigned char *)&ret,
				    sizeof(ret));
	  } else {
	    // indicate we're supposed to receive...
	    state.receiving_rtsp_response = 1;
	  }
	  break;
	case RTSP_MSG_PERFORM_CALLBACK:
	  ret = rtsp_msg_thread_perform_callback(info);
	  rtsp_thread_ipc_respond(info,
				  (const unsigned char *)&ret,
				  sizeof(ret));
	  break;
	case RTSP_MSG_SET_RTP_CALLBACK:
	  ret = rtsp_msg_thread_set_rtp_callback(info);
	  rtsp_thread_ipc_respond(info,
				  (const unsigned char *)&ret,
				  sizeof(ret));
	  break;
	}
      }
    }

    /*
     * See if the data socket has any data
     */
    if (info->server_socket != -1) {
      ret = SERVER_SOCKET_HAS_DATA;
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
	      ret = rtsp_receive_socket(info->server_socket,
					info->m_resp_buffer + bytes,
					4 - bytes,
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
	
	      ret = rtsp_recv(info, state.header, 3);
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
  rtsp_close_socket(info);
#ifndef HAVE_SOCKETPAIR
  closesocket(info->comm_socket_write_to);
  info->comm_socket_write_to = -1;
  unlink(info->socket_name);
#endif
  // exiting thread - get rid of the sockets.
#ifdef _WINDOWS
  WSACleanup();
#endif
  return 0;
}

/*
 * rtsp_create_thread - create the thread we need, along with the
 * communications socket.
 */
int rtsp_create_thread (rtsp_client_t *info)
{
#ifdef HAVE_SOCKETPAIR
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, info->comm_socket) < 0) {
    rtsp_debug(LOG_CRIT, "Couldn't create comm sockets - errno %d", errno);
    return -1;
  }
#else
  int ret;
  struct sockaddr_un addr;
  COMM_SOCKET_THREAD = -1;
  COMM_SOCKET_CALLER = socket(AF_UNIX, SOCK_STREAM, 0);
  snprintf(info->socket_name, sizeof(info->socket_name) - 1, "RTSPCLIENT%p", info);
  unlink(info->socket_name);
#endif
  info->thread = SDL_CreateThread(rtsp_thread, info);
  if (info->thread == NULL) {
    rtsp_debug(LOG_CRIT, "Couldn't create comm thread");
    return -1;
  }
#ifndef HAVE_SOCKETPAIR
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, info->socket_name);
  ret = -1;
  do {
    ret = connect(COMM_SOCKET_CALLER, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
      SDL_Delay(10);
  } while (ret < 0);
#endif
  return 0;
}

/*
 * rtsp_thread_ipc_send - send message to rtsp thread
 */
int rtsp_thread_ipc_send (rtsp_client_t *info,
			  const unsigned char *msg,
			  int len)
{
  int ret;
  rtsp_debug(LOG_DEBUG, "Sending msg to thread %d - len %d", msg[0], len);
  ret = send(COMM_SOCKET_CALLER, msg, len, 0);
  return ret == len;
}

/*
 * rtsp_thread_ipc_send_wait
 * send a message, and wait for response
 * returns number of bytes we've received.
 */
int rtsp_thread_ipc_send_wait (rtsp_client_t *info,
			       const unsigned char *msg,
			       int msg_len,
			       char *return_msg,
			       int return_msg_len)
{
  int read, ret;
#ifdef HAVE_POLL
  struct pollfd pollit;
#else
  fd_set read_set;
  struct timeval timeout;
#endif
  SDL_LockMutex(info->msg_mutex);
  ret = send(COMM_SOCKET_CALLER, msg, msg_len, 0);
  if (ret != msg_len) {
    SDL_UnlockMutex(info->msg_mutex);
    return -1;
  }
#ifdef HAVE_POLL
  pollit.fd = COMM_SOCKET_CALLER;
  pollit.events = POLLIN | POLLPRI;
  pollit.revents = 0;

  ret = poll(&pollit, 1, 30 * 1000);
  rtsp_debug(LOG_DEBUG, "return comm socket value %x", pollit.revents);
#else
  FD_ZERO(&read_set);
  FD_SET(COMM_SOCKET_CALLER, &read_set);
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  ret = select(COMM_SOCKET_CALLER + 1, &read_set, NULL, NULL, &timeout);
#endif

  if (ret <= 0) {
    if (ret < 0) {
      //rtsp_debug(LOG_ERR, "RTSP loop error %d errno %d", ret, errno);
    }
    SDL_UnlockMutex(info->msg_mutex);
    return -1;
  }

  read = recv(COMM_SOCKET_CALLER, return_msg, return_msg_len, 0);
  SDL_UnlockMutex(info->msg_mutex);
  rtsp_debug(LOG_DEBUG, "comm socket got return value of %d", read);
  return (read);
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
				  (const unsigned char *)&callback_body,
				  sizeof(callback_body),
				  (char *)&callback_ret,
				  sizeof(callback_ret));
  if (ret != sizeof(callback_ret)) {
    return -1;
  }
  return (callback_ret);
}

int rtsp_thread_set_rtp_callback (rtsp_client_t *info,
				  rtp_callback_f rtp_callback,
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
				  (const unsigned char *)&callback_body,
				  sizeof(callback_body),
				  (char *)&callback_ret,
				  sizeof(callback_ret));
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
					       int *err)
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
  info = rtsp_create_client_common(url, err);
  if (info == NULL) return (NULL);
  info->msg_mutex = SDL_CreateMutex();
  info->comm_socket[0] = -1;
  info->comm_socket[1] = -1;
  info->comm_socket_write_to = -1;
  if (rtsp_create_thread(info) != 0) {
    free_rtsp_client(info);
    return (NULL);
  }
  msg = RTSP_MSG_START;
  ret = rtsp_thread_ipc_send_wait(info,
				  (const unsigned char *)&msg,
				  sizeof(msg),
				  (char *)&resp,
				  sizeof(resp));
  if (ret < 0 || resp < 0) {
    free_rtsp_client(info);
    *err = resp;
    return NULL;
  }
				  
  return (info);
}

