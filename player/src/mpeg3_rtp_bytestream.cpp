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
static rtp_packet *end_of_pak (rtp_packet *start)
{
  while (start->rtp_next->rtp_pak_ts == start->rtp_pak_ts) 
    start = start->rtp_next;

  return start;
}

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

uint64_t CMpeg3RtpByteStream::start_next_frame (uint8_t **buffer, 
						uint32_t *buflen,
						void **ud)
{
  uint16_t seq = 0;
  uint32_t ts = 0, outts;
  uint64_t timetick;
  int first = 0;
  int finished = 0;
  rtp_packet *rpak;
  int32_t diff;
  int correct_hdr;
  int dropped_seq;

  diff = m_buffer_len - m_bytes_used;

  m_doing_add = 0;
  if (diff > 2) {
    // Still bytes in the buffer...
    *buffer = m_buffer + m_bytes_used;
    *buflen = diff;
#ifdef DEBUG_RTP_PAKS
    rtp_message(LOG_DEBUG, "%s Still left - %d bytes", m_name, *buflen);
#endif
    return (m_last_realtime);
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
    if (m_next_seq != rpak->rtp_pak_seq) {
      correct_hdr = 0;
      rtp_message(LOG_INFO, "%s missing rtp sequence - should be %u is %u", 
		  m_name, m_next_seq, rpak->rtp_pak_seq);
      first = 0;
    } else {
      if (first == 0) {
	ts = rpak->rtp_pak_ts;
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
    } else {
      dropped_seq = 1;
    }

    m_next_seq = rpak->rtp_pak_seq + 1; // handles wrap correctly

    uint8_t *from;
    uint32_t len;
    m_skip_on_advance_bytes = 4;
    if ((*rpak->rtp_data & 0x4) != 0) {
      m_skip_on_advance_bytes = 8;
      if ((rpak->rtp_data[4] & 0x40) != 0) {
      }
    }
    frame_type = rpak->rtp_data[2] & 0x7;
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
  if (dropped_seq) {
    outts = calc_this_ts_from_future(frame_type, ts);
  } else {
    if (m_have_prev_frame_type) {
      if (frame_type < 3) {
	// B frame
	outts = ts + m_rtp_frame_add;
      } else {
	outts = m_prev_ts + m_rtp_frame_add;
      }
    } else 
      outts = calc_this_ts_from_future(frame_type, ts);
  }
#ifdef MPEG3_RTP_TIME
  rtp_message(LOG_DEBUG, "frame type %d pak ts %u out ts %u", frame_type, 
	      ts, outts);
#endif
  m_have_prev_frame_type = 1;
  m_prev_frame_type = frame_type;
  m_prev_ts = outts;

  if (m_rtpinfo_set_from_pak != 0) {
    if (outts < m_rtp_base_ts) {
      m_rtp_base_ts = outts;
    }
    m_rtpinfo_set_from_pak = 0;
  }
  m_ts = outts;
  timetick = rtp_ts_to_msec(outts, m_wrap_offset);
  
  return (timetick);
}

uint32_t CMpeg3RtpByteStream::calc_this_ts_from_future (int frame_type,
						   uint32_t pak_ts)
{
  int new_frame_type;
  uint32_t outts;
  if (frame_type >= 3) {
    // B frame - ts is always pak_ts + 1 frame time
    return pak_ts + m_rtp_frame_add;
  }

  SDL_LockMutex(m_rtp_packet_mutex);
  if (m_head->rtp_pak_seq != m_next_seq) {
    outts = m_head->rtp_pak_ts - m_rtp_frame_add; // not accurate, but close enuff
  } else {
    new_frame_type = m_head->rtp_data[2] & 0x7;
    if (new_frame_type >= 3) {
      outts = m_head->rtp_pak_ts;
    }  else if (frame_type == 2 ||
		(frame_type == 1 && new_frame_type == 1)) {
    // I frame followed by I frame, P frame followed by I or P frame
      outts = pak_ts;
    } else {
      // I frame followed by P frame
      rtp_packet *next_pak = end_of_pak(m_head);
      next_pak = next_pak->rtp_next;
      new_frame_type = next_pak->rtp_data[2] & 0x7;
      if (new_frame_type < 3) {
	outts = pak_ts;
      } else {
	outts = next_pak->rtp_pak_ts - (2 * m_rtp_frame_add);
      }
    }
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return outts;
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
  if (m_head == NULL) return TRUE;
  first = temp = m_head;
  int count = 0;
  SDL_LockMutex(m_rtp_packet_mutex);
  do {
    if (temp->rtp_pak_m == 1) {
      count++;
      if (count > 1) {
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return FALSE;
      }
    }
    temp = temp->rtp_next;
  } while (temp != NULL && temp != first);
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return TRUE;
}


void CMpeg3RtpByteStream::rtp_done_buffering (void) 
{
  rtp_packet *p, *q;
  int had_b;
  SDL_LockMutex(m_rtp_packet_mutex);

  m_next_seq = m_head->rtp_pak_seq;


  p = end_of_pak(m_head);
  had_b = 0;
  while (p != m_head) {
    q = end_of_pak(p->rtp_next);
    if ((p->rtp_data[2] & 0x7) == 3) {
      // B frame 
      had_b = 1;
      if ((((q->rtp_pak_seq + 1) & 0xffff) == q->rtp_next->rtp_pak_seq) &&
	  ((q->rtp_next->rtp_data[2] & 0x7) == 3)) {
	// 2 consec b frames
	q = q->rtp_next;
	m_rtp_frame_add  = q->rtp_pak_ts - p->rtp_pak_ts;
#ifdef MPEG3_RTP_TIME
	rtp_message(LOG_DEBUG, "2 consec b frames %d", m_rtp_frame_add);
#endif
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return;
      }
    }
    p = q->rtp_next;
  }
  if (had_b == 0) {
    // no b frames 
    p = end_of_pak(m_head);
    while (p != m_head) {
      q = end_of_pak(p->rtp_next);
      if (((q->rtp_pak_seq + 1) & 0xffff) == q->rtp_next->rtp_pak_seq) {
	q = q->rtp_next;
	m_rtp_frame_add  = q->rtp_pak_ts - p->rtp_pak_ts;
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return;
      }
      p = q->rtp_next;
    }
  }

  m_buffering = 0;
  SDL_UnlockMutex(m_rtp_packet_mutex);
  flush_rtp_packets();
}
