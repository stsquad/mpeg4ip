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

  rtsp_debug(LOG_DEBUG, "In rtsp_thread_send_and_get");
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

/*
 * rtsp_thread() - rtsp thread handler - receives and
 * processes all data
 */
static int rtsp_thread (void *data)
{
  rtsp_client_t *info = (rtsp_client_t *)data;
  int continue_thread;
  int ret;
  int receiving_rtsp_response;
  unsigned char header[3];
  unsigned short rtp_len, rtp_len_gotten;
  rtp_packet *rtp_ptr;
  int ix;

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
  rtsp_debug(LOG_DEBUG, "rtsp_thread running");
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
  receiving_rtsp_response = 0;
  rtp_ptr = NULL;
  rtp_len = rtp_len_gotten = 0;
  
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
	    receiving_rtsp_response = 1;
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
	/*
	 * First see if we're in the middle of receiving an RTP packet.
	 * If so - receive the rest of the data.
	 */
	int try_periodic = 0;
	if (rtp_ptr != NULL) {
	  ret = recv(info->server_socket,
		     ((unsigned char *)rtp_ptr) + RTP_PACKET_HEADER_SIZE + rtp_len_gotten,
		     rtp_len - rtp_len_gotten,
		     0);
	  rtsp_debug(LOG_DEBUG, "got %d more", ret);
	  rtp_len_gotten += ret;
	  if (rtp_len_gotten == rtp_len) {
	    callback_for_rtp_packet(info, header[0], rtp_ptr, rtp_len);
	    rtp_ptr = NULL;
	    try_periodic = 1;
	  }
	} else {
	  /*
	   * At the beginning... Either we're getting a $, or getting
	   * a RTP packet.
	   */
	  ret = recv(info->server_socket,
		     info->m_resp_buffer,
		     1,
		     0);
	  if (ret != 1) continue;

	  // we either have a $ - indicating RTP, or a R (for RTSP response)
	  if (*info->m_resp_buffer == '$') {
	    /*
	     * read the 3 byte header - 1 byte for interleaved channel,
	     * 2 byte length.
	     */
	    ret = recv(info->server_socket, header, sizeof(header), 0);
	    if (ret != sizeof(header)) continue;
	    rtp_len = (header[1] << 8) | header[2];
	    rtsp_debug(LOG_DEBUG, "RTP pak - %d len %d", header[0], rtp_len);
	    /*
	     * Start to receive the data packet - might not get the whole
	     * thing
	     */
	    rtp_ptr = (rtp_packet *)xmalloc(rtp_len + RTP_PACKET_HEADER_SIZE);
	    rtp_len_gotten = recv(info->server_socket,
				  ((unsigned char *)rtp_ptr) + RTP_PACKET_HEADER_SIZE,
				  rtp_len,
				  0);
	    rtsp_debug(LOG_DEBUG, "recvd %d", rtp_len_gotten);
	    if (rtp_len_gotten == rtp_len) {
	      callback_for_rtp_packet(info, header[0], rtp_ptr, rtp_len);
	      rtp_ptr = NULL;
	      try_periodic = 1;
	    }
	  } else if (tolower(*info->m_resp_buffer) == 'r') {
	    /*
	     * Getting the rtsp response.  Indicate we've got 1 byte in the
	     * buffer, then read the response.
	     */
	    info->m_offset_on = 0;
	    info->m_buffer_len = 1;
	    ret = rtsp_get_response(info);
	    if (receiving_rtsp_response != 0) {
	      /*
	       * Are they waiting ?  If so, pop them the return value
	       */
	      rtsp_thread_ipc_respond(info,
				      (const unsigned char *)&ret,
				      sizeof(ret));
	      receiving_rtsp_response = 0;
	    }
	  }
	}
	// If we got a complete packet, try the periodic vector.
	if (try_periodic != 0) {
	  ix = header[0] / 2;
	  if (info->m_callback[ix].rtp_callback_set &&
	      info->m_callback[ix].rtp_periodic != NULL) {
	    (info->m_callback[ix].rtp_periodic)(info->m_callback[ix].rtp_userdata);
	  }
	} // end try_periodic != 0
      } // end server_socket has data
    } // end have server socket
  } // end while continue_thread
  /*
   * Okay - we've gotten a quit - we're done
   */
  if (rtp_ptr != NULL) {
    xfree(rtp_ptr);
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

