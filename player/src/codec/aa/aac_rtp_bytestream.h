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

#ifndef __AAC_RTP_BYTESTREAM_H__
#define __AAC_RTP_BYTESTREAM_H__ 1
#include "rtp_bytestream.h"

class CAacRtpByteStream : public CRtpByteStreamBase
{
 public:
  CAacRtpByteStream(unsigned int rtp_proto,
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

  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int Bset);
  void reset(void);
  uint64_t start_next_frame(void);
  size_t read(unsigned char *buffer, size_t bytes);
  size_t read(char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };

 private:
  char *m_frame_ptr;
  size_t m_offset_in_frame;
  size_t m_frame_len;
  size_t m_bookmark_offset_in_frame;
};

#endif
