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
 * rtsp_comm.c - contains communication routines.  Written for linux -
 * if you need PC, you'll have to add some stuff.
 */
#include "systems.h"
#ifndef _WINDOWS
#include <sys/poll.h>
#endif
#include "rtsp_private.h"

/*
 * rtsp_create_socket()
 * creates and connects socket to server.  Requires rtsp_info_t fields
 * port, server_addr, server_name be set.
 * returns 0 for success, -1 for failure
 */
int rtsp_create_socket (rtsp_client_t *info)
{
  struct sockaddr_in sockaddr;
  int result;

  // Do we have a socket already - if so, go ahead
  if (info->server_socket != -1) {
    return (0);
  }
  
  if (info->server_name == NULL) {
    return (-1);
  }

  info->server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (info->server_socket == -1) {
    return (-1);
  }

  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(info->port);
  sockaddr.sin_addr = info->server_addr;

  result = connect(info->server_socket,
		   (struct sockaddr *)&sockaddr,
		   sizeof(sockaddr));

  if (result == -1) {
    return (-1);
  }
  return (0);
}

/*
 * rtsp_send()
 * Sends a buffer over connected socket.  If socket isn't connected,
 * tries that first.
 * Buffer must be formatted to RTSP spec.
 * Inputs:
 *   info - pointer to rtsp_client_t for client session
 *   buff - pointer to buffer
 *   len  - length of buffer
 * Outputs:
 *   0 - success, -1 failure
 */
int rtsp_send (rtsp_client_t *info, const char *buff, uint32_t len)
{
  int ret;

  if (info->server_socket == -1) {
    if (rtsp_create_socket(info) != 0)
      return (-1);
  }
  ret = send(info->server_socket, buff, len, 0);
  return (ret);
}

/*
 * rtsp_receive()
 * Receives a response from server with a timeout.  If recv returns a
 * full buffer, and the last character is not \r or \n, will make a
 * bigger buffer and try to receive.
 *
 * Will set fields in rtsp_client_t.  Relevent fields are:
 *   recv_buff - pointer to receive buffer (malloc'ed so we can always add
 *           \0 at end).
 *   recv_buff_len - max size of receive buffer.
 *   recv_buff_used - number of bytes received.
 *   recv_buff_parsed - used by above routine in case we got more than
 *      1 response at a time.
 */
int rtsp_receive (rtsp_client_t *info)
{

  int ret, totcnt;
#ifndef _WINDOWS
  struct pollfd pollit;
#else
  fd_set read_set;
  struct timeval timeout;
#endif
  bool done;
  uint32_t bufflen;
  char *new;

  totcnt = 0;
  info->recv_buff_used = 0;
  info->recv_buff_parsed = 0;
  do {
#ifndef _WINDOWS
    pollit.fd = info->server_socket;
    pollit.events = POLLIN | POLLPRI;
    pollit.revents = 0;

    ret = poll(&pollit, 1, info->recv_timeout);
#else
	FD_ZERO(&read_set);
	FD_SET(info->server_socket, &read_set);
	timeout.tv_sec = info->recv_timeout / 1000;
	timeout.tv_usec = (info->recv_timeout % 1000) * 1000;
	ret = select(0, &read_set, NULL, NULL, &timeout);
#endif

    if (ret <= 0) {
      rtsp_debug(LOG_ERR, "Response timed out %d %d", info->recv_timeout, ret);
      if (ret == -1) {
	rtsp_debug(LOG_ERR, "Errorno is %d", errno);
      }
      return (-1);
    }

    bufflen = info->recv_buff_len - info->recv_buff_used;
    ret = recv(info->server_socket,
	       info->recv_buff + info->recv_buff_used,
	       info->recv_buff_len - info->recv_buff_used,
	       0);
    totcnt += ret;
    // We can always do this - recv_buff has a 1 byte pad at end
    info->recv_buff[totcnt] = '\0';
    if (ret == -1) {
      printf("Return from recv was -1");
      return (-1);
    }
    if (ret < bufflen) {
      done = TRUE;
      info->recv_buff_used = ret;
    } else {
      done = FALSE;
      if (ret == bufflen) {
	// We have a full buffer - if the last character is \r or \n,
	// go ahead and return.  Otherwise, realloc the buffer to a larger
	// one.
	if ((info->recv_buff[info->recv_buff_len] == '\n') ||
	    (info->recv_buff[info->recv_buff_len] == '\r')) {
	  info->recv_buff_used = info->recv_buff_len;
	  done = TRUE;
	} else {
	  new = realloc(info->recv_buff,
			info->recv_buff_len + RECV_BUFF_DEFAULT_LEN);
	  if (new == NULL) {
	    return (-1);
	  } else {
	    info->recv_buff = new;
	    info->recv_buff_used = info->recv_buff_len;
	    info->recv_buff_len += RECV_BUFF_DEFAULT_LEN;
	    rtsp_debug(LOG_DEBUG, "Reallocated buffer to len %d",
		       info->recv_buff_len);
	  }
	}
      } else {
	return (-1);
      }
    }
  } while (done == FALSE);
  return (totcnt);

}

/*
 * rtsp_receive_more()
 * Receives more data into the buffer if needed.  If we, for some reason,
 * didn't parse the message right (basically, in the case of entity which
 * is large, and managed to have a \n where the buffer would fill, but still
 * had more data), we can use this routine to get more data.
 */
int rtsp_receive_more (rtsp_client_t *info, uint32_t more_cnt)
{
  int ret;

#ifndef _WINDOWS
  struct pollfd pollit;
#else
  fd_set read_set;
  struct timeval timeout;
#endif
  uint32_t bufflen;

  if (info->recv_buff_len < info->recv_buff_used + more_cnt) {
    char *new;
    new = realloc(info->recv_buff,
		  info->recv_buff_used + more_cnt);
    if (new == NULL) {
      return (-1);
    } else {
      info->recv_buff = new;
      info->recv_buff_len = info->recv_buff_used + more_cnt;
      rtsp_debug(LOG_DEBUG, "Reallocated buffer to len %d",
		 info->recv_buff_len);
    }
  }
  
  do {
#ifndef _WINDOWS
    pollit.fd = info->server_socket;
    pollit.events = POLLIN | POLLPRI;
    pollit.revents = 0;

    ret = poll(&pollit, 1, info->recv_timeout);
#else
	FD_ZERO(&read_set);
	FD_SET(info->server_socket, &read_set);
	timeout.tv_sec = info->recv_timeout / 1000;
	timeout.tv_usec = (info->recv_timeout % 1000) * 1000;
	ret = select(0, &read_set, NULL, NULL, &timeout);
#endif  
    if (ret <= 0) {
      return (-1);
    }

    bufflen = info->recv_buff_len - info->recv_buff_used;
    ret = recv(info->server_socket,
	       info->recv_buff + info->recv_buff_used,
	       bufflen,
	       0);
    // We can always do this - recv_buff has a 1 byte pad at end
    info->recv_buff[info->recv_buff_used + ret] = '\0';
    more_cnt -= ret;
  } while (more_cnt > 0);

 return (0);
}

/*
 * rtsp_close_socket
 * closes the socket.  Duh.
 */
void rtsp_close_socket (rtsp_client_t *info)
{
  closesocket(info->server_socket);
  info->server_socket = -1;
}
