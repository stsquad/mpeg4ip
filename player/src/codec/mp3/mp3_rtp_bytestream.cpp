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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mp3_rtp_bytestream.h - provides an RTP bytestream for the codecs
 * to access
 */
#include "mp3_rtp_bytestream.h"
#include <rtp/memory.h>
//#define DEBUG_RTP_PAKS 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mp3_rtp_message, "mp3rtpbyst")
#else
#define mp3_rtp_message(loglevel, fmt...) message(loglevel, "mp3rtpbyst", fmt)
#endif
CMP3RtpByteStream::CMP3RtpByteStream (unsigned int rtp_pt,
				      format_list_t *fmt,
				      int ondemand,
				      uint64_t tps,
				      rtp_packet **head, 
				      rtp_packet **tail,
				      int rtp_seq_set, 
				      uint16_t rtp_seq,
				      int rtp_ts_set,
				      uint32_t rtp_base_ts,
				      int rtcp_received,
				      uint32_t ntp_frac,
				      uint32_t ntp_sec,
				      uint32_t rtp_ts) :
  CRtpByteStream("mp3", 
		 fmt,
		 rtp_pt,
		 ondemand,
		 tps,
		 head, 
		 tail,
		 rtp_seq_set,
		 rtp_seq,
		 rtp_ts_set,
		 rtp_base_ts,
		 rtcp_received,
		 ntp_frac,
		 ntp_sec,
		 rtp_ts)
{
  m_pak_on = NULL;
  set_skip_on_advance(4);
  init();
}

CMP3RtpByteStream::~CMP3RtpByteStream(void)
{
}

int CMP3RtpByteStream::have_no_data (void)
{
  if (m_head == NULL) return TRUE;
  return FALSE;
}

int CMP3RtpByteStream::check_rtp_frame_complete_for_payload_type (void)
{
  return m_head != NULL;
}
void CMP3RtpByteStream::reset(void)
{
  m_buffer_len = m_bytes_used = 0;
  CRtpByteStream::reset();
}

uint64_t CMP3RtpByteStream::start_next_frame (uint8_t **buffer, 
					      uint32_t *buflen,
					      void **userdata)
{
  uint64_t timetick;
  int32_t diff;

  diff = m_buffer_len - m_bytes_used;

  if (diff > 2) {
    // Still bytes in the buffer...
    *buffer = m_mp3_frame + m_bytes_used;
    *buflen = diff;
#ifdef DEBUG_RTP_PAKS
    mp3_rtp_message(LOG_DEBUG, "%s Still left - %d bytes", m_name, *buflen);
#endif
    return (m_last_realtime);
  } else {
    m_buffer_len = 0;
    if (m_pak_on != NULL) {
      xfree(m_pak_on);
    }
    m_pak_on = m_head;
    remove_packet_rtp_queue(m_pak_on, 0);
      
    m_mp3_frame = (uint8_t *)m_pak_on->rtp_data;
    m_buffer_len = m_pak_on->rtp_data_len;
    m_ts = m_pak_on->rtp_pak_ts;

    m_bytes_used = m_skip_on_advance_bytes;
    *buffer = m_mp3_frame + m_bytes_used;
    *buflen = m_buffer_len - m_bytes_used;
#ifdef DEBUG_RTP_PAKS
    mp3_rtp_message(LOG_DEBUG, "%s buffer len %d", m_name, m_buffer_len);
#endif
  }
  timetick = rtp_ts_to_msec(m_ts, m_pak_on->pd.rtp_pd_timestamp, m_wrap_offset);
  
  return (timetick);
}
