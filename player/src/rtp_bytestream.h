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
 * player_rtp_bytestream.h - provides an RTP bytestream for the codecs
 * to access
 */

#ifndef __RTP_BYTESTREAM_H__
#define __RTP_BYTESTREAM_H__ 1
#include "our_bytestream.h"
#include "player_util.h"
#include "rtp/rtp.h"
#include "mpeg4ip_sdl_includes.h"
#include <sdp/sdp.h>
#include "player_session.h"


class CRtpByteStreamBase : public COurInByteStream
{
 public:
  CRtpByteStreamBase(const char *name,
		     format_list_t *fmt,
		     unsigned int rtp_pt,
		     int ondemand,
		     uint64_t tickpersec,
		     rtp_packet **head, 
		     rtp_packet **tail,
		     int rtp_seq_set,
		     uint16_t rtp_base_seq,
		     int rtp_ts_set,
		     uint32_t rtp_base_ts,
		     int rtcp_received,
		     uint32_t ntp_frac,
		     uint32_t ntp_sec,
		     uint32_t rtp_ts);

  ~CRtpByteStreamBase();
  int eof (void) { return m_eof; };
  virtual void reset(void) {
    player_debug_message("%s rtp bytestream reset", m_name);
    init();
    m_buffering = 0;
    m_base_ts_set = 0;
    m_rtcp_received = false;
    m_rtp_base_seq_set = 0;
    m_have_sync_info = false;
    m_have_recv_last_ts = false;
    m_have_first_pak_ts = false;
    m_recvd_pak = false;
    m_recvd_pak_timeout = false;
  };
  void set_skip_on_advance (uint32_t bytes_to_skip) {
    m_skip_on_advance_bytes = bytes_to_skip;
  };
  double get_max_playtime (void) { 
    if (m_fmt->media->media_range.have_range) {
      return m_fmt->media->media_range.range_end;
    } else if (m_fmt->media->parent->session_range.have_range) {
      return m_fmt->media->parent->session_range.range_end;
    }
    return 0.0; 
  };

  // various routines for RTP interface.
  void set_rtp_base_ts(uint32_t t, uint64_t value = 0) { 
    m_base_ts_set = true; 
    m_base_rtp_ts = t;
    m_base_ts = value;
  };
  void set_rtp_base_seq(uint16_t s) { 
    m_rtp_base_seq_set = true;
    m_rtp_base_seq = s;
  };
  int can_skip_frame (void) { return 1; } ;
  void set_wallclock_offset (uint64_t wclock, uint32_t rtp_ts);
  int recv_callback(struct rtp *session, rtp_event *e);
  virtual void flush_rtp_packets(void);
  void rtp_periodic(void);
  uint32_t get_last_rtp_timestamp (void) {return m_last_rtp_ts; };
  void remove_packet_rtp_queue(rtp_packet *pak, int free);
  void pause(void);
  void set_sync(CPlayerSession *psptr);

  void synchronize(rtcp_sync_t *sync);
  bool check_seq (uint16_t seq);
  void set_last_seq(uint16_t seq);
  bool find_mbit(void);
  void display_status(void);
  void set_rtp_buffer_time (uint64_t ts) { m_rtp_buffer_time = ts; };
  virtual bool check_rtp_frame_complete_for_payload_type(void);
  int check_buffering(void);
 protected:
  void init(void);
  // Make sure all classes call this to calculate real time.
  uint64_t rtp_ts_to_msec(uint32_t rtp_ts, uint64_t uts, uint64_t &wrap_offset);
  rtp_packet *m_head, *m_tail;
  int m_offset_in_pak;
  uint32_t m_skip_on_advance_bytes;
  uint64_t m_total;
  bool m_base_ts_set;
  uint32_t m_base_rtp_ts;
  uint64_t m_base_ts;
  bool m_rtp_base_seq_set;
  uint16_t m_rtp_base_seq;
  uint64_t m_timescale;
  int m_stream_ondemand;
  uint64_t m_wrap_offset;
  bool m_rtcp_received;
  uint64_t m_rtcp_ts;
  uint32_t m_rtcp_rtp_ts;
  uint64_t m_wallclock_offset_wrap;
  void calculate_wallclock_offset_from_rtcp(uint32_t ntp_frac,
					    uint32_t ntp_sec,
					    uint32_t rtp_ts);
  SDL_mutex *m_rtp_packet_mutex;
  int m_buffering;
  uint64_t m_rtp_buffer_time;
  unsigned int m_rtp_pt;
  bool m_recvd_pak;
  bool m_recvd_pak_timeout;
  uint64_t m_recvd_pak_timeout_time;
  uint64_t m_last_realtime;
  uint32_t m_last_rtp_ts;
  format_list_t *m_fmt;
  int m_eof;
  int m_rtpinfo_set_from_pak;
  uint16_t m_next_seq;
  bool m_have_first_pak_ts;
  uint64_t m_first_pak_ts;
  uint32_t m_first_pak_rtp_ts;
  CPlayerSession *m_psptr;
  bool m_have_sync_info;
  rtcp_sync_t m_sync_info;
  bool m_have_recv_last_ts;
  uint32_t m_recv_last_ts;
};

class CRtpByteStream : public CRtpByteStreamBase
{
 public:
  CRtpByteStream(const char *name,
		 format_list_t *fmt,
		 unsigned int rtp_pt,
		 int ondemand,
		 uint64_t tickpersec,
		 rtp_packet **head, 
		 rtp_packet **tail,
		 int rtp_seq_set,
		 uint16_t rtp_base_seq,
		 int rtp_ts_set,
		 uint32_t rtp_base_ts,
		 int rtcp_received,
		 uint32_t ntp_frac,
		 uint32_t ntp_sec,
		 uint32_t rtp_ts);
  ~CRtpByteStream();
  bool start_next_frame(uint8_t **buffer, uint32_t *buflen,
			frame_timestamp_t *ts, void **userdata);
  bool skip_next_frame(frame_timestamp_t *ts, int *havesync, uint8_t **buffer,
		      uint32_t *buflen, void **userdata = NULL);
  void used_bytes_for_frame(uint32_t bytes);
  bool have_frame(void);
  void flush_rtp_packets(void);
  void reset(void);
 protected:
  uint8_t *m_buffer;
  uint32_t m_buffer_len;
  uint32_t m_buffer_len_max;
  uint32_t m_bytes_used;
};

class CAudioRtpByteStream : public CRtpByteStream
{
 public:
  CAudioRtpByteStream(unsigned int rtp_pt,
		      format_list_t *fmt,
		      int ondemand,
		      uint64_t tickpersec,
		      rtp_packet **head, 
		      rtp_packet **tail,
		      int rtp_seq_set,
		      uint16_t rtp_base_seq,
		      int rtp_ts_set,
		      uint32_t rtp_base_ts,
		      int rtcp_received,
		      uint32_t ntp_frac,
		      uint32_t ntp_sec,
		      uint32_t rtp_ts);
  ~CAudioRtpByteStream();
  bool have_frame(void);
  bool check_rtp_frame_complete_for_payload_type(void);
  bool start_next_frame(uint8_t **buffer, uint32_t *buflen,
			frame_timestamp_t *ts, void **userdata);
  void flush_rtp_packets(void);
  void reset(void);
 private:
  rtp_packet *m_working_pak;
};
int add_rtp_packet_to_queue(rtp_packet *pak,
			    rtp_packet **head,
			    rtp_packet **tail,
			    const char *name);
#endif
