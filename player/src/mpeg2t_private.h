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
#ifndef __MPEG2T_PRIVATE_H__
#define __MPEG2T_PRIVATE_H__ 1
#include "mpeg2t_thread.h"
#include "mpeg2t_thread_ipc.h"
#include <rtp/rtp.h>
#include "mpeg4ip_sdl_includes.h"
#include <mpeg2t/mpeg2_transport.h>
#include <rtsp/rtsp_client.h>
#include "media_utils.h"
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/*
 * Some useful macros.
 */
class CPlayerMedia;

typedef struct mpeg2t_thread_info_ mpeg2t_thread_info_t;

struct addrinfo;
typedef struct mpeg2t_stream_t {
  struct mpeg2t_stream_t *next_stream;
  mpeg2t_client_t *m_parent;
  CPlayerMedia *m_mptr;
  int m_have_info;
  int m_is_video;
  int m_buffering;
  int m_have_eof;
} mpeg2t_stream_t;


struct mpeg2t_client_ {
  /*
   * Information about the server we're talking to.
   */
  char *address;
  int useRTP;
  struct in_addr server_addr;
  struct addrinfo *addr_info;
  in_port_t rx_port, tx_port;

  mpeg2t_t *decoder;

  // if using RTP
  double rtcp_bw;
  int ttl;
  struct rtp *rtp_session;

  socket_udp *udp;
  /*
   * Communications information - socket, receive buffer
   */
#ifndef _WIN32
  int data_socket;
  int rtcp_socket;
#else
  SOCKET data_socket;
  int rtcp_socket;
#endif
  int recv_timeout;

  uint16_t rtp_seq;
  /*
   * Thread information
   */
  SDL_Thread *thread;
  SDL_mutex *msg_mutex;

  SDL_sem *pam_recvd_sem;
  mpeg2t_thread_info_t *m_thread_info;

  /*
   * receive buffer
   */
  uint32_t m_buffer_len, m_offset_on;
  char m_resp_buffer[1500];
  mpeg2t_stream_t *stream;
  int m_have_rtsp;
  rtsp_client_t *m_rtsp_client;
  char *m_rtsp_url;
  int m_have_end_time;
  uint64_t m_end_time;

  int m_have_start_psts;
  uint64_t m_start_psts;
};

#ifdef __cplusplus
extern "C" {
#endif

  int mpeg2t_create_thread(mpeg2t_client_t *info);
  int mpeg2t_thread(void *data); // don't call
  void mpeg2t_close_thread(mpeg2t_client_t *info); // only from thread
int mpeg2t_thread_ipc_respond(mpeg2t_client_t *info, int ret); // only from thread
int mpeg2t_thread_ipc_receive(mpeg2t_client_t *info, char *buffer, size_t len);
void mpeg2t_thread_init_thread_info(mpeg2t_client_t *info);
int mpeg2t_thread_wait_for_event(mpeg2t_client_t *info);
int mpeg2t_thread_has_control_message(mpeg2t_client_t *info);
int mpeg2t_thread_get_control_message(mpeg2t_client_t *info, mpeg2t_msg_type_t *msg);
int mpeg2t_thread_has_receive_data(mpeg2t_client_t *info);
int mpeg2t_thread_has_rtcp_data(mpeg2t_client_t *info);
void mpeg2t_thread_close(mpeg2t_client_t *info);
void mpeg2t_thread_set_nonblocking(mpeg2t_client_t *info);

// Call these only from the server thread.
int mpeg2t_thread_ipc_send (mpeg2t_client_t *info,
			  unsigned char *msg,
			  int len);
int mpeg2t_thread_ipc_send_wait(mpeg2t_client_t *info,
			      unsigned char *msg,
			      int msg_len,
			      int *return_msg);


  void mpeg2t_debug(int loglevel, const char *fmt, ...);
#ifdef __cplusplus
}
#endif
void mpeg2t_check_streams(video_query_t **pvq, 
			  audio_query_t **paq,
			  mpeg2t_t *decoder,
			  unsigned int &audio_count,
			  unsigned int &video_count,
			  CPlayerSession *psptr,
			  control_callback_vft_t *cc_vft);
     
#endif
