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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * rtp_bytestream_plugin.h - provides an RTP plugin bytestream for the codecs
 * to access
 */

#ifndef __RTP_BYTESTREAM_PLUGIN_H__
#define __RTP_BYTESTREAM_PLUGIN_H__ 1
#include "rtp_bytestream.h"
#include "rtp_plugin.h"

class CPluginRtpByteStream : public CRtpByteStreamBase
{
 public:
  CPluginRtpByteStream(rtp_plugin_t *rtp_plugin,
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
  ~CPluginRtpByteStream();
  bool start_next_frame(uint8_t **buffer, 
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **userdata);
  void used_bytes_for_frame(uint32_t bytes);
  bool skip_next_frame (frame_timestamp_t *ts, int *hasSyncFrame, uint8_t **buffer, uint32_t *buflen, void **userdata);
  bool have_frame(void);
  void flush_rtp_packets(void);
  void reset(void);
  
  uint64_t get_rtp_ts_to_msec (uint32_t rtp_ts, 
			       uint64_t ts, 
			       int just_checking) {
    if (just_checking == 1) {
      uint64_t check_value = m_wrap_offset;
      return (rtp_ts_to_msec(rtp_ts, ts, check_value));
    }
    return (rtp_ts_to_msec(rtp_ts, ts, m_wrap_offset));
  };
  rtp_packet *get_next_pak(rtp_packet *current, int remove);
  void remove_from_list(rtp_packet *pak);
  bool check_rtp_frame_complete_for_payload_type(void) 
    { return have_frame(); };
  rtp_packet *get_head_and_check(bool fail_if_not, 
				 uint32_t rtp_ts);
 protected:
  rtp_plugin_t *m_rtp_plugin;
  rtp_plugin_data_t *m_rtp_plugin_data;
};
#endif
