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

#define THROW_RTP_SEQ_NUM_VIOLATION ((int)1)
#define THROW_RTP_BOOKMARK_SEQ_NUM_VIOLATION ((int)2)
#define THROW_RTP_NO_MORE_DATA ((int) 3)
#define THROW_RTP_BOOKMARK_NO_MORE_DATA ((int) 4)
#define THROW_RTP_NULL_WHEN_START ((int) 5)
#define THROW_RTP_DECODE_ACROSS_TS ((int) 6)
#define THROW_RTP_BASE_MAX 6
class CRtpByteStreamBase : public COurInByteStream
{
 public:
  CRtpByteStreamBase(unsigned int rtp_proto,
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
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int Bset);
  virtual void reset(void) {
    player_debug_message("rtp bytestream reset");
    init();
    m_buffering = 0;
  };
  int have_no_data(void);
  uint64_t start_next_frame(void);
  double get_max_playtime (void) { return 0.0; };
  ssize_t read(unsigned char *buffer, size_t bytes);
  ssize_t read(char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };
  const char *get_throw_error(int error);
  int throw_error_minor(int error);

  // various routines for RTP interface.
  void set_rtp_rtptime(uint32_t t) { m_rtp_rtptime = t;};
  void set_skip_on_advance (uint32_t bytes_to_skip) {
    m_skip_on_advance_bytes = bytes_to_skip;
  };
  void set_wallclock_offset (uint64_t wclock) {
    if (m_wallclock_offset_set == 0) {
    m_wallclock_offset = wclock;
    m_wallclock_offset_set = 1;
    }
  };
  int rtp_ready (void) {
    return (m_stream_ondemand | m_wallclock_offset_set);
  };
  void check_for_end_of_pak(int nothrow = 0); // used by read and get
  void recv_callback(struct rtp *session, rtp_event *e);
  virtual void flush_rtp_packets(void);
  int recv_task(int waiting);
  uint32_t get_last_rtp_timestamp (void) {return m_rtptime_last; };
  void remove_packet_rtp_queue(rtp_packet *pak, int free);
 protected:
  void init(void);
  rtp_packet *m_pak, *m_head, *m_tail;
  int m_offset_in_pak;
  rtp_packet *m_bookmark_pak;
  int m_bookmark_offset_in_pak;
  int m_bookmark_set;
  uint32_t m_skip_on_advance_bytes;
  uint32_t m_ts;
  uint64_t m_total;
  uint64_t m_total_book;
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
  int check_rtp_frame_complete_for_proto(void);
  int m_rtp_rtpinfo_received;
  uint32_t m_rtptime_last;
};

int add_rtp_packet_to_queue(rtp_packet *pak,
			    rtp_packet **head,
			    rtp_packet **tail);
#endif
