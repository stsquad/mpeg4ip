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
 * http.h - library interface
 */
#ifndef __MPEG4IP_HTTP_H__
#define __MPEG4IP_HTTP_H__ 1

typedef struct http_client_ http_client_t;

/*
 * http_resp_t - everything you should need to know about the response
 */
typedef struct http_resp_t {
  int ret_code;
  const char *content_type;
  const char *resp_phrase; // After the response code, a phrase may occur
  char *body;
  uint32_t body_len;
} http_resp_t;

#ifdef __cplusplus
extern "C" {
#endif

  /*
   * http_init_connection - start with this - create a connection to a
   * particular destination.
   */
  http_client_t *http_init_connection(const char *url);
  /*
   * http_free_connection - when done talking to this particular destination,
   * call this.
   */
  void http_free_connection(http_client_t *handle);

  /*
   * http_get - get from a particular url on the location specified above
   */
  int http_get(http_client_t *, const char *url, http_resp_t **resp);
  int http_post (http_client_t *cptr,
		 const char *url,
		 http_resp_t **resp,
		 char *body);
  /*
   * http_resp_free - free up response structure passed as a result of
   * http_get
   */
  void http_resp_free(http_resp_t *);

  /*
   * http_set_loglevel - set the log level for library output
   */
  void http_set_loglevel(int loglevel);
  /*
   * http_set_error_func - error message handler to call
   */
  void http_set_error_func(error_msg_func_t func);
#ifdef __cplusplus
}
#endif
#endif
