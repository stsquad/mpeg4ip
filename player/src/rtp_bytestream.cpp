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
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(rtp_message, "rtpbyst")
#else
#define rtp_message(loglevel, fmt...) message(loglevel, "rtpbyst", fmt)
#endif
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
  rtp_message(LOG_DEBUG, "CBThread %u - m %u pt %u seq %d ts %d len %d", 
	      SDL_ThreadID(),
	      pak->rtp_pak_m, pak->rtp_pak_pt, pak->rtp_pak_seq, 
	      pak->rtp_pak_ts, pak->rtp_data_len);
#endif
  
  if (*head == NULL) {
    *head = *tail = pak;
  } else if (*head == *tail) {
    if (pak->rtp_pak_seq == (*head)->rtp_pak_seq) {
      rtp_message(LOG_ERR, "Duplicate RTP sequence number %d received", 
		  pak->rtp_pak_seq);
      inserted = FALSE;
    } else {
      (*head)->rtp_next = pak;
      (*head)->rtp_prev = pak;
      pak->rtp_next = pak->rtp_prev = *head;
      diff = pak->rtp_pak_seq - (*head)->rtp_pak_seq;
      if (diff > 0) {
	*tail = pak;
      } else {
	*head = pak;
      }
    }
  } else if ((((*head)->rtp_pak_seq < (*tail)->rtp_pak_seq) &&
	      ((pak->rtp_pak_seq > (*tail)->rtp_pak_seq) || 
	       (pak->rtp_pak_seq < (*head)->rtp_pak_seq))) ||
	     (((*head)->rtp_pak_seq > (*tail)->rtp_pak_seq) &&
	      ((pak->rtp_pak_seq > (*tail)->rtp_pak_seq && 
		pak->rtp_pak_seq < (*head)->rtp_pak_seq)))) {
    // insert between tail and head
    // Maybe move head - probably move tail.
    (*tail)->rtp_next = pak;
    pak->rtp_prev = *tail;
    (*head)->rtp_prev = pak;
    pak->rtp_next = *head;
    diff = (*head)->rtp_pak_seq - pak->rtp_pak_seq;
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
      if (pak->rtp_pak_seq == q->rtp_pak_seq || pak->rtp_pak_seq == q->rtp_next->rtp_pak_seq) {
	// dup seq number
	inserted = FALSE;
	break;
      }
      /*
       * Okay - this is disgusting, but works.  The first part (before the
       * or) will see if pak->rtp_pak_seq is between q and q->rtp_next, 
       * assuming that the sequence number for q and q->rtp_next are ascending.
       *
       * 2nd part of the if is the converse case (q->rtp_next->rtp_pak_seq 
       * is smaller than q->rtp_pak_seq).  In that case, we need to make 
       * sure that pak->rtp_pak_seq is either larger than q->rtp_pak_seq 
       * or less than q->rtp_next->rtp_pak_seq
       */
      if (((q->rtp_next->rtp_pak_seq > q->rtp_pak_seq) &&
	   (q->rtp_pak_seq < pak->rtp_pak_seq && 
	    pak->rtp_pak_seq < q->rtp_next->rtp_pak_seq)) ||
	  ((q->rtp_next->rtp_pak_seq < q->rtp_pak_seq) &&
	   (pak->rtp_pak_seq < q->rtp_next->rtp_pak_seq || 
	    pak->rtp_pak_seq > q->rtp_pak_seq))) {
	q->rtp_next->rtp_prev = pak;
	pak->rtp_next = q->rtp_next;
	q->rtp_next = pak;
	pak->rtp_prev = q;
	break;
      }
      q = q->rtp_next;
    } while (q != *tail);
    if (q == *tail) {
      inserted = FALSE;
      rtp_message(LOG_ERR, "Couldn't insert %u between %u and %u", 
		  pak->rtp_pak_seq, 
		  (*head)->rtp_pak_seq, 
		  (*tail)->rtp_pak_seq);
    }
  }

  if (inserted == FALSE) {
    rtp_message(LOG_ERR, "Couldn't insert pak");
    rtp_message(LOG_DEBUG, "pak seq %u", pak->rtp_pak_seq);
    if (*head != NULL) {
      rtp_message(LOG_DEBUG, "head seq %u, tail seq %u", 
		  (*head)->rtp_pak_seq,
		  (*tail)->rtp_pak_seq);
    }
    xfree(pak);
    return (0);
  }
  return (1);
}

CRtpByteStreamBase::CRtpByteStreamBase(const char *name,
				       unsigned int rtp_proto,
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
  COurInByteStream(name)
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
  m_offset_in_pak = m_skip_on_advance_bytes;
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
  rtp_message(LOG_DEBUG, "wallclock offset is %llu - prev %llu", 
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
    if (rpak->rtp_data_len == 0) {
      xfree(rpak);
    } else {
      // need to add lock/unlock of mutex here
      if (SDL_mutexP(m_rtp_packet_mutex) == -1) {
	rtp_message(LOG_CRIT, "SDL Lock mutex failure in rtp bytestream recv");
	return;
      }
      add_rtp_packet_to_queue(rpak, &m_head, &m_tail);
      if (SDL_mutexV(m_rtp_packet_mutex) == -1) {
	rtp_message(LOG_CRIT, "SDL Lock mutex failure in rtp bytestream recv");
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
    rtp_message(LOG_DEBUG, "Thread %u - Callback from rtp with %d %p", 
		SDL_ThreadID(),e->type, e->rtp_data);
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
      (m_head->rtp_next == NULL || m_head->rtp_next == m_head)) {
    m_head = NULL;
    m_tail = NULL;
  } else {
    pak->rtp_next->rtp_prev = pak->rtp_prev;
    pak->rtp_prev->rtp_next = pak->rtp_next;
    if (m_head == pak) {
      m_head = pak->rtp_next;
    }
    if (m_tail == pak) {
      m_tail = pak->rtp_prev;
    }
  }
  if (pak->rtp_data_len < 0) {
    // restore the packet data length
    pak->rtp_data_len = 0 - pak->rtp_data_len;
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
	rtp_message(LOG_DEBUG, 
		    "Determined proto, but rtp bytestream is not ready");
	uint64_t calc;
	do {
	  head_ts = m_head->rtp_pak_ts;
	  tail_ts = m_tail->rtp_pak_ts;
	  calc = (tail_ts - head_ts);
	  calc *= 1000;
	  calc /= m_rtptime_tickpersec;
	  if (calc > m_rtp_buffer_time) {
	    rtp_packet *temp = m_head;
	    m_head = m_head->rtp_next;
	    m_tail->rtp_next = m_head;
	    m_head->rtp_prev = m_tail;
	    xfree((void *)temp);
	  }
	} while (calc > m_rtp_buffer_time);
	return 0;
      }
      if (check_rtp_frame_complete_for_proto()) {
	head_ts = m_head->rtp_pak_ts;
	tail_ts = m_tail->rtp_pak_ts;
	uint64_t calc;
	calc = (tail_ts - head_ts);
	calc *= M_LLU;
	calc /= m_rtptime_tickpersec;
	if (calc > m_rtp_buffer_time) {
	  if (m_rtp_rtpinfo_received == 0) {
	    rtp_message(LOG_NOTICE, "Setting rtp seq and time from 1st pak");
	    m_rtp_rtptime = m_head->rtp_pak_ts;
	    m_rtp_rtpinfo_received = 1;
	  }
	  m_buffering = 1;
#if 1
	  rtp_message(LOG_INFO, 
		      "%s buffering complete - seq %d head %u tail %u "LLU, 
		      m_name, m_head->rtp_pak_seq,
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
  return (m_head && m_tail->rtp_pak_m == 1);
}

uint64_t CRtpByteStreamBase::rtp_ts_to_msec (uint32_t ts,
					     uint64_t &wrap_offset)
{
  uint64_t timetick;

  if (m_stream_ondemand) {
    int neg = 0;
    
    if (ts >= m_rtp_rtptime) {
      timetick = ts;
      timetick -= m_rtp_rtptime;
    } else {
      timetick = m_rtp_rtptime - ts;
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
	((ts & 0x80000000) == 0)) {
      wrap_offset += (I_LLU << 32);
    }
    timetick = ts + m_wrap_offset;
    timetick *= 1000;
    timetick /= m_rtptime_tickpersec;
    timetick += m_wallclock_offset;
  }
  return (timetick);
}


CRtpByteStream::CRtpByteStream(const char *name,
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
			       uint32_t rtp_ts) :
  CRtpByteStreamBase(name, rtp_proto, ondemand, tickpersec, head, tail,
		     rtpinfo_received, rtp_rtptime, rtcp_received,
		     ntp_frac, ntp_sec, rtp_ts)
{
  m_buffer = (unsigned char *)malloc(4096);
  m_buffer_len_max = 4096;
  m_bytes_used = m_buffer_len = 0;
}

CRtpByteStream::~CRtpByteStream (void)
{
  free(m_buffer);
  m_buffer = NULL;
}

uint64_t CRtpByteStream::start_next_frame (unsigned char **buffer, 
					   uint32_t *buflen)
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
    ts = m_ts;
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
      if ((m_buffer_len + rpak->rtp_data_len) > m_buffer_len_max) {
	// realloc
	m_buffer_len_max = m_buffer_len + rpak->rtp_data_len + 1024;
	m_buffer = (unsigned char *)realloc(m_buffer, m_buffer_len_max);
      }
      memcpy(m_buffer + m_buffer_len, 
	     rpak->rtp_data,
	     rpak->rtp_data_len);
      m_buffer_len += rpak->rtp_data_len;
      if (rpak->rtp_pak_m == 1) {
	finished = 1;
      }
      xfree(rpak);
    }
    m_bytes_used = m_skip_on_advance_bytes;
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

int CRtpByteStream::skip_next_frame (uint64_t *pts, int *hasSyncFrame,
				     unsigned char **buffer, 
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
  ts = start_next_frame(buffer, buflen);
  *pts = ts;
  return (1);
}

void CRtpByteStream::used_bytes_for_frame (uint32_t bytes)
{
  m_bytes_used += bytes;
#ifdef DEBUG_RTP_PAKS
  rtp_message(LOG_DEBUG, "%s Used %d bytes", m_name, bytes);
#endif
}

int CRtpByteStream::have_no_data (void)
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

void CRtpByteStream::flush_rtp_packets (void)
{
  CRtpByteStreamBase::flush_rtp_packets();

  m_bytes_used = m_buffer_len = 0;
}

CAudioRtpByteStream::CAudioRtpByteStream (unsigned int rtp_proto,
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
  CRtpByteStream("audio", 
		 rtp_proto,
		 ondemand,
		 tps,
		 head, 
		 tail,
		 rtpinfo_received,
		 rtp_rtptime,
		 rtcp_received,
		 ntp_frac,
		 ntp_sec,
		 rtp_ts)
{
  init();
  m_working_pak = NULL;
}

CAudioRtpByteStream::~CAudioRtpByteStream(void)
{
}

int CAudioRtpByteStream::have_no_data (void)
{
  if (m_head == NULL) return TRUE;
  return FALSE;
}

int CAudioRtpByteStream::check_rtp_frame_complete_for_proto (void)
{
  return m_head != NULL;
}

uint64_t CAudioRtpByteStream::start_next_frame (unsigned char **buffer, 
						uint32_t *buflen)
{
  uint32_t ts;
  int32_t diff;

  if (m_working_pak != NULL) {
    diff = m_working_pak->rtp_data_len - m_bytes_used;
  } else diff = 0;

  m_doing_add = 0;
  if (diff > 2) {
    // Still bytes in the buffer...
    *buffer = (unsigned char *)m_working_pak->rtp_data + m_bytes_used;
    *buflen = diff;
    ts = m_ts;
#ifdef DEBUG_RTP_PAKS
    rtp_message(LOG_DEBUG, "%s Still left - %d bytes", m_name, *buflen);
#endif
  } else {
    if (m_working_pak) xfree(m_working_pak);
    m_buffer_len = 0;
    m_working_pak = m_head;
    remove_packet_rtp_queue(m_working_pak, 0);
    *buffer = (unsigned char *)m_working_pak->rtp_data;
    *buflen = m_working_pak->rtp_data_len;
    ts = m_working_pak->rtp_pak_ts;
#ifdef DEBUG_RTP_PAKS
    rtp_message(LOG_DEBUG, "%s buffer seq %d ts %x len %d", m_name, 
		m_working_pak->rtp_pak_seq, 
		m_working_pak->rtp_pak_ts, m_buffer_len);
#endif
  }

  // We're going to have to handle wrap better...
  m_ts = ts;
  return rtp_ts_to_msec(ts, m_wrap_offset);

}
