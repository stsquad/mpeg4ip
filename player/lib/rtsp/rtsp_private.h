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
#include <stdint.h>
#include <netinet/in.h>
#include <sys/syslog.h>
#include <rtsp_client.h>

#ifndef __BOOL__
#define __BOOL__
typedef int bool;
#define TRUE 1
#define FALSE 0
#endif

#define RECV_BUFF_DEFAULT_LEN 2048
/*
 * Some useful macros.
 */
#define ADV_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}
#define CHECK_AND_FREE(a, b) { if (a->b != NULL) { free(a->b); a->b = NULL;}}

/*
 * Session structure.
 */
struct rtsp_session_ {
  struct rtsp_session_ *next;
  rtsp_client_t *parent;
  char *session;
  char *url;
};

/*
 * client main structure
 */
struct rtsp_client_ {
  /*
   * Information about the server we're talking to.
   */
  char *orig_url;
  char *url;
  char *server_name;
  uint16_t redirect_count;
  bool useTCP;
  struct in_addr server_addr;
  uint16_t port;

  /*
   * Communications information - socket, receive buffer
   */
  int server_socket;
  int recv_timeout;
  char *recv_buff;
  size_t recv_buff_len;
  size_t recv_buff_used;
  size_t recv_buff_parsed;

  /*
   * rtsp information gleamed from other packets
   */
  uint32_t next_cseq;
  char *cookie;
  rtsp_decode_t *decode_response;

  rtsp_session_t *session_list;
};

void clear_decode_response(rtsp_decode_t *decode);

void free_rtsp_client(rtsp_client_t *rptr);
void free_session_info(rtsp_session_t *session);

/* communications routines */
int rtsp_create_socket(rtsp_client_t *info);
void rtsp_close_socket(rtsp_client_t *info);

int rtsp_send(rtsp_client_t *info, const char *buff, size_t len);
void rtsp_flush(rtsp_client_t *info);
int rtsp_receive(rtsp_client_t *info);
int rtsp_receive_more(rtsp_client_t *info, size_t more_cnt);

int rtsp_get_response(rtsp_client_t *info);

int rtsp_setup_redirect(rtsp_client_t *info);


void rtsp_debug(int loglevel, const char *fmt, ...);

