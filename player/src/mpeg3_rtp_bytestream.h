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

#ifndef __MPEG3_RTP_BYTESTREAM_H__
#define __MPEG3_RTP_BYTESTREAM_H__ 1
#include "rtp_bytestream.h"


class CMpeg3RtpByteStream : public CRtpByteStream
{
 public:
  CMpeg3RtpByteStream(unsigned int rtp_pt,
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
  bool have_frame(void);
  bool start_next_frame(uint8_t **buffer, uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);
  bool skip_next_frame(frame_timestamp_t *ts, int *hasSyncFrame,
		      uint8_t **buffer, uint32_t *buflen,
		      void **userdata = NULL);
 protected:
  //  void rtp_done_buffering(void);
 private:
#if 0
  uint32_t m_rtp_frame_add;
  int m_have_mpeg_ip_ts;
  uint32_t m_mpeg_ip_ts;
  uint32_t calc_this_ts_from_future(int frame_type, uint32_t pak_ts, uint8_t temp_ref);
#endif
  
};
#endif
