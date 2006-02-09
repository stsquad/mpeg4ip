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
/*
 * http.c - public APIs
 */
#include "mpeg4ip.h"
#include "http_private.h"
#include "mpeg4ip_utils.h"

/*
 * http_init_connection()
 * decode url and make connection
 */
http_client_t *http_init_connection (const char *name)
{
  http_client_t *ptr;
  char *url;
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;
 
	wVersionRequested = MAKEWORD( 2, 0 );
 
	ret = WSAStartup( wVersionRequested, &wsaData );
	if ( ret != 0 ) {
	   /* Tell the user that we couldn't find a usable */
	   /* WinSock DLL.*/
		http_debug(LOG_ERR, "Can't initialize http_debug");
	    return (NULL);
	}
#endif
  ptr = (http_client_t *)malloc(sizeof(http_client_t));
  if (ptr == NULL) {
    return (NULL);
  }

  memset(ptr, 0, sizeof(http_client_t));
  ptr->m_state = HTTP_STATE_INIT;
  url = convert_url(name);
  http_debug(LOG_INFO, "Connecting to %s", url);
  if (http_decode_and_connect_url(url, ptr) < 0) {
    free(url);
    http_free_connection(ptr);
    return (NULL);
  }
  free(url);
  return (ptr);
}

/*
 * http_free_connection - disconnect (if still connected) and free up
 * everything to do with this session
 */
void http_free_connection (http_client_t *ptr)
{
  if (ptr->m_state == HTTP_STATE_CONNECTED) {
    closesocket(ptr->m_server_socket);
    ptr->m_server_socket = -1;
  }
  FREE_CHECK(ptr, m_orig_url);
  FREE_CHECK(ptr, m_current_url);
  FREE_CHECK(ptr, m_host);
  FREE_CHECK(ptr, m_resource);
  FREE_CHECK(ptr, m_redir_location);
  FREE_CHECK(ptr, m_content_location);
  free(ptr);
#ifdef _WIN32
  WSACleanup();
#endif
}

/*
 * http_get - get from url after client already set up
 */
int http_get (http_client_t *cptr,
	      const char *url,
	      http_resp_t **resp)
{
  char header_buffer[4096];
  uint32_t buffer_len;
  int ret;
  int more;
  
  if (cptr == NULL)
    return (-1);

  http_debug(LOG_DEBUG, "url is %s\n", url);
  if (url != NULL) {
    http_debug(LOG_DEBUG, "resource is now %s", url);
    CHECK_AND_FREE(cptr->m_resource);
    cptr->m_resource = strdup(url);
  } else {
    cptr->m_resource = cptr->m_content_location;
    cptr->m_content_location = NULL;
  }
  
  if (*resp != NULL) {
    http_resp_clear(*resp);
  }
  buffer_len = 0;
  /*
   * build header and send message
   */
  ret = http_build_header(header_buffer, 4096, &buffer_len, cptr, "GET",
			  NULL, NULL);
  http_debug(LOG_DEBUG, "%s", header_buffer);
  if (send(cptr->m_server_socket,
	   header_buffer,
	   buffer_len,
	   0) < 0) {
    http_debug(LOG_CRIT,"Http send failure");
    return (-1);
  }
  cptr->m_redirect_count = 0;
  more = 0;
  /*
   * get response - handle redirection here
   */
  do {
    ret = http_get_response(cptr, resp);
    http_debug(LOG_INFO, "Response %d", (*resp)->ret_code);
    http_debug(LOG_DEBUG, "%s", (*resp)->body);
    if (ret < 0) return (ret);
    switch ((*resp)->ret_code / 100) {
    default:
    case 1:
      more = 0;
      break;
    case 2:
      return (1);
    case 3:
      cptr->m_redirect_count++;
      if (cptr->m_redirect_count > 5) {
	return (-1);
      }
      if (http_decode_and_connect_url(cptr->m_redir_location, cptr) < 0) {
	http_debug(LOG_CRIT, "Couldn't reup location %s", cptr->m_redir_location);
	return (-1);
      }
      buffer_len = 0;
      ret = http_build_header(header_buffer, 4096, &buffer_len, cptr, "GET",
			      NULL, NULL);
      http_debug(LOG_DEBUG, "%s", header_buffer);
      if (send(cptr->m_server_socket,
	       header_buffer,
	       buffer_len,
	       0) < 0) {
	http_debug(LOG_CRIT,"Send failure");
	return (-1);
      }
      
      break;
    case 4:
    case 5:
      return (0);
    }
  } while (more == 0);
  return (ret);
}

int http_post (http_client_t *cptr,
	       const char *url,
	       http_resp_t **resp,
	       char *body)
{
  char header_buffer[4096];
  uint32_t buffer_len;
  int ret;
  int more;
  
  if (cptr == NULL)
    return (-1);
  
  if (*resp != NULL) {
    http_resp_clear(*resp);
  }
  buffer_len = 0;
  if (url != NULL) {
    CHECK_AND_FREE(cptr->m_resource);
    cptr->m_resource = strdup(url);
  }
  /*
   * build header and send message
   */
  ret = http_build_header(header_buffer, 4096, &buffer_len, cptr, "POST",
			  "Content-Type: application/x-www-form-urlencoded", body);
  if (ret == -1) {
    http_debug(LOG_ERR, "Could not build header");
    return -1;
  }
  http_debug(LOG_DEBUG, "%s", header_buffer);
  if (send(cptr->m_server_socket,
	   header_buffer,
	   buffer_len,
	   0) < 0) {
    http_debug(LOG_CRIT,"Http send failure");
    return (-1);
  }
  cptr->m_redirect_count = 0;
  more = 0;
  /*
   * get response - handle redirection here
   */
  do {
    ret = http_get_response(cptr, resp);
    http_debug(LOG_INFO, "Response %d", (*resp)->ret_code);
    http_debug(LOG_DEBUG, "%s", (*resp)->body);
    if (ret < 0) return (ret);
    switch ((*resp)->ret_code / 100) {
    default:
    case 1:
      more = 0;
      break;
    case 2:
      return (1);
    case 3:
      cptr->m_redirect_count++;
      if (cptr->m_redirect_count > 5) {
	return (-1);
      }
      if (http_decode_and_connect_url(cptr->m_redir_location, cptr) < 0) {
	http_debug(LOG_CRIT, "Couldn't reup location %s", cptr->m_redir_location);
	return (-1);
      }
      buffer_len = 0;
      ret = http_build_header(header_buffer, 4096, &buffer_len, cptr, "POST",
			      "Content-type: application/x-www-form-urlencoded",  body);
      http_debug(LOG_DEBUG, "%s", header_buffer);
      if (send(cptr->m_server_socket,
	       header_buffer,
	       buffer_len,
	       0) < 0) {
	http_debug(LOG_CRIT,"Send failure");
	return (-1);
      }
      
      break;
    case 4:
    case 5:
      return (0);
    }
  } while (more == 0);
  return (ret);
}
