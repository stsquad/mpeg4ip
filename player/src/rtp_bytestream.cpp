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
#include "rtp_bytestream.h"
#include "our_config_file.h"
//#define DEBUG_RTP_PAKS 1
/*
 * add_rtp_packet_to_queue() - adds rtp packet to doubly linked lists - 
 * this is used both by the bytestream, and by the player_media when trying
 * to determine which rtp protocol is being used.
 */
int add_rtp_packet_to_queue (rtp_packet *pak, 
			     rtp_packet **head,
			     rtp_packet **tail)
{
  rtp_packet *q;
  int inserted = TRUE;
  int16_t diff;
#ifdef DEBUG_RTP_PAKS
  player_debug_message("CBThread %u - m %u pt %u seq %x ts %x len %d", 
		       SDL_ThreadID(),
		       pak->m, pak->pt, pak->seq, pak->ts, 
		       pak->data_len);
#endif
  
  if (*head == NULL) {
    *head = *tail = pak;
  } else if (*head == *tail) {
    if (pak->seq == (*head)->seq) {
      player_debug_message("Duplicate RTP sequence number %d received", 
			   pak->seq);
      inserted = FALSE;
    } else {
      (*head)->next = pak;
      (*head)->prev = pak;
      pak->next = pak->prev = *head;
      diff = pak->seq - (*head)->seq;
      if (diff > 0) {
	*tail = pak;
      } else {
	*head = pak;
      }
    }
  } else if ((((*head)->seq < (*tail)->seq) &&
	      ((pak->seq > (*tail)->seq) || pak->seq < (*head)->seq)) ||
	     (((*head)->seq > (*tail)->seq) &&
	      ((pak->seq > (*tail)->seq && pak->seq < (*head)->seq)))) {
    // insert between tail and head
    // Maybe move head - probably move tail.
    (*tail)->next = pak;
    pak->prev = *tail;
    (*head)->prev = pak;
    pak->next = *head;
    diff = (*head)->seq - pak->seq;
    if (diff > 0 && diff < 4) {
      // between tail and head, and really close to the head - move the
      // head pointer
      *head = pak;
    } else {
      // otherwise, just insert at end
      *tail = pak;
    }
  } else {
    // insert in middle
    // Loop through until we find where it should fit
    q = *head;
    do {
      // check for duplicates
      if (pak->seq == q->seq || pak->seq == q->next->seq) {
	// dup seq number
	inserted = FALSE;
	break;
      }
      // Okay - this is disgusting, but works.  The first part (before the
      // or) will see if pak->seq is between q and q->next, assuming that the 
      // sequence number for q and q->next are ascending.
      //
      // 2nd part of the if is the converse case (q->next->seq is smaller
      // than q->seq).  In that case, we need to make sure that pak->seq is
      // either larger than q->seq or less than q->next->seq
      if (((q->next->seq > q->seq) &&
	   (q->seq < pak->seq && pak->seq < q->next->seq)) ||
	  ((q->next->seq < q->seq) &&
	   (pak->seq < q->next->seq || pak->seq > q->seq))) {
	q->next->prev = pak;
	pak->next = q->next;
	q->next = pak;
	pak->prev = q;
	break;
      }
      q = q->next;
    } while (q != *tail);
    if (q == *tail) {
      inserted = FALSE;
      player_error_message("Couldn't insert %u between %u and %u", 
			   pak->seq, (*head)->seq, (*tail)->seq);
    }
  }

  if (inserted == FALSE) {
    player_error_message("Couldn't insert pak");
    xfree(pak);
    return (0);
  }
  return (1);
}

CRtpByteStreamBase::CRtpByteStreamBase(unsigned int rtp_proto,
				       int ondemand,
				       uint64_t tps,
				       rtp_packet **head, 
				       rtp_packet **tail,
				       int rtpinfo_received,
				       uint32_t rtp_rtptime,
				       int rtcp_received,
				       uint32_t ntp_frac,
				       uint32_t ntp_sec,
				       uint32_t rtp_ts) :
  COurInByteStream()
{
  m_head = *head;
  *head = NULL;
  m_tail = *tail;
  *tail = NULL;
  m_rtp_rtpinfo_received = rtpinfo_received;
  m_rtp_rtptime = rtp_rtptime;
  
  m_rtp_proto = rtp_proto;
  uint64_t temp;
  temp = config.get_config_value(CONFIG_RTP_BUFFER_TIME);
  if (temp > 0) {
    m_rtp_buffer_time = temp;
    m_rtp_buffer_time *= M_LLU;
  } else {
    m_rtp_buffer_time = (ondemand ? 2 : 5)* M_LLU;
  }

  m_rtptime_tickpersec = tps;

  if (rtcp_received) {
    calculate_wallclock_offset_from_rtcp(ntp_frac, ntp_sec, rtp_ts);
  }
  init();

  m_ts = 0;
  m_total =0;
  m_skip_on_advance_bytes = 0;
  m_stream_ondemand = ondemand;
  m_wrap_offset = 0;
  m_wallclock_offset_set = 0;
  m_rtp_packet_mutex = SDL_CreateMutex();
  m_buffering = 0;
}

CRtpByteStreamBase::~CRtpByteStreamBase (void)
{
  flush_rtp_packets();
  if (m_rtp_packet_mutex) {
    SDL_DestroyMutex(m_rtp_packet_mutex);
    m_rtp_packet_mutex = NULL;
  }
}

void CRtpByteStreamBase::init (void)
{
  m_pak = NULL;
  m_offset_in_pak = 0;
  m_bookmark_set = 0;
}

void CRtpByteStreamBase::check_for_end_of_pak (int nothrow)
{
  rtp_packet *p;
  uint16_t nextseq;
  const char *err;

  // See if we're at the end of the packet - if not, return...
  if (m_offset_in_pak < m_pak->data_len) {
    return;
  }

  // End of packet - move to next one
  m_offset_in_pak = m_skip_on_advance_bytes;

  if (m_bookmark_set == 1) {
    /* 
     * If we're doing bookmark - make sure we keep the previous packets
     * around
     */
    if (SDL_mutexP(m_rtp_packet_mutex) == -1) {
      player_error_message("SDL Lock mutex failure in decode thread");
      return;
    }
    if (m_head == NULL || m_head == m_head->next || m_head->next == NULL) {
      SDL_mutexV(m_rtp_packet_mutex);
      m_pak = NULL;
      return;
    }
    p = m_head->next;
    nextseq = m_head->seq + 1;
    if (nextseq != p->seq) {
      //player_debug_message("bookmark seq number");
      m_pak = NULL;
    }
    SDL_mutexV(m_rtp_packet_mutex);
    return;
  }

  nextseq = m_head->seq + 1;
  remove_packet_rtp_queue(m_head, 1);
  /*
   * Check the sequence number...
   */
  if (m_head && m_head->seq == nextseq) {
    m_pak = m_head;
    return;
  }

  if (m_head) {
    if (m_bookmark_set == 0)
      err = "Sequence number violation";
    else 
      err = "Bookmark sequence number violation";
    player_debug_message("seq # violation - should %d is %d", nextseq, m_head->seq);
  } else {
    if (m_bookmark_set == 0) 
      err = "No more data";
    else
      err = "Bookmark no more data";
  }
  m_pak = NULL;
  init();
  if (nothrow == 0) {
    throw err;
  }
}

unsigned char CRtpByteStreamBase::get (void)
{
  unsigned char ret;

  if (m_pak == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw "NULL when start";
  }

  if ((m_offset_in_pak == 0) &&
      (m_ts != m_pak->ts)) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    throw("DECODE ACROSS TS");
  }
  ret = m_pak->data[m_offset_in_pak];
  m_offset_in_pak++;
  m_total++;
  check_for_end_of_pak();
  return (ret);
}

unsigned char CRtpByteStreamBase::peek (void) 
{
  return (m_pak ? m_pak->data[m_offset_in_pak] : 0);
}

void CRtpByteStreamBase::bookmark (int bSet)
{
  if (bSet == TRUE) {
    m_bookmark_set = 1;
    m_bookmark_pak = m_pak;
    m_bookmark_offset_in_pak = m_offset_in_pak;
    m_total_book = m_total;
    //player_debug_message("bookmark on");
  } else {
    m_bookmark_set = 0;
    m_pak= m_bookmark_pak;
    m_offset_in_pak = m_bookmark_offset_in_pak;
    m_total = m_total_book;
    //player_debug_message("book restore %d", m_offset_in_pak);
  }
}

uint64_t CRtpByteStreamBase::start_next_frame (void)
{
  uint64_t timetick;
  // We're going to have to handle wrap better...
  if (m_stream_ondemand) {
    m_ts = m_pak->ts;
    int neg = 0;
    
    if (m_ts >= m_rtp_rtptime) {
      timetick = m_ts;
      timetick -= m_rtp_rtptime;
    } else {
      timetick = m_rtp_rtptime - m_ts;
      neg = 1;
    }
    timetick *= 1000;
    timetick /= m_rtptime_tickpersec;
    if (neg == 1) {
      if (timetick > m_play_start_time) return (0);
      return (m_play_start_time - timetick);
    }
    timetick += m_play_start_time;
  } else {
    if (((m_ts & 0x80000000) == 0x80000000) &&
	((m_pak->ts & 0x80000000) == 0)) {
      m_wrap_offset += (I_LLU << 32);
    }
    m_ts = m_pak->ts;
    timetick = m_ts + m_wrap_offset;
    timetick *= 1000;
    timetick /= m_rtptime_tickpersec;
    timetick += m_wallclock_offset;
#if 0
    player_debug_message("time %x "LLU" "LLU" "LLU" "LLU "offset %d",
			 m_ts, m_wrap_offset, m_rtptime_tickpersec, m_wallclock_offset,
			 timetick, m_offset_in_pak);
#endif
  }

  return (timetick);
}

size_t CRtpByteStreamBase::read (unsigned char *buffer, size_t bytes_to_read)
{
  size_t inbuffer, readbytes = 0;

  if (m_pak == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw "NULL when start";
  }

  do {
    if ((m_offset_in_pak == 0) &&
	(m_ts != m_pak->ts)) {
      if (m_bookmark_set == 1) {
	return (0);
      }
      throw("DECODE ACROSS TS");
    }
    inbuffer = m_pak->data_len - m_offset_in_pak;
    if (bytes_to_read < inbuffer) {
      inbuffer = bytes_to_read;
    }
    memcpy(buffer, &m_pak->data[m_offset_in_pak], inbuffer);
    buffer += inbuffer;
    bytes_to_read -= inbuffer;
    m_offset_in_pak += inbuffer;
    m_total += inbuffer;
    readbytes += inbuffer;
    check_for_end_of_pak();
  } while (bytes_to_read > 0 && m_pak != NULL);
  return (readbytes);
}

int CRtpByteStreamBase::have_no_data (void)
{
  if (m_pak == NULL) {
    m_pak = m_head;
  }
  return (m_pak == NULL);
}

/*
 * calculate_wallclock_offset_from_rtcp
 * Given certain information from an rtcp message, Calculate what the
 * wallclock time for rtp sequence number 0 would be.
 */
void
CRtpByteStreamBase::calculate_wallclock_offset_from_rtcp (uint32_t ntp_frac,
							  uint32_t ntp_sec,
							  uint32_t rtp_ts)
{
  uint64_t wclock;
  wclock = ntp_frac;
  wclock *= M_LLU;
  wclock /= (I_LLU << 32);
  wclock += (ntp_sec - NTP_TO_UNIX_TIME) * 1000;
  uint64_t offset;
  offset = rtp_ts;
  offset *= M_LLU;
  offset /= m_rtptime_tickpersec;
  // offset is now a consistent offset between wall clock time and rtp
  // for rtp ts 0.
  wclock -= offset;
#if 0
  player_debug_message("wallclock offset is %llu - prev %llu", 
		       wclock, m_wallclock_offset);
#endif
  set_wallclock_offset(wclock);
}

/*
 * recv_callback - callback for when bytestream is active - basically, 
 * put things on the queue
 */
void CRtpByteStreamBase::recv_callback (struct rtp *session, rtp_event *e)
{
  switch (e->type) {
  case RX_RTP:
    rtp_packet *rpak;
    rpak = (rtp_packet *)e->data;
    if (rpak->data_len == 0) {
      xfree(rpak);
    } else {
      // need to add lock/unlock of mutex here
      if (SDL_mutexP(m_rtp_packet_mutex) == -1) {
	player_error_message("SDL Lock mutex failure in rtp bytestream recv");
	return;
      }
      add_rtp_packet_to_queue(rpak, &m_head, &m_tail);
      if (SDL_mutexV(m_rtp_packet_mutex) == -1) {
	player_error_message("SDL Lock mutex failure in rtp bytestream recv");
	return;
      }
    }
    break;
  case RX_SR:
    rtcp_sr *srpak;
    srpak = (rtcp_sr *)e->data;
    calculate_wallclock_offset_from_rtcp(srpak->ntp_frac, 
					 srpak->ntp_sec, 
					 srpak->rtp_ts);
    break;
  default:
#if 0
    player_debug_message("Thread %u - Callback from rtp with %d %p", 
			 SDL_ThreadID(),e->type, e->data);
#endif
    break;
    break;
  }
}
void CRtpByteStreamBase::remove_packet_rtp_queue (rtp_packet *pak, 
						       int free)
{
  SDL_LockMutex(m_rtp_packet_mutex);
  if ((m_head == pak) &&
      (m_head->next == NULL || m_head->next == m_head)) {
    m_head = NULL;
    m_tail = NULL;
  } else {
    pak->next->prev = pak->prev;
    pak->prev->next = pak->next;
    if (m_head == pak) {
      m_head = pak->next;
    }
    if (m_tail == pak) {
      m_tail = pak->prev;
    }
  }
  if (pak->data_len < 0) {
    // restore the packet data length
    pak->data_len = 0 - pak->data_len;
  }
  if (free == 1) {
    xfree(pak);
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
}

void CRtpByteStreamBase::flush_rtp_packets (void)
{
  while (m_head != NULL) {
    remove_packet_rtp_queue(m_head, 1);
  }
}

/*
 * recv_task - called from the player media rtp task - make sure
 * we have 2 seconds of buffering, then go...
 */
int CRtpByteStreamBase::recv_task (int decode_thread_waiting)
{
  /*
   * We need to make sure we have some buffering.  We'll buffer
   * about 2 seconds worth, then let the decode task know to go...
   */
  if (m_buffering == 0) {
    uint32_t head_ts, tail_ts;
    if (m_head != NULL) {
      /*
       * Make sure that the proto is the same
       */
      /*
       * Protocol the same.  Make sure we have at least 2 seconds of
       * good data
       */
      if (rtp_ready() == 0) {
	player_debug_message("Determined proto, but rtp bytestream is not ready");
	uint64_t calc;
	do {
	  head_ts = m_head->ts;
	  tail_ts = m_tail->ts;
	  calc = (tail_ts - head_ts);
	  calc *= 1000;
	  calc /= m_rtptime_tickpersec;
	  if (calc > m_rtp_buffer_time) {
	    rtp_packet *temp = m_head;
	    m_head = m_head->next;
	    m_tail->next = m_head;
	    m_head->prev = m_tail;
	    xfree((void *)temp);
	  }
	} while (calc > m_rtp_buffer_time);
	return 0;
      }
      if (check_rtp_frame_complete_for_proto()) {
	head_ts = m_head->ts;
	tail_ts = m_tail->ts;
	uint64_t calc;
	calc = (tail_ts - head_ts);
	calc *= M_LLU;
	calc /= m_rtptime_tickpersec;
	if (calc > m_rtp_buffer_time) {
	  if (m_rtp_rtpinfo_received == 0) {
	    player_debug_message("Setting rtp seq and time from 1st pak");
	    m_rtp_rtptime = m_head->ts;
	    m_rtp_rtpinfo_received = 1;
	  }
	  m_buffering = 1;
#if 1
	  player_debug_message("buffering complete - seq %d head %u tail %u "LLU, 
			       m_head->seq,
			       head_ts, tail_ts, calc);
#endif
	  
	}
      }
    }

  } else if (decode_thread_waiting != 0) {
    /*
   * We're good with buffering - but the decode thread might have
   * caught up, and will be waiting.  Post a message to kickstart
   * it
   */
    if (check_rtp_frame_complete_for_proto()) {
      return (1);
    }
  }
  return (m_buffering);
}

int CRtpByteStreamBase::check_rtp_frame_complete_for_proto (void)
{
  switch (m_rtp_proto) {
  case 14:
    return (m_head != NULL);
  default:
    return (m_head && m_tail->m == 1);
  }
}
