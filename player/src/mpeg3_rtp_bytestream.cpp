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

#include "systems.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>
#include <sdp/sdp.h> // for NTP_TO_UNIX_TIME
#include "mpeg3_rtp_bytestream.h"
#include "our_config_file.h"
//#define DEBUG_RTP_PAKS 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(rtp_message, "rtpbyst")
#else
#define rtp_message(loglevel, fmt...) message(loglevel, "rtpbyst", fmt)
#endif

CMpeg3RtpByteStream::CMpeg3RtpByteStream(unsigned int rtp_pt,
					 format_list_t *fmt,
					 int ondemand,
					 uint64_t tickpersec,
					 rtp_packet **head, 
					 rtp_packet **tail,
					 int rtpinfo_received,
					 uint32_t rtp_rtptime,
					 int rtcp_received,
					 uint32_t ntp_frac,
					 uint32_t ntp_sec,
					 uint32_t rtp_ts) :
  CRtpByteStream("mpeg3", fmt, rtp_pt, ondemand, tickpersec, head, tail,
		 rtpinfo_received, rtp_rtptime, rtcp_received,
		 ntp_frac, ntp_sec, rtp_ts)
{
}

uint64_t CMpeg3RtpByteStream::start_next_frame (uint8_t **buffer, 
						uint32_t *buflen,
						void **ud)
{
  uint16_t seq = 0;
  uint32_t ts = 0;
  uint64_t timetick;
  int first = 0;
  int finished = 0;
  rtp_packet *rpak;
  int32_t diff;

  diff = m_buffer_len - m_bytes_used;

  m_doing_add = 0;
  if (diff > 2) {
    // Still bytes in the buffer...
    *buffer = m_buffer + m_bytes_used;
    *buflen = diff;
    return (m_last_realtime);
#ifdef DEBUG_RTP_PAKS
    rtp_message(LOG_DEBUG, "%s Still left - %d bytes", m_name, *buflen);
#endif
  } else {
    m_buffer_len = 0;
    while (finished == 0) {
      rpak = m_head;
      remove_packet_rtp_queue(rpak, 0);
      
      if (first == 0) {
	seq = rpak->rtp_pak_seq + 1;
	ts = rpak->rtp_pak_ts;
	first = 1;
      } else {
	if ((seq != rpak->rtp_pak_seq) ||
	    (ts != rpak->rtp_pak_ts)) {
	  if (seq != rpak->rtp_pak_seq) {
	    rtp_message(LOG_INFO, "%s missing rtp sequence - should be %u is %u", 
			m_name, seq, rpak->rtp_pak_seq);
	  } else {
	    rtp_message(LOG_INFO, "%s timestamp error - seq %u should be %x is %x", 
			m_name, seq, ts, rpak->rtp_pak_ts);
	  }
	  m_buffer_len = 0;
	  ts = rpak->rtp_pak_ts;
	}
	seq = rpak->rtp_pak_seq + 1;
      }
      uint8_t *from;
      uint32_t len;
      m_skip_on_advance_bytes = 4;
      if ((*rpak->rtp_data & 0x4) != 0) {
	m_skip_on_advance_bytes = 8;
	if ((rpak->rtp_data[4] & 0x40) != 0) {
	}
      }
      from = (uint8_t *)rpak->rtp_data + m_skip_on_advance_bytes;
      len = rpak->rtp_data_len - m_skip_on_advance_bytes;
      if ((m_buffer_len + len) > m_buffer_len_max) {
	// realloc
	m_buffer_len_max = m_buffer_len + len + 1024;
	m_buffer = (uint8_t *)realloc(m_buffer, m_buffer_len_max);
      }
      memcpy(m_buffer + m_buffer_len, 
	     from,
	     len);
      m_buffer_len += len;
      if (rpak->rtp_pak_m == 1) {
	finished = 1;
      }
      xfree(rpak);
    }
    m_bytes_used = 0;
    *buffer = m_buffer + m_bytes_used;
    *buflen = m_buffer_len - m_bytes_used;
#ifdef DEBUG_RTP_PAKS
    rtp_message(LOG_DEBUG, "%s buffer len %d", m_name, m_buffer_len);
#endif
  }
  m_ts = ts;
  timetick = rtp_ts_to_msec(ts, m_wrap_offset);
  
  return (timetick);
}

int CMpeg3RtpByteStream::skip_next_frame (uint64_t *pts, int *hasSyncFrame,
					  uint8_t **buffer, 
					  uint32_t *buflen)
{
  uint64_t ts;
  *hasSyncFrame = -1;  // we don't know if we have a sync frame
  m_buffer_len = m_bytes_used = 0;
  if (m_head == NULL) return 0;
  ts = m_head->rtp_pak_ts;
  do {
    remove_packet_rtp_queue(m_head, 1);
  } while (m_head != NULL && m_head->rtp_pak_ts == ts);

  if (m_head == NULL) return 0;
  init();
  m_buffer_len = m_bytes_used = 0;
  ts = start_next_frame(buffer, buflen, NULL);
  *pts = ts;
  return (1);
}

int CMpeg3RtpByteStream::have_no_data (void)
{
  rtp_packet *temp, *first;
  first = temp = m_head;
  if (temp == NULL) return TRUE;
  do {
    if (temp->rtp_pak_m == 1) return FALSE;
    temp = temp->rtp_next;
  } while (temp != NULL && temp != first);
  return TRUE;
}

