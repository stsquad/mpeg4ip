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
#include <SDL.h>
#include <SDL_thread.h>

class CRtpByteStreamBase : public COurInByteStream
{
 public:
  CRtpByteStreamBase(const char *name,
		     unsigned int rtp_proto,
		     int ondemand,
		     uint64_t tickpersec,
		     rtp_packet **head, 
		     rtp_packet **tail,
		     int rtpinfo_received,
		     uint32_t rtp_rtptime,
		     int rtcp_received,
		     uint32_t ntp_frac,
		     uint32_t ntp_sec,
		     uint32_t rtp_ts);

  ~CRtpByteStreamBase();
  int eof (void) { return 0; };
  virtual void reset(void) {
    player_debug_message("rtp bytestream reset");
    init();
    m_buffering = 0;
  };
  void set_skip_on_advance (uint32_t bytes_to_skip) {
    m_skip_on_advance_bytes = bytes_to_skip;
  };
  double get_max_playtime (void) { return 0.0; };

  // various routines for RTP interface.
  void set_rtp_rtptime(uint32_t t) { m_rtp_rtptime = t;};
  void set_wallclock_offset (uint64_t wclock) {
    if (m_wallclock_offset_set == 0) {
    m_wallclock_offset = wclock;
    m_wallclock_offset_set = 1;
    }
  };
  int rtp_ready (void) {
    return (m_stream_ondemand | m_wallclock_offset_set);
  };
  void recv_callback(struct rtp *session, rtp_event *e);
  virtual void flush_rtp_packets(void);
  int recv_task(int waiting);
  uint32_t get_last_rtp_timestamp (void) {return m_rtptime_last; };
  void remove_packet_rtp_queue(rtp_packet *pak, int free);
 protected:
  void init(void);
  uint64_t rtp_ts_to_msec(uint32_t ts, uint64_t &wrap_offset);
  rtp_packet *m_head, *m_tail;
  int m_offset_in_pak;
  uint32_t m_skip_on_advance_bytes;
  uint32_t m_ts;
  uint64_t m_total;
  uint32_t m_rtp_rtptime;
  uint64_t m_rtptime_tickpersec;
  int m_stream_ondemand;
  uint64_t m_wrap_offset;
  int m_wallclock_offset_set;
  uint64_t m_wallclock_offset;
  void calculate_wallclock_offset_from_rtcp(uint32_t ntp_frac,
					    uint32_t ntp_sec,
					    uint32_t rtp_ts);
  SDL_mutex *m_rtp_packet_mutex;
  int m_buffering;
  uint64_t m_rtp_buffer_time;
  unsigned int m_rtp_proto;
  virtual int check_rtp_frame_complete_for_proto(void);
  int m_rtp_rtpinfo_received;
  uint32_t m_rtptime_last;
  int m_doing_add;
  uint32_t m_add;
};

class CRtpByteStream : public CRtpByteStreamBase
{
 public:
  CRtpByteStream(const char *name,
		 unsigned int rtp_proto,
		 int ondemand,
		 uint64_t tickpersec,
		 rtp_packet **head, 
		 rtp_packet **tail,
		 int rtpinfo_received,
		 uint32_t rtp_rtptime,
		 int rtcp_received,
		 uint32_t ntp_frac,
		 uint32_t ntp_sec,
		 uint32_t rtp_ts);
  ~CRtpByteStream();
  uint64_t start_next_frame(unsigned char **buffer, uint32_t *buflen);
  int can_skip_frame (void) { return 1; } ;
  int skip_next_frame(uint64_t *ts, int *havesync, unsigned char **buffer,
		      uint32_t *buflen);
  void used_bytes_for_frame(uint32_t bytes);
  int have_no_data(void);
  void flush_rtp_packets(void);
 protected:
  unsigned char *m_buffer;
  uint32_t m_buffer_len;
  uint32_t m_buffer_len_max;
  uint32_t m_bytes_used;
};

class CAudioRtpByteStream : public CRtpByteStream
{
 public:
  CAudioRtpByteStream(unsigned int rtp_proto,
		      int ondemand,
		      uint64_t tickpersec,
		      rtp_packet **head, 
		      rtp_packet **tail,
		      int rtpinfo_received,
		      uint32_t rtp_rtptime,
		      int rtcp_received,
		      uint32_t ntp_frac,
		      uint32_t ntp_sec,
		      uint32_t rtp_ts);
  ~CAudioRtpByteStream();
  int have_no_data(void);
  int check_rtp_frame_complete_for_proto(void);
  uint64_t start_next_frame(unsigned char **buffer, uint32_t *buflen);
 private:
  rtp_packet *m_working_pak;
};
int add_rtp_packet_to_queue(rtp_packet *pak,
			    rtp_packet **head,
			    rtp_packet **tail);
#endif
