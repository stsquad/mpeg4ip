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

#define THROW_RTP_DECODE_PAST_EOF (int)(THROW_RTP_BASE_MAX + 1)
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
  ssize_t read(unsigned char *buffer, size_t bytes);
  ssize_t read(char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };
  const char *get_throw_error(int error);
  int throw_error_minor(int error);
 private:
  char *m_frame_ptr;
  uint32_t m_offset_in_frame;
  uint32_t m_frame_len;
  uint32_t m_bookmark_offset_in_frame;
};

CRtpByteStreamBase *create_aac_rtp_bytestream (format_list_t *media_fmt,
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
#endif
