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

#ifndef __PLAYER_RTP_BYTESTREAM_H__
#define __PLAYER_RTP_BYTESTREAM_H__ 1
#include "our_bytestream.h"
#include "player_util.h"
#include "rtp/rtp.h"

class CPlayerMedia;

class CInByteStreamRtp : public COurInByteStream
{
 public:
  CInByteStreamRtp(CPlayerMedia *m, int ondemand);
  ~CInByteStreamRtp();
  int eof (void) { return 0; };
  char get(void);
  char peek(void);
  void bookmark(int Bset);
  void reset(void) { init(); };
  int have_no_data(void);
  uint64_t start_next_frame(void);
  double get_max_playtime (void) { return 0.0; };
  size_t read(char *buffer, size_t bytes);
  size_t read(unsigned char *buffer, size_t bytes) {
    return (read((char *)buffer, bytes));
  };
  int still_same_ts(void);

  // various routines for RTP interface.
  void assign_pak (rtp_packet *pak);
  void dont_check_across_ts (void) {m_dont_check_across_ts = 1;} ;
  void set_rtp_rtptime(uint32_t t) { m_rtp_rtptime = t;};
  void set_rtp_config (uint64_t tps) {
    m_rtptime_tickpersec = tps;
  };
  void set_skip_on_advance (size_t bytes_to_skip) {
    m_skip_on_advance_bytes = bytes_to_skip;
  };
  void set_wallclock_offset (uint64_t wclock) {
    m_wallclock_offset = wclock;
    m_wallclock_offset_set = 1;
  };
  int rtp_ready (void) {
    return (m_stream_ondemand | m_wallclock_offset_set);
  };
  void check_for_end_of_pak(void); // used by read and get
 private:
  void init(void);
  rtp_packet *m_pak;
  int m_offset_in_pak;
  rtp_packet *m_bookmark_pak;
  int m_bookmark_offset_in_pak;
  int m_bookmark_set;
  size_t m_skip_on_advance_bytes;
  uint32_t m_ts;
  uint64_t m_total;
  uint64_t m_total_book;
  int m_dont_check_across_ts;
  uint32_t m_rtp_rtptime;
  uint64_t m_rtptime_tickpersec;
  int m_stream_ondemand;
  uint64_t m_wrap_offset;
  int m_wallclock_offset_set;
  uint64_t m_wallclock_offset;
};
#endif
