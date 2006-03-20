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
#include "rtsp_client.h"
#include "rtsp_thread_ipc.h"
#include "mpeg4ip_sdl_includes.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define RECV_BUFF_DEFAULT_LEN 2048
/*
 * Some useful macros.
 */
/*
 * Session structure.
 */
struct rtsp_session_ {
  struct rtsp_session_ *next;
  rtsp_client_t *parent;
  char *session;
  char *url;
};
#define MAX_RTP_THREAD_SESSIONS 4
/*
 * client main structure
 */

typedef struct rtsp_thread_info_ rtsp_thread_info_t;

struct addrinfo;

struct rtsp_client_ {
  /*
   * Information about the server we're talking to.
   */
  char *orig_url;
  char *url;
  char *server_name;
  uint16_t redirect_count;
  int useTCP;
  struct in_addr server_addr;
  struct addrinfo *addr_info;
  uint16_t port;

  bool use_proxy;
  const char *proxy_name;
  in_port_t proxy_port;
  /*
   * Communications information - socket, receive buffer
   */
#ifndef _WIN32
  int server_socket;
#else
  SOCKET server_socket;
#endif
  int recv_timeout;

  /*
   * rtsp information gleamed from other packets
   */
  uint32_t next_cseq;
  char *cookie;
  rtsp_decode_t *decode_response;
  char *session;
  rtsp_session_t *session_list;
  /*
   * Thread information
   */
  struct {
    int rtp_callback_set;
    process_rtp_callback_f rtp_callback;
    rtsp_thread_callback_f rtp_periodic;
    void *rtp_userdata;
  } m_callback[MAX_RTP_THREAD_SESSIONS];
  SDL_Thread *thread;
  SDL_mutex *msg_mutex;

  rtsp_thread_info_t *m_thread_info;

  /*
   * receive buffer
   */
  uint32_t m_buffer_len, m_offset_on;
  char m_resp_buffer[RECV_BUFF_DEFAULT_LEN + 1];

};

#ifdef __cplusplus
extern "C" {
#endif

void clear_decode_response(rtsp_decode_t *decode);

void free_rtsp_client(rtsp_client_t *rptr);
void free_session_info(rtsp_session_t *session);
rtsp_client_t *rtsp_create_client_common(const char *url, int *perr,
					 const char *proxy_name,
					 in_port_t proxy_port);
int rtsp_dissect_url(rtsp_client_t *rptr, const char *url);
/* communications routines */
int rtsp_create_socket(rtsp_client_t *info);
void rtsp_close_socket(rtsp_client_t *info);

int rtsp_send(rtsp_client_t *info, const char *buff, uint32_t len);
void rtsp_flush(rtsp_client_t *info);
int rtsp_receive_socket(rtsp_client_t *rptr, char *buffer, uint32_t len,
			uint32_t msec_timeout, int wait);

int rtsp_get_response(rtsp_client_t *info);

int rtsp_setup_redirect(rtsp_client_t *info);


void rtsp_debug(int loglevel, const char *fmt, ...)
#ifndef _WIN32
     __attribute__((format(__printf__, 2, 3)));
#else
     ;
#endif

int rtsp_create_thread(rtsp_client_t *info);
void rtsp_close_thread(rtsp_client_t *info);

// Call these only from the rtsp thread
int rtsp_thread(void *data); // don't call this...
int rtsp_thread_ipc_respond(rtsp_client_t *info, int ret); // only from thread
int rtsp_thread_ipc_receive(rtsp_client_t *info, char *buffer, size_t len);
void rtsp_thread_init_thread_info(rtsp_client_t *info);
int rtsp_thread_wait_for_event(rtsp_client_t *info);
int rtsp_thread_has_control_message(rtsp_client_t *info);
int rtsp_thread_get_control_message(rtsp_client_t *info, rtsp_msg_type_t *msg);
int rtsp_thread_has_receive_data(rtsp_client_t *info);
void rtsp_thread_close(rtsp_client_t *info);
void rtsp_thread_set_nonblocking(rtsp_client_t *info);

// Call these only from the server thread.
int rtsp_thread_ipc_send (rtsp_client_t *info,
			  unsigned char *msg,
			  int len);
int rtsp_thread_ipc_send_wait(rtsp_client_t *info,
			      unsigned char *msg,
			      int msg_len,
			      int *return_msg);

int rtsp_send_and_get(rtsp_client_t *info,
		      char *buffer,
		      uint32_t buflen);

int rtsp_recv(rtsp_client_t *cptr, char *buffer, uint32_t len);

int rtsp_bytes_in_buffer(rtsp_client_t *cptr);


#ifdef __cplusplus
}
#endif

