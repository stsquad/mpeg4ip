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
  CInByteStreamRtp(CPlayerMedia *m);
  ~CInByteStreamRtp();
  int eof (void) { return 0; };
  char get(void);
  char peek(void);
  void bookmark(int Bset);
  void reset(void) { init(); };
  int have_no_data(void);
  uint64_t start_next_frame(void);
  double get_max_playtime (void) { return 0.0; };
  int still_same_ts(void);

  // various routines for RTP interface.
  void assign_pak (rtp_packet *pak);
  void dont_check_across_ts (void) {m_dont_check_across_ts = 1;} ;
  void set_rtp_rtptime(uint32_t t) { m_rtp_rtptime = t;};
  void set_rtp_config (uint64_t tps) {
    m_rtptime_tickpersec = tps;
  };
 private:
  void init(void);
  rtp_packet *m_pak;
  int m_offset_in_pak;
  rtp_packet *m_bookmark_pak;
  int m_bookmark_offset_in_pak;
  int m_bookmark_set;
  uint32_t m_ts;
  uint64_t m_total;
  uint64_t m_total_book;
  int m_dont_check_across_ts;
  uint32_t m_rtp_rtptime;
  uint64_t m_rtptime_tickpersec;

};
#endif
