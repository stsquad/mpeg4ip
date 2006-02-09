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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
#include "rtsp_private.h"
#include "utils/mpeg4ip_utils.h"
/*
 * free_rtsp_client()
 * frees all memory associated with rtsp client information
 */
void free_rtsp_client (rtsp_client_t *rptr)
{
  if (rptr->thread != NULL) {
    rtsp_close_thread(rptr);
  } else {
    rtsp_close_socket(rptr);
#ifdef _WIN32
    WSACleanup();
#endif
  }
  if (rptr->msg_mutex != NULL) {
    SDL_DestroyMutex(rptr->msg_mutex);
    rptr->msg_mutex = NULL;
  }
  CHECK_AND_FREE(rptr->orig_url);
  CHECK_AND_FREE(rptr->url);
  CHECK_AND_FREE(rptr->server_name);
  CHECK_AND_FREE(rptr->cookie);
  CHECK_AND_FREE(rptr->proxy_name);
  free_decode_response(rptr->decode_response);
  rptr->decode_response = NULL;
  free(rptr);
}


rtsp_client_t *rtsp_create_client_common (const char *url, int *perr,
					  const char *proxy_addr,
					  in_port_t proxy_port)
{
  int err;
  rtsp_client_t *info;
  char *converted_url;

  info = malloc(sizeof(rtsp_client_t));
  if (info == NULL) {
    *perr = ENOMEM;
    return (NULL);
  }
  memset(info, 0, sizeof(rtsp_client_t));
  info->url = NULL;
  info->orig_url = NULL;
  info->server_name = NULL;
  info->cookie = NULL;
  info->recv_timeout = 2 * 1000;  // default timeout is 2 seconds.
  info->server_socket = -1;
  info->next_cseq = 1;
  info->session = NULL;
  info->m_offset_on = 0;
  info->m_buffer_len = 0;
  info->m_resp_buffer[RECV_BUFF_DEFAULT_LEN] = '\0';
  info->thread = NULL;

  converted_url = convert_url(url);
  err = rtsp_dissect_url(info, converted_url);
  if (err != 0) {
    free(converted_url);
    rtsp_debug(LOG_ALERT, "Couldn't decode url %d\n", err);
    *perr = err;
    free_rtsp_client(info);
    return (NULL);
  }
  free(converted_url);

  info->use_proxy = proxy_addr != NULL && *proxy_addr != '\0';
  if (info->use_proxy) {
    info->proxy_name = strdup(proxy_addr);
    info->proxy_port = proxy_port != 0 ? proxy_port : 554;
  }
  return (info);
}


rtsp_client_t *rtsp_create_client (const char *url, int *err,
				   const char *proxy_addr, 
				   in_port_t proxy_port)
{
  rtsp_client_t *info;

#ifdef _WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  int ret;
 
  wVersionRequested = MAKEWORD( 2, 0 );
 
  ret = WSAStartup( wVersionRequested, &wsaData );
  if ( ret != 0 ) {
    /* Tell the user that we couldn't find a usable */
    /* WinSock DLL.*/
    *err = ret;
    return (NULL);
  }
#endif

  info = rtsp_create_client_common(url, err, proxy_addr, proxy_port);
  if (info == NULL) return (NULL);
  
  *err = rtsp_create_socket(info);
  if (*err != 0) {
    rtsp_debug(LOG_EMERG,"Couldn't create socket %d\n", *err);
    free_rtsp_client(info);
    return (NULL);
  }
  return (info);
}



int rtsp_send_and_get (rtsp_client_t *info,
		       char *buffer,
		       uint32_t buflen)
{
  int ret;
  if (info->thread == NULL) {
    ret = rtsp_send(info, buffer, buflen);
    if (ret < 0) {
      return (RTSP_RESPONSE_RECV_ERROR);
    }

    ret = rtsp_get_response(info);
  } else {
   rtsp_wrap_send_and_get_t msg;
    int ret_msg;
    msg.msg = RTSP_MSG_SEND_AND_GET;
    msg.body.buffer = buffer;
    msg.body.buflen = buflen;
    ret = rtsp_thread_ipc_send_wait(info,
				    (unsigned char *)&msg,
				    sizeof(msg),
				    &ret_msg);
    if (ret != sizeof(ret_msg)) {
      return (RTSP_RESPONSE_RECV_ERROR);
    }
    ret = ret_msg;
  }
  return ret;
}

const struct in_addr *rtsp_get_server_address (rtsp_client_t *client)
{
  return &client->server_addr;
}
