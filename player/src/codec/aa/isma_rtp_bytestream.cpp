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
#include "bitstream/bitstream.h"
#include "isma_rtp_bytestream.h"
#include "our_config_file.h"
#include "mpeg4_audio_config.h"
//#define DEBUG_ISMA_AAC
/*
 * I'm not sure that I'm happy with this.  For aac, we now use
 * the frame length in the packet.  We don't move to the next
 * packet until we get the start of frame. This causes have_no_data
 * to pass, but start_next_frame to fail.  I've gimmicked it up to 
 * work, but there might be a more elegant solution.
 */

/*
 * aac rtp bytestream basically works like a RTP byte stream; however, 
 * it uses local pointers for the frame and data, and doesn't adjust
 * the normal rtp byte stream values (m_offset_in_pak, m_pak) until the
 * start of the next frame.
 */
CIsmaAudioRtpByteStream::CIsmaAudioRtpByteStream (format_list_t *media_fmt,
						 fmtp_parse_t *fmtp,
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
  CRtpByteStreamBase(rtp_proto, ondemand, tps, head, tail, 
		     rtpinfo_received, rtp_rtptime, rtcp_received,
		     ntp_frac, ntp_sec, rtp_ts)
{
  m_frame_ptr = NULL;
  m_offset_in_frame = 0;
  m_frame_len = 0;
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  m_outfile = fopen("isma.aac", "w");
#endif
  m_pak_data_head = NULL;
  m_pak_data_free = NULL;
  isma_pak_data_t *p;
  for (m_pak_data_max = 0; m_pak_data_max < 25; m_pak_data_max++) {
    p = (isma_pak_data_t *)malloc(sizeof(isma_pak_data_t));
    p->pak_data_next = m_pak_data_free;
    m_pak_data_free = p;
  }
  mpeg4_audio_config_t audio_config;
  decode_mpeg4_audio_config(fmtp->config_binary,
			    fmtp->config_binary_len,
			    &audio_config);
  m_rtp_ts_add = audio_config.codec.aac.frame_len_1024 != 0 ? 1024 : 960;
  m_rtp_ts_add = (m_rtp_ts_add * media_fmt->rtpmap->clock_rate) /
    audio_config.frequency;
  player_debug_message("Rtp ts add is %d", m_rtp_ts_add);
}

CIsmaAudioRtpByteStream::~CIsmaAudioRtpByteStream (void)
{
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(m_outfile);
#endif
  isma_pak_data_t *p;
  
  while (m_pak_data_free != NULL) {
    p = m_pak_data_free;
    m_pak_data_free = p->pak_data_next;
    free(p);
  }
  while (m_pak_data_head != NULL) {
    p = m_pak_data_head;
    m_pak_data_head = p->pak_data_next;
    free(p);
  }
}

unsigned char CIsmaAudioRtpByteStream::get (void)
{
  unsigned char ret;

  if (m_pak == NULL || m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw "NULL when start";
  }

  if (m_offset_in_frame >= m_frame_len) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    throw("DECODE PAST END OF FRAME");
  }
  ret = m_frame_ptr[m_offset_in_frame];
  m_offset_in_frame++;
  m_total++;
  //check_for_end_of_pak();
  return (ret);
}

unsigned char CIsmaAudioRtpByteStream::peek (void) 
{
  return (m_frame_ptr ? m_frame_ptr[m_offset_in_frame] : 0);
}

void CIsmaAudioRtpByteStream::bookmark (int bSet)
{
  if (bSet == TRUE) {
    m_bookmark_set = 1;
    m_bookmark_offset_in_frame = m_offset_in_frame;
    m_total_book = m_total;
    //player_debug_message("bookmark on");
  } else {
    m_bookmark_set = 0;
    m_offset_in_frame = m_bookmark_offset_in_frame;
    m_total = m_total_book;
    //player_debug_message("book restore %d", m_offset_in_pak);
  }
}

int CIsmaAudioRtpByteStream::insert_pak_data (isma_pak_data_t *pak_data)
{
  if (m_pak_data_head == NULL) {
    m_pak_data_head = pak_data;
  } else {
    int32_t diff;
    isma_pak_data_t *p, *q;
    q = NULL;
    p = m_pak_data_head;

    do {
      diff = pak_data->rtp_timestamp - p->rtp_timestamp;
      if (diff == 0) {
	player_error_message("Duplicate timestamp of %x found in RTP packet",
			     pak_data->rtp_timestamp);
	player_debug_message("Seq number orig %d new %d", 
			     p->pak->seq, pak_data->pak->seq);
	pak_data->pak_data_next = m_pak_data_free;
	m_pak_data_free = pak_data;
	return 1;
      } else if (diff < 0) {
	if (q == NULL) {
	  pak_data->pak_data_next = m_pak_data_head;
	  m_pak_data_head = pak_data;
	} else {
	  q->pak_data_next = pak_data;
	  pak_data->pak_data_next = p;
	}
	return 0;
      }
      q = p;
      p = p->pak_data_next;
    } while (p != NULL);
    // insert at end;
    q->pak_data_next = pak_data;

  }
  return 0;
}

void CIsmaAudioRtpByteStream::remove_packet (rtp_packet *pak)
{
  SDL_mutexP(m_rtp_packet_mutex);
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
  SDL_mutexV(m_rtp_packet_mutex);
  xfree(pak);
}

void CIsmaAudioRtpByteStream::process_packet_header (void)
{
  rtp_packet *pak;
  uint32_t frame_len;
  uint16_t header_len;
  uint32_t retvalue;

  pak = m_head;
  if (pak == NULL) {
    return;
  }
  while (pak->data_len < 0 && pak != m_tail) {
    pak = pak->next;
  }
#ifdef DEBUG_ISMA_AAC
  player_debug_message("processing pak seq %d ts %x len %d", 
		       pak->seq, pak->ts, pak->data_len);
#endif
  // This pak has not had it's header processed
  // length in bytes
  header_len = ntohs(*(unsigned short *)pak->data) / 8;
  if (pak->data_len == 0 || header_len == 0) {
    // bye bye, frame...
    remove_packet(pak);
    return;
  }

  m_header_bitstream.init(&pak->data[sizeof(uint16_t)],
					 header_len);
  if (m_header_bitstream.getbits(13, frame_len) != 0) 
    return;
  m_header_bitstream.getbits(3, retvalue);
#ifdef DEBUG_ISMA_AAC
  player_debug_message("1st - header len %u frame len %u ts %x %p", 
		       header_len, frame_len, pak->ts, pak->data);
#endif
  if (frame_len == 0) {
    remove_packet(pak);
    return;
  }
  char *frame_ptr;
  isma_pak_data_t *pak_data;
  uint32_t ts;
  ts = pak->ts;
  pak_data = get_pak_data();
  pak_data->pak = pak;
  // frame pointer is after header_len + header_len size
  pak_data->frame_ptr = &pak->data[header_len + sizeof(uint16_t)];
  /*
   * Need to compare frame_len with pak->data_len, to make sure that
   * we don't have a fragment.  If so, probably want to indicate it, 
   * make sure that next packets are cool.
   */
  pak_data->frame_len = frame_len;
  pak_data->rtp_timestamp = ts;
  int error = insert_pak_data(pak_data);

  frame_ptr = pak_data->frame_ptr + pak_data->frame_len;
  while (m_header_bitstream.bits_remain() >= 16) {
    uint32_t stride;
    m_header_bitstream.getbits(13, frame_len);
    m_header_bitstream.getbits(3, stride);
    ts += (m_rtp_ts_add * (1 + stride));
#ifdef DEBUG_ISMA_AAC
    player_debug_message("Stride %d len %d ts %x", stride, frame_len, ts);
#endif
    pak_data = get_pak_data();
    pak_data->pak = pak;
    pak_data->frame_ptr = frame_ptr;
    pak_data->frame_len = frame_len;
    frame_ptr += frame_len;
    pak_data->rtp_timestamp = ts;
    error |= insert_pak_data(pak_data);
  }
  // Mark last packet, mark that we've used this packet by negating
  // the data length.
  pak->data_len = 0 - pak->data_len;
  if (error == 0 && pak_data != NULL) 
    pak_data->last_in_pak = 1;
  else {
    isma_pak_data_t *p, *last = NULL;
    p = m_pak_data_head;
    while (p != NULL) {
      if (p->pak == pak) last = p;
      p = p->pak_data_next;
    }
    if (last != NULL) {
      last->last_in_pak = 1;
      player_debug_message("error at end - marked ts %x", last->rtp_timestamp);
    } else {
      // Didn't find pak in list.  Weird
      player_error_message("Decoded packet with RTP timestamp %x and didn't"
			   "see any good frames", pak->ts);
      remove_packet(pak);
    }
  }
      
}

uint64_t CIsmaAudioRtpByteStream::start_next_frame (void)
{
  uint64_t timetick;
  
  if (m_pak_data_head != NULL) {
    uint32_t next_ts;
#ifdef DEBUG_ISMA_AAC
    player_debug_message("Advancing to next pak data - old ts %x", 
			 m_pak_data_head->rtp_timestamp);
#endif
    if (m_pak_data_head->last_in_pak != 0) {
#ifdef DEBUG_ISMA_AAC
      player_debug_message("removing pak");
#endif
      rtp_packet *pak = m_pak_data_head->pak;
      remove_packet(pak);
    }
    isma_pak_data_t *p;
    p = m_pak_data_head;
    next_ts = p->rtp_timestamp;
    m_pak_data_head = p->pak_data_next;
    p->pak_data_next = m_pak_data_free;
    m_pak_data_free = p;
    // Now, look for the next timestamp
    next_ts += m_rtp_ts_add;
    if (m_pak_data_head == NULL || m_pak_data_head->rtp_timestamp != next_ts) {
      // process next pak in list.  Process until next timestamp is found, 
      // or 500 msec worth of data is found (in which case, go with first)
      do {
	process_packet_header();
      } while (((m_pak_data_head == NULL) || 
		(m_pak_data_head->rtp_timestamp != next_ts)) &&  
	       (m_pak_data_free != NULL));
    } else {
      // m_pak_data_head is correct
    }
  } else {
    // first time.  Process a bunch of packets, go with first one...
    do {
      process_packet_header();
    } while (m_pak_data_free != NULL);
  }
    
  m_offset_in_frame = 0;
  if (m_pak_data_head != NULL) {
    m_frame_ptr = m_pak_data_head->frame_ptr;
    m_frame_len = m_pak_data_head->frame_len;
  } else {
    m_frame_ptr = NULL;
    m_frame_len = 0;
  }

#ifdef DEBUG_ISMA_AAC
  player_debug_message("start next frame %p %d", m_frame_ptr, m_frame_len);
#endif
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  if (m_frame_ptr != NULL) {
    fwrite(m_frame_ptr, m_frame_len, 1, m_outfile);
  }
#endif

  // We're going to have to handle wrap better...
  if (m_stream_ondemand) {
    int neg = 0;
    if (m_pak_data_head != NULL)
      m_ts = m_pak_data_head->rtp_timestamp;
    
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

    if (m_pak_data_head != NULL) {
      if (((m_ts & 0x80000000) == 0x80000000) &&
	  ((m_pak_data_head->rtp_timestamp & 0x80000000) == 0)) {
	m_wrap_offset += (I_LLU << 32);
      }
      m_ts = m_pak_data_head->rtp_timestamp;
    }
    timetick = m_ts + m_wrap_offset;
    timetick *= 1000;
    timetick /= m_rtptime_tickpersec;
    timetick += m_wallclock_offset;
#if 0
	player_debug_message("time %x "LLX" "LLX" "LLX" "LLX,
			m_ts, m_wrap_offset, m_rtptime_tickpersec, m_wallclock_offset,
			timetick);
#endif
  }
  return (timetick);
					   
}

size_t CIsmaAudioRtpByteStream::read (unsigned char *buffer, size_t bytes_to_read)
{
  size_t inbuffer;

  if (m_pak == NULL || m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw "NULL when start";
  }

  inbuffer = m_frame_len - m_offset_in_frame;
  if (inbuffer < bytes_to_read) {
    bytes_to_read = inbuffer;
  }
  memcpy(buffer, &m_frame_ptr[m_offset_in_frame], bytes_to_read);
  m_offset_in_frame += bytes_to_read;
  m_total += bytes_to_read;
  return (bytes_to_read);
}

void CIsmaAudioRtpByteStream::reset (void)
{
  isma_pak_data_t *p;
  player_debug_message("Hit isma rtp reset");
  if (m_pak_data_head != NULL) {
    p = m_pak_data_head;
    while (p->pak_data_next != NULL) {
      p = p->pak_data_next;
    }
    p->pak_data_next = m_pak_data_free;
    m_pak_data_free = m_pak_data_head;
    m_pak_data_head = NULL;
  }
  m_frame_ptr = NULL;
  m_offset_in_frame = 0;
  m_frame_len = 0;
  CRtpByteStreamBase::reset();
}
