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
 * Copyright (C) Cisco Systems Inc. 2001-2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "mpeg4ip.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>
#include <sdp/sdp.h> // for NTP_TO_UNIX_TIME
#include "mpeg3_rtp_bytestream.h"
#include "our_config_file.h"
//#define DEBUG_RTP_PAKS 1
//#define DEBUG_MPEG3_RTP_TIME 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(rtp_message, "rtpbyst")
#else
#define rtp_message(loglevel, fmt...) message(loglevel, "rtpbyst", fmt)
#endif
#if 0
static rtp_packet *end_of_pak (rtp_packet *start)
{
  while (start->rtp_next->rtp_pak_ts == start->rtp_pak_ts) 
    start = start->rtp_next;

  return start;
}
#endif

CMpeg3RtpByteStream::CMpeg3RtpByteStream(unsigned int rtp_pt,
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
					 uint32_t rtp_ts) :
  CRtpByteStream("mpeg3", fmt, rtp_pt, ondemand, tickpersec, head, tail,
		 rtp_seq_set, rtp_base_seq, rtp_ts_set, rtp_base_ts, 
		 rtcp_received, ntp_frac, ntp_sec, rtp_ts)
{
}

bool CMpeg3RtpByteStream::start_next_frame (uint8_t **buffer, 
					    uint32_t *buflen,
					    frame_timestamp_t *pts,
					    void **ud)
{
  uint16_t seq = 0;
  uint32_t ts = 0;
  uint64_t timetick;
  uint64_t pak_ts = 0;
  int first = 0;
  int finished = 0;
  rtp_packet *rpak;
  int32_t diff;
  int correct_hdr;
  int dropped_seq;
  uint8_t temp_ref;
  diff = m_buffer_len - m_bytes_used;

  if (diff > 4) {
    // Still bytes in the buffer...
    *buffer = m_buffer + m_bytes_used;
    *buflen = diff;
#ifdef DEBUG_RTP_PAKS
    rtp_message(LOG_DEBUG, "%s Still left - %d bytes", m_name, *buflen);
#endif
    pts->msec_timestamp = m_last_realtime;
    pts->timestamp_is_pts = true;
    return true;
  }
  int frame_type;
  m_buffer_len = 0;
  dropped_seq = 0;
  while (finished == 0) {
    rpak = m_head;
    if (rpak == NULL) {
      *buffer = NULL;
      return 0;
    }
    remove_packet_rtp_queue(rpak, 0);

    correct_hdr = 1;
    if (check_seq(rpak->rtp_pak_seq) == false) {
      correct_hdr = 0;
      dropped_seq = 1;
      first = 0;
    } else {
      if (first == 0) {
	ts = rpak->rtp_pak_ts;
	pak_ts = rpak->pd.rtp_pd_timestamp;
	first = 1;
      } else {
	if (ts != rpak->rtp_pak_ts) {
	  rtp_message(LOG_INFO, 
		      "%s timestamp error - seq %u should be %x is %x", 
		      m_name, seq, ts, rpak->rtp_pak_ts);
	  correct_hdr = 0;
	}
	ts = rpak->rtp_pak_ts;
      }
    }
    if (correct_hdr == 0) {
      m_buffer_len = 0;
    } 

    set_last_seq(rpak->rtp_pak_seq);

    uint8_t *from;
    uint32_t len;
    m_skip_on_advance_bytes = 4;
    if ((*rpak->rtp_data & 0x4) != 0) {
      m_skip_on_advance_bytes = 8;
      if ((rpak->rtp_data[4] & 0x40) != 0) {
      }
    }
    frame_type = rpak->rtp_data[2] & 0x7;
    temp_ref = ((rpak->rtp_data[0] & 0x3) << 8) | rpak->rtp_data[1];
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
  //  rtp_message(LOG_DEBUG, "frame type %d timestamp %u", frame_type, ts);
#ifdef DEBUG_RTP_PAKS
  rtp_message(LOG_DEBUG, "%s buffer len %d", m_name, m_buffer_len);
#endif

#ifdef DEBUG_MPEG3_RTP_TIME
  rtp_message(LOG_DEBUG, "frame type %d pak ts %u %d", frame_type, 
	      ts, temp_ref);
#endif

  if (m_rtpinfo_set_from_pak != 0) {
    if (ts < m_base_rtp_ts) {
      m_base_rtp_ts = ts;
    }
    m_rtpinfo_set_from_pak = 0;
  }
  m_last_rtp_ts = ts;
  timetick = rtp_ts_to_msec(ts, pak_ts, m_wrap_offset);
  pts->msec_timestamp = timetick;
  pts->timestamp_is_pts = true;
  return true;
}

#if 0
uint32_t CMpeg3RtpByteStream::calc_this_ts_from_future (int frame_type,
							uint32_t pak_ts,
							uint8_t temp_ref)
{
  int new_frame_type;
  uint32_t outts = 0;

  m_have_mpeg_ip_ts = 0;
  if (frame_type >= 3) {
    // B frame - ts is always pak_ts
    return pak_ts; //  + m_rtp_frame_add;
  }
  if (frame_type == 1) {
    outts = pak_ts - ((temp_ref + 1) * m_rtp_frame_add);
    m_mpeg_ip_ts = pak_ts;
    m_have_mpeg_ip_ts = 1;
    return outts;
  }

  SDL_LockMutex(m_rtp_packet_mutex);
  if (check_seq(m_head->rtp_pak_seq) == false) {
    outts = m_head->rtp_pak_ts - m_rtp_frame_add; // not accurate, but close enuff
#ifdef DEBUG_MPEG3_RTP_TIME
    rtp_message(LOG_DEBUG, "calc no checkseq");
#endif
  } else {
    new_frame_type = m_head->rtp_data[2] & 0x7;
#ifdef DEBUG_MPEG3_RTP_TIME
    rtp_message(LOG_DEBUG, "calc ftype %d next %d", frame_type, new_frame_type);
#endif
    if (new_frame_type >= 3) {
      // P frame followed by B frame - take the B frame's timestamp
      outts = m_head->rtp_pak_ts - m_rtp_frame_add;
      m_mpeg_ip_ts = pak_ts;
      m_have_mpeg_ip_ts = 1;
    }  else {
      // P frame followed by I frame, P frame followed by I or P frame
      outts = pak_ts - m_rtp_frame_add;
      m_mpeg_ip_ts = pak_ts;
      m_have_mpeg_ip_ts = 1;
    } 
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return outts;
}
#endif
							
bool CMpeg3RtpByteStream::skip_next_frame (frame_timestamp_t *pts, 
					  int *hasSyncFrame,
					  uint8_t **buffer, 
					  uint32_t *buflen, 
					  void **ud)
{
  uint64_t ts;
  *hasSyncFrame = -1;  // we don't know if we have a sync frame
  m_buffer_len = m_bytes_used = 0;
  if (m_head == NULL) return 0;
  ts = m_head->rtp_pak_ts;
  do {
    check_seq(m_head->rtp_pak_seq);
    set_last_seq(m_head->rtp_pak_seq);
    remove_packet_rtp_queue(m_head, 1);
  } while (m_head != NULL && m_head->rtp_pak_ts == ts);

  if (m_head == NULL) return false;
  init();
  m_buffer_len = m_bytes_used = 0;
  return start_next_frame(buffer, buflen, pts, NULL);
}

bool CMpeg3RtpByteStream::have_frame (void)
{
  rtp_packet *temp, *first;
  if (m_head == NULL) return false;
  first = temp = m_head;
  int count = 0;
  SDL_LockMutex(m_rtp_packet_mutex);
  do {
    if (temp->rtp_pak_m == 1) {
      count++;
      if (count > 1) {
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return true;
      }
    }
    temp = temp->rtp_next;
  } while (temp != NULL && temp != first);
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return false;
}

