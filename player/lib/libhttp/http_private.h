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
 * http_private.h - private data structures and routines.
 */
#ifndef __MPEG4IP_HTTP_PRIVATE_H__
#define __MPEG4IP_HTTP_PRIVATE_H__ 1
#include "http.h"

typedef enum {
  HTTP_STATE_INIT,
  HTTP_STATE_CONNECTED,
  HTTP_STATE_CLOSED
} http_state_t;

#define RESP_BUF_SIZE 2048

struct http_client_ {
  const char *m_orig_url;
  const char *m_current_url;
  const char *m_host;
  const char *m_resource;
  const char *m_content_location;
  http_state_t m_state;
  uint16_t m_redirect_count;
  const char *m_redir_location;
  uint16_t m_port;
  struct in_addr m_server_addr;
  int m_server_socket;

  // headers decoded
  int m_connection_close;
  int m_content_len_received;
  int m_transfer_encoding_chunked;
  uint32_t m_content_len;
  http_resp_t *m_resp;

  // http response buffers
  uint32_t m_buffer_len, m_offset_on;
  char m_resp_buffer[RESP_BUF_SIZE + 1];
};

#define FREE_CHECK(a,b){ if (a->b != NULL) { free((void *)a->b); a->b = NULL;}}

int http_decode_and_connect_url (const char *name,
				 http_client_t *ptr);
  
int http_build_header(char *buffer, uint32_t maxlen, uint32_t *at,
		      http_client_t *cptr, const char *method,
		      const char *add_header, char *content_body);

int http_get_response(http_client_t *handle, http_resp_t **resp);

void http_resp_clear(http_resp_t *rptr);
#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

void http_debug(int loglevel, const char *fmt, ...)
#ifndef _WIN32
     __attribute__((format(__printf__, 2, 3)));
#endif
     ;

#endif
