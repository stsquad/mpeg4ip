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
 * rtsp_comm.c - contains communication routines.
 */
#include "rtsp_private.h"
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif

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
  struct hostent *host;

  // Do we have a socket already - if so, go ahead

  if (info->server_socket != -1) {
    return (0);
  }
  
  if (info->server_name == NULL) {
    return (-1);
  }
  host = gethostbyname(info->server_name);
  if (host == NULL) {
    return (h_errno);
  }
  info->server_addr = *(struct in_addr *)host->h_addr;

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
int rtsp_receive (int rsocket, char *buffer, uint32_t len,
		  uint32_t msec_timeout)
{

  int ret;
#ifdef HAVE_POLL
  struct pollfd pollit;
  
  pollit.fd = rsocket;
  pollit.events = POLLIN | POLLPRI;
  pollit.revents = 0;

  ret = poll(&pollit, 1, msec_timeout);
#else
  fd_set read_set;
  struct timeval timeout;
  FD_ZERO(&read_set);
  FD_SET(rsocket, &read_set);
  timeout.tv_sec = msec_timeout / 1000;
  timeout.tv_usec = (msec_timeout % 1000) * 1000;
  ret = select(rsocket + 1, &read_set, NULL, NULL, &timeout);
#endif

  if (ret <= 0) {
    rtsp_debug(LOG_ERR, "Response timed out %d %d", msec_timeout, ret);
    if (ret == -1) {
      rtsp_debug(LOG_ERR, "Errorno is %d", errno);
    }
    return (-1);
  }

  ret = recv(rsocket, buffer, len, 0);
  return (ret);
}

/*
 * rtsp_close_socket
 * closes the socket.  Duh.
 */
void rtsp_close_socket (rtsp_client_t *info)
{
  if (info->server_socket != -1)
    closesocket(info->server_socket);
  info->server_socket = -1;
}
