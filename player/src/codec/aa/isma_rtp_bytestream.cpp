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
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
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
  m_fmtp = *fmtp;
  m_min_first_header_bits = m_fmtp.size_length + m_fmtp.index_length;
  m_min_header_bits = m_fmtp.size_length + m_fmtp.index_delta_length;
  if (m_fmtp.CTS_delta_length > 0) {
    m_min_header_bits++;
    m_min_first_header_bits++;
  }
  if (m_fmtp.DTS_delta_length > 0) {
    m_min_header_bits++;
    m_min_first_header_bits++;
  }
  player_debug_message("min headers are %d %d", m_min_first_header_bits,
		       m_min_header_bits);
    
  m_min_header_bits += m_fmtp.auxiliary_data_size_length;
  m_min_first_header_bits += m_fmtp.auxiliary_data_size_length;
  
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

  if (m_frame_ptr == NULL) {
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

size_t CIsmaAudioRtpByteStream::read (unsigned char *buffer, 
				      size_t bytes_to_read)
{
  size_t inbuffer;

  if (m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw "NULL when start - read";
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

int CIsmaAudioRtpByteStream::insert_pak_data (isma_pak_data_t *pak_data)
{
  SDL_LockMutex(m_rtp_packet_mutex);
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
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return 1;
      } else if (diff < 0) {
	if (q == NULL) {
	  pak_data->pak_data_next = m_pak_data_head;
	  m_pak_data_head = pak_data;
	} else {
	  q->pak_data_next = pak_data;
	  pak_data->pak_data_next = p;
	}
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return 0;
      }
      q = p;
      p = p->pak_data_next;
    } while (p != NULL);
    // insert at end;
    q->pak_data_next = pak_data;

  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return 0;
}

void CIsmaAudioRtpByteStream::get_au_header_bits (void)
{
  uint32_t temp;
  if (m_fmtp.CTS_delta_length > 0) {
    m_header_bitstream.getbits(1, temp);
    if (temp > 0) {
      m_header_bitstream.getbits(m_fmtp.CTS_delta_length, temp);
    }
  }
  if (m_fmtp.DTS_delta_length > 0) {
    m_header_bitstream.getbits(1, temp);
    if (temp > 0) {
      m_header_bitstream.getbits(m_fmtp.DTS_delta_length, temp);
    }
  }
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
#ifdef DEBUG_ISMA_AAC
  player_debug_message("processing pak seq %d ts %x len %d", 
		       pak->seq, pak->ts, pak->data_len);
#endif
  // This pak has not had it's header processed
  // length in bytes
  header_len = ntohs(*(unsigned short *)pak->data);
  if (pak->data_len == 0 || header_len < m_min_first_header_bits) {
    // bye bye, frame...
    remove_packet_rtp_queue(pak, 1);
    return;
  }

  m_header_bitstream.init(&pak->data[sizeof(uint16_t)],
					 header_len);
  if (m_header_bitstream.getbits(m_fmtp.size_length, frame_len) != 0) 
    return;
  m_header_bitstream.getbits(m_fmtp.index_length, retvalue);
  get_au_header_bits();
#ifdef DEBUG_ISMA_AAC
  uint64_t wrap_offset = m_wrap_offset;
  uint64_t msec = rtp_ts_to_msec(pak->ts, wrap_offset);
  player_debug_message("1st - header len %u frame len %u ts %x %llu", 
		       header_len, frame_len, pak->ts, msec);
#endif
  if (frame_len == 0) {
    remove_packet_rtp_queue(pak, 1);
    return;
  }
  char *frame_ptr;
  isma_pak_data_t *pak_data;
  uint32_t ts;
  ts = pak->ts;
  pak_data = get_pak_data();
  pak_data->pak = pak;
  // frame pointer is after header_len + header_len size.  Header_len
  // is in bits - add 7, divide by 8 to get padding correctly.
  pak_data->frame_ptr = &pak->data[((header_len + 7) / 8) + sizeof(uint16_t)];
  /*
   * Need to compare frame_len with pak->data_len, to make sure that
   * we don't have a fragment.  If so, probably want to indicate it, 
   * make sure that next packets are cool.
   */
  pak_data->frame_len = frame_len;
  pak_data->rtp_timestamp = ts;
  int error = insert_pak_data(pak_data);

  frame_ptr = pak_data->frame_ptr + pak_data->frame_len;
  while (m_header_bitstream.bits_remain() >= m_min_header_bits) {
    uint32_t stride;
    m_header_bitstream.getbits(m_fmtp.size_length, frame_len);
    m_header_bitstream.getbits(m_fmtp.index_delta_length, stride);
    get_au_header_bits();
    ts += (m_rtp_ts_add * (1 + stride));
#ifdef DEBUG_ISMA_AAC
    wrap_offset = m_wrap_offset;
    msec = rtp_ts_to_msec(ts, wrap_offset);
    player_debug_message("Stride %d len %d ts %x %llu", stride, frame_len, ts, msec);
#endif
    pak_data = get_pak_data();
    pak_data->pak = pak;
    pak_data->frame_ptr = frame_ptr;
    pak_data->frame_len = frame_len;
    frame_ptr += frame_len;
    pak_data->rtp_timestamp = ts;
    error |= insert_pak_data(pak_data);
  }
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
      remove_packet_rtp_queue(pak, 1);
      return;
    }
  }
  if (m_fmtp.auxiliary_data_size_length > 0) {
    m_header_bitstream.byte_align();
    size_t aux_len;
    m_header_bitstream.getbits(m_fmtp.auxiliary_data_size_length, aux_len);
    aux_len = (aux_len + 7) / 8;
#ifdef DEBUG_ISMA_AAC
    player_debug_message("Adding %d bytes for aux data size", 
			 aux_len);
#endif
    isma_pak_data_t *p;
    p = m_pak_data_head;
    while (p != NULL) {
      if (p->pak == pak) {
	p->frame_ptr += aux_len;
      }
      p = p->pak_data_next;
    }
  }
    
  remove_packet_rtp_queue(pak, 0);
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
      rtp_packet *pak = m_pak_data_head->pak;
      m_pak_data_head->pak = NULL;
      xfree(pak);
#ifdef DEBUG_ISMA_AAC
      player_debug_message("removing pak %d", pak->seq);
#endif
    }
    isma_pak_data_t *p;
    SDL_LockMutex(m_rtp_packet_mutex);
    p = m_pak_data_head;
    next_ts = p->rtp_timestamp;
    m_pak_data_head = p->pak_data_next;
    p->pak_data_next = m_pak_data_free;
    m_pak_data_free = p;
    SDL_UnlockMutex(m_rtp_packet_mutex);
    // Now, look for the next timestamp
    next_ts += m_rtp_ts_add;
    if (m_pak_data_head == NULL || m_pak_data_head->rtp_timestamp != next_ts) {
      // process next pak in list.  Process until next timestamp is found, 
      // or 500 msec worth of data is found (in which case, go with first)
      do {
	process_packet_header();
      } while (m_head != NULL && 
	       ((m_pak_data_head == NULL) || 
		(m_pak_data_head->rtp_timestamp != next_ts)) &&  
	       (m_pak_data_free != NULL));
    } else {
      // m_pak_data_head is correct
    }
  } else {
    // first time.  Process a bunch of packets, go with first one...
    // asdf - will want to eventually add code to drop the first couple
    // of packets if they're not consecutive.
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

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  if (m_frame_ptr != NULL) {
    fwrite(m_frame_ptr, m_frame_len, 1, m_outfile);
  }
#endif
  timetick = rtp_ts_to_msec(m_pak_data_head != NULL ? 
			    m_pak_data_head->rtp_timestamp : m_ts, 
			    m_wrap_offset);
  if (m_pak_data_head != NULL)
    m_ts =  m_pak_data_head->rtp_timestamp;
  // We're going to have to handle wrap better...
#ifdef DEBUG_ISMA_AAC
  player_debug_message("start next frame %p %d ts %x "LLU, 
		       m_frame_ptr, m_frame_len, m_ts, timetick);
#endif
  return (timetick);
					   
}


void CIsmaAudioRtpByteStream::flush_rtp_packets (void)
{
  isma_pak_data_t *p;
  SDL_LockMutex(m_rtp_packet_mutex);
  if (m_pak_data_head != NULL) {
    p = m_pak_data_head;
    while (p->pak_data_next != NULL) {
#ifdef DEBUG_ISMA_AAC
      player_debug_message("reset removing pak %d", p->pak->seq);
#endif
      if (p->last_in_pak != 0) {
	xfree(p->pak);
      }
      p = p->pak_data_next;
    }
    p->pak_data_next = m_pak_data_free;
    m_pak_data_free = m_pak_data_head;
    m_pak_data_head = NULL;
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  CRtpByteStreamBase::flush_rtp_packets();
}

void CIsmaAudioRtpByteStream::reset (void)
{
  player_debug_message("Hit isma rtp reset");
  m_frame_ptr = NULL;
  m_offset_in_frame = 0;
  m_frame_len = 0;
  CRtpByteStreamBase::reset();
}

uint64_t CIsmaAudioRtpByteStream::rtp_ts_to_msec (uint32_t ts,
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

int CIsmaAudioRtpByteStream::have_no_data (void)
{
  return (m_head == NULL && m_pak_data_head == NULL);
}
