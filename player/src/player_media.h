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
 * player_media.h - provides CPlayerMedia class, which defines the
 * interface to a particular media steam.
 */
#ifndef __PLAYER_MEDIA_H__
#define __PLAYER_MEDIA_H__ 1

#include <SDL.h>
#include <SDL_thread.h>
#include <sdp/sdp.h>
#include <rtsp/rtsp_client.h>
#include <rtp/config_unix.h>
extern "C" {
#include <rtp/rtp.h>
}
#include "msg_queue.h"
#include "ip_port.h"
#include "player_rtp_bytestream.h"

class CPlayerSession;
class CAudioSync;
class CVideoSync;

typedef struct video_info_t {
  int height;
  int width;
  int frame_rate;
} video_info_t;

typedef struct audio_info_t {
  int freq;
  int stream_has_length;
};

class CPlayerMedia {
 public:
  CPlayerMedia();
  ~CPlayerMedia();
  /* API routine - create - for RTP stream */
  int create_streaming(CPlayerSession *p,
		       media_desc_t *sdp_media,
		       const char **errmsg);
  /* API routine - create - where we provide the bytestream */
  int create_from_file (CPlayerSession *p, COurInByteStream *b, int is_video);
  /* API routine - play, pause */
  int do_play(double start_time_offset = 0.0);
  int do_pause(void);
  int is_video(void) { return (m_is_video); };
  double get_max_playtime (void) {
    if (m_byte_stream) {
      return (m_byte_stream->get_max_playtime());
    }
    return (0.0);
  }
  /* API routine - ip port information */
  uint16_t get_our_port (void) { return m_our_port; };
  void set_server_port (uint16_t port) { m_server_port = port; };
  uint16_t get_server_port (void) { return m_server_port; };

  media_desc_t *get_sdp_media_desc (void) { return m_media_info; };
  void set_source_addr (char *s)
    {
      if (m_source_addr) free(m_source_addr);
      m_source_addr = s;
    }
  CPlayerMedia *get_next (void) { return m_next; };
  void set_next (CPlayerMedia *newone) { m_next = newone; };
  int recv_thread(void);
  int decode_thread(void);
  void recv_callback(struct rtp *session, rtp_event *e);

  /* RTP information for media - maybe it should be associated with
   * the rtp byte stream, instead of here
   */
  void set_rtp_ssrc (uint32_t ssrc)
    { m_rtp_ssrc = ssrc; m_rtp_ssrc_set = TRUE;};
  void set_rtp_rtptime(uint32_t time);
  void set_rtp_init_seq (uint16_t seq)
    { m_rtp_init_seq = seq; m_rtp_init_seq_set = TRUE; };
  rtp_packet *advance_head(int bookmark_set, const char **);
  rtp_packet *get_rtp_head (void) {return m_head; };
  
  void set_video_sync(CVideoSync *p) {m_video_sync = p;};
  void set_audio_sync(CAudioSync *p) {m_audio_sync = p;};

  const video_info_t *get_video_info (void) { return m_video_info; };
  const audio_info_t *get_audio_info (void) { return m_audio_info; };
  void set_video_info (video_info_t *p) { m_video_info = p; };
  void set_audio_info (audio_info_t *p) { m_audio_info = p; };
 private:
  int m_is_video;
  int m_paused;
  CPlayerMedia *m_next;
  CPlayerSession *m_parent;
  media_desc_t *m_media_info;
  format_list_t *m_media_fmt;        // format currently running.
  rtsp_session_t *m_rtsp_session;
  C2ConsecIpPort *m_ports;
  uint16_t m_our_port;
  uint16_t m_server_port;
  char *m_source_addr;
  size_t m_rtp_packet_received;
  uint64_t m_rtp_data_received;
  time_t m_start_time;
  uint32_t m_rtptime_last;

  uint m_rtp_proto;
  
  int m_sync_time_set;
  uint64_t m_sync_time_offset;
  uint32_t m_rtptime_tickpersec;
  uint64_t m_rtptime_ntptickperrtptick;
  double m_play_start_time;
  // Receive thread variables
  SDL_Thread *m_recv_thread;
  struct rtp *m_rtp_session;

  // RTP packet queues
  SDL_mutex *m_rtp_packet_mutex;
  rtp_packet *m_head, *m_tail;
  size_t m_rtp_queue_len;
  size_t m_rtp_queue_len_max;

  // Other rtp information
  int m_rtp_ssrc_set;
  uint32_t m_rtp_ssrc;
  int m_rtp_init_seq_set;
  uint16_t m_rtp_init_seq;
  // conversion from rtptime in rtp packet to relative time in packet
  uint32_t m_rtp_rtptime;

  // Decoder thread variables
  SDL_Thread *m_decode_thread;
  volatile int m_decode_thread_waiting;
  SDL_sem *m_decode_thread_sem;

  // State change variable
  CMsgQueue m_rtp_msg_queue;
  CMsgQueue m_decode_msg_queue;
  // Private routines
  int process_rtsp_transport(char *transport);
  int process_rtsp_rtpinfo(char *rtpinfo);
  int determine_proto_from_rtp(void);
  int add_packet_to_queue(rtp_packet *pak);
  void flush_rtp_packets(void);
  CAudioSync *m_audio_sync;
  CVideoSync *m_video_sync;
  void parse_decode_message(int &thread_stop, int &decoding);
  COurInByteStream *m_byte_stream;
  CInByteStreamRtp *m_rtp_byte_stream;
  video_info_t *m_video_info;
  audio_info_t *m_audio_info;
};

#endif
