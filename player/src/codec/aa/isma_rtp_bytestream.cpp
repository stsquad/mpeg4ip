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
#include <mp4util/mpeg4_audio_config.h>
//#define DEBUG_ISMA_AAC

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(isma_message, "ismartp")
#else
#define isma_message(loglevel, fmt...) message(loglevel, "ismartp", fmt)
#endif

/*
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
CIsmaAudioRtpByteStream::CIsmaAudioRtpByteStream (format_list_t *media_fmt,
						  fmtp_parse_t *fmtp,
						  unsigned int rtp_pt,
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
  CRtpByteStreamBase("ismaaac", rtp_pt, ondemand, tps, head, tail, 
		     rtpinfo_received, rtp_rtptime, rtcp_received,
		     ntp_frac, ntp_sec, rtp_ts)
{
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  m_outfile = fopen("isma.aac", "w");
#endif
  m_frame_data_head = NULL;
  m_frame_data_on = NULL;
  m_frame_data_free = NULL;
  isma_frame_data_t *p;
  for (m_frame_data_max = 0; m_frame_data_max < 25; m_frame_data_max++) {
    p = (isma_frame_data_t *)malloc(sizeof(isma_frame_data_t));
    p->frame_data_next = m_frame_data_free;
    m_frame_data_free = p;
  }
  mpeg4_audio_config_t audio_config;
  decode_mpeg4_audio_config(fmtp->config_binary,
			    fmtp->config_binary_len,
			    &audio_config);
  m_rtp_ts_add = audio_config.codec.aac.frame_len_1024 != 0 ? 1024 : 960;
  m_rtp_ts_add = (m_rtp_ts_add * media_fmt->rtpmap->clock_rate) /
    audio_config.frequency;
  isma_message(LOG_DEBUG, 
	       "Rtp ts add is %d (%d %d)", m_rtp_ts_add,
	       media_fmt->rtpmap->clock_rate, 
	       audio_config.frequency);
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
  isma_message(LOG_DEBUG, "min headers are %d %d", m_min_first_header_bits,
	       m_min_header_bits);

  m_min_header_bits += m_fmtp.auxiliary_data_size_length;
  m_min_first_header_bits += m_fmtp.auxiliary_data_size_length;
  m_frag_reass_buffer = NULL;
  m_frag_reass_size_max = 0;
}

CIsmaAudioRtpByteStream::~CIsmaAudioRtpByteStream (void)
{
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(m_outfile);
#endif
  isma_frame_data_t *p;
  
  if (m_frag_reass_buffer != NULL) {
    free(m_frag_reass_buffer);
    m_frag_reass_buffer = NULL;
  }
  if (m_frame_data_on != NULL) {
    m_frame_data_on->frame_data_next = m_frame_data_head;
    m_frame_data_head = m_frame_data_on;
    m_frame_data_on = NULL;
  }
  while (m_frame_data_free != NULL) {
    p = m_frame_data_free;
    m_frame_data_free = p->frame_data_next;
    free(p);
  }
  while (m_frame_data_head != NULL) {
    p = m_frame_data_head;
    // if fragmented frame, free all frag_data
    if (p->is_fragment == 1) {
      isma_frag_data_t * q = p->frag_data;
      while (q != NULL) {
	p->frag_data = q->frag_data_next;
	free(q);
	q = p->frag_data;
      }
    }
    m_frame_data_head = p->frame_data_next;
    free(p);
  }
}

int CIsmaAudioRtpByteStream::insert_frame_data (isma_frame_data_t *frame_data)
{
  SDL_LockMutex(m_rtp_packet_mutex);
  if (m_frame_data_head == NULL) {
    m_frame_data_head = frame_data;
  } else {
    int32_t diff;
    isma_frame_data_t *p, *q;
    q = NULL;
    p = m_frame_data_head;

    do {
      diff = frame_data->rtp_timestamp - p->rtp_timestamp;
      if (diff == 0) {
	isma_message(LOG_ERR, "Duplicate timestamp of %x found in RTP packet",
		     frame_data->rtp_timestamp);
	isma_message(LOG_DEBUG, 
		     "Seq number orig %d new %d", 
		     p->pak->rtp_pak_seq, frame_data->pak->rtp_pak_seq); 
	// if fragmented frame, free all frag_data
	if (frame_data->is_fragment == 1) {
	  isma_frag_data_t * p = NULL;
	  while ((p = frame_data->frag_data) != NULL) {
	    frame_data->frag_data = p->frag_data_next;
	    free(p);
	  }
	}
	// put frame_data on free list
	frame_data->frame_data_next = m_frame_data_free;
	m_frame_data_free = frame_data;

	SDL_UnlockMutex(m_rtp_packet_mutex);
	return 1;
      } else if (diff < 0) {
	if (q == NULL) {
	  frame_data->frame_data_next = m_frame_data_head;
	  m_frame_data_head = frame_data;
	} else {
	  q->frame_data_next = frame_data;
	  frame_data->frame_data_next = p;
	}
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return 0;
      }
      q = p;
      p = p->frame_data_next;
    } while (p != NULL);
    // insert at end;
    q->frame_data_next = frame_data;

  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return 0;
}

void CIsmaAudioRtpByteStream::get_au_header_bits (void)
{
  uint32_t temp;
  if (m_fmtp.CTS_delta_length > 0) {
    m_header_bitstream.getbits(1, &temp);
    if (temp > 0) {
      m_header_bitstream.getbits(m_fmtp.CTS_delta_length, &temp);
    }
  }
  if (m_fmtp.DTS_delta_length > 0) {
    m_header_bitstream.getbits(1, &temp);
    if (temp > 0) {
      m_header_bitstream.getbits(m_fmtp.DTS_delta_length, &temp);
    }
  }
}

// check where need to lock 
void CIsmaAudioRtpByteStream::cleanup_frag (isma_frame_data_t * frame_data)
{
  // free all frag_data for this frame
  isma_frag_data_t * p = NULL;
  while ((p = frame_data->frag_data) != NULL) {
    frame_data->frag_data = p->frag_data_next;
    free(p);
  } 
  // now put frame_data back on free list
  SDL_LockMutex(m_rtp_packet_mutex);
  frame_data->frame_data_next = m_frame_data_free;
  m_frame_data_free = frame_data;
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return;
}

// Frame is fragmented.
// Process next RTP paks until have the entire frame.
// Paks will be in order in the queue, but maybe some missing?
// So if process pkt w/ Mbit set (last of fragm) and total < frameLength
// ignore pkt.
// Insert frame data only after got all fragments for the frame.
int CIsmaAudioRtpByteStream::process_fragment (rtp_packet *pak, 
					       isma_frame_data_t *frame_data)
{
  uint16_t seq = pak->rtp_pak_seq; 
  uint32_t ts = pak->rtp_pak_ts;
  isma_frag_data_t *cur = NULL;
  int read_mBit = 0;
  uint32_t total_len = 0; 
  frame_data->is_fragment = 1;
  do {
    if (read_mBit == 1) {
      // get rid of frame_data - last frag seen but total length wrong
      cleanup_frag(frame_data);
      isma_message(LOG_ERR, "Error processing frag: early mBit");
      return (1); 
    }
    if (pak == NULL) {
      cleanup_frag(frame_data);
      isma_message(LOG_ERR, "Error processing frag: not enough packets");
      return (1);
    }
    // check if ts and rtp seq numbers are ok, and lengths match
    if (ts != pak->rtp_pak_ts) {
      cleanup_frag(frame_data);
      isma_message(LOG_ERR, 
		   "Error processing frag: wrong ts: ts= %x, pak->ts = %x", 
		   ts, pak->rtp_pak_ts);
      return (1);
    }
    if (seq != pak->rtp_pak_seq) {
      cleanup_frag(frame_data);
      isma_message(LOG_ERR, "Error processing frag: wrong seq num");
      return (1);
    }
    // insert fragment info
    isma_frag_data_t *p = (isma_frag_data_t *)
      malloc(sizeof(isma_frag_data_t));
    if (p == NULL) {
      isma_message(LOG_ERR, "Error processing frag: can't malloc");
      remove_packet_rtp_queue(pak, 0);
      return (1);
    }
    if (cur == NULL) {
      frame_data->frag_data = p;
      cur = p;		
    } else {
      cur->frag_data_next = p;
      cur = p;
    }
    cur->frag_data_next = NULL;
    cur->pak = pak; 
    // length in bits
    uint16_t header_len = ntohs(*(unsigned short *)pak->rtp_data);
    m_header_bitstream.init(&pak->rtp_data[sizeof(uint16_t)], header_len);
    // frag_ptr should just point to beginning of data in pkt
    uint32_t header_len_bytes = ((header_len + 7) / 8) + sizeof(uint16_t);
    cur->frag_ptr =  &pak->rtp_data[header_len_bytes];
    cur->frag_len = pak->rtp_data_len - header_len_bytes;
    // if aux data, move frag pointer
    if (m_fmtp.auxiliary_data_size_length > 0) {
      m_header_bitstream.byte_align();
      uint32_t aux_len;
      m_header_bitstream.getbits(m_fmtp.auxiliary_data_size_length, &aux_len);
      aux_len = (aux_len + 7) / 8;
      cur->frag_ptr += aux_len;
      cur->frag_len -= aux_len;
    }
    total_len += cur->frag_len;
#ifdef DEBUG_ISMA_RTP_FRAGS
    isma_message(LOG_DEBUG, 
		 "rtp seq# %d, fraglen: %d, ts: %x", 
		 pak->seq, cur->frag_len, pak->ts);
#endif	
    seq = pak->rtp_pak_seq + 1;
    if (pak->rtp_pak_m) 
      read_mBit = 1;
    remove_packet_rtp_queue(pak, 0);
    pak = m_head; // get next pkt in the queue
  } while (total_len < frame_data->frame_len);

  // insert frame and return
  int error = insert_frame_data(frame_data);
  frame_data->last_in_pak = 1; // only one frame in pak
  // no need to remove pkt from queue, done at the end of do-while
  return (error);
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
  isma_message(LOG_DEBUG, 
	       "processing pak seq %d ts %x len %d", 
	       pak->rtp_pak_seq, pak->rtp_pak_ts, pak->rtp_data_len);
#endif
  // This pak has not had it's header processed
  // length in bytes
  if (pak->rtp_data_len == 0) {
    remove_packet_rtp_queue(pak, 1);
    isma_message(LOG_ERR, "RTP audio packet with data length of 0");
    return;
  }

  header_len = ntohs(*(unsigned short *)pak->rtp_data);
  if (header_len < m_min_first_header_bits) {
    // bye bye, frame...
    remove_packet_rtp_queue(pak, 1);
    isma_message(LOG_ERR, "ISMA rtp - header len %d less than min %d", 
		 header_len, m_min_first_header_bits);
    return;
  }

  m_header_bitstream.init(&pak->rtp_data[sizeof(uint16_t)],
			  header_len);
  if (m_header_bitstream.getbits(m_fmtp.size_length, &frame_len) != 0) 
    return;
  m_header_bitstream.getbits(m_fmtp.index_length, &retvalue);
  get_au_header_bits();
#ifdef DEBUG_ISMA_AAC
  uint64_t wrap_offset = m_wrap_offset;
  uint64_t msec = rtp_ts_to_msec(pak->rtp_pak_ts, wrap_offset);
  isma_message(LOG_DEBUG, 
	       "1st - header len %u frame len %u ts %x %llu", 
	       header_len, frame_len, pak->rtp_pak_ts, msec);
#endif
  if (frame_len == 0) {
    remove_packet_rtp_queue(pak, 1);
    return;
  }
  char *frame_ptr;
  isma_frame_data_t *frame_data;
  uint32_t ts;
  ts = pak->rtp_pak_ts;
  frame_data = get_frame_data();
  frame_data->pak = pak;
  // frame pointer is after header_len + header_len size.  Header_len
  // is in bits - add 7, divide by 8 to get padding correctly.
  frame_data->frame_ptr = &pak->rtp_data[((header_len + 7) / 8) 
				    + sizeof(uint16_t)];
  frame_data->frame_len = frame_len;
  frame_data->rtp_timestamp = ts;

  // Check if frame is fragmented
  // frame_len plus the length of the 2 headers
  int frag_check = frame_len + sizeof(uint16_t);
  frag_check += m_fmtp.size_length / 8;
  if ((m_fmtp.size_length % 8) != 0) frag_check++;
  if (frag_check > pak->rtp_data_len) {
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, "Frame is fragmented");
#endif
    frame_data->is_fragment = 1;
    int err = process_fragment(pak, frame_data);
    if (err == 1)
      isma_message(LOG_ERR, "Error in processing the fragment");
    return; 
  }
  else {
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, "Frame is not fragmented");
#endif
    frame_data->is_fragment = 0;
    frame_data->frag_data = NULL;
  }
  int error = insert_frame_data(frame_data);
  frame_ptr = frame_data->frame_ptr + frame_data->frame_len;
  while (m_header_bitstream.bits_remain() >= m_min_header_bits) {
    uint32_t stride;
    m_header_bitstream.getbits(m_fmtp.size_length, &frame_len);
    m_header_bitstream.getbits(m_fmtp.index_delta_length, &stride);
    get_au_header_bits();
    ts += (m_rtp_ts_add * (1 + stride));
#ifdef DEBUG_ISMA_AAC
    wrap_offset = m_wrap_offset;
    msec = rtp_ts_to_msec(ts, wrap_offset);
    isma_message(LOG_DEBUG, 
		 "Stride %d len %d ts %x %llu", 
		 stride, frame_len, ts, msec);
#endif
    frame_data = get_frame_data();
    frame_data->pak = pak;
    frame_data->is_fragment = 0;
    frame_data->frag_data = NULL;
    frame_data->frame_ptr = frame_ptr;
    frame_data->frame_len = frame_len;
    frame_ptr += frame_len;
    frame_data->rtp_timestamp = ts;
    error |= insert_frame_data(frame_data);
  }
  if (error == 0 && frame_data != NULL) { 
    frame_data->last_in_pak = 1;
  }
  else {
    isma_frame_data_t *p, *last = NULL;
    p = m_frame_data_head;
    while (p != NULL) {
      if (p->pak == pak) last = p;
      p = p->frame_data_next;
    }
    if (last != NULL) {
      last->last_in_pak = 1;
      isma_message(LOG_WARNING, "error at end - marked ts %x", last->rtp_timestamp);
    } else {
      // Didn't find pak in list.  Weird
      isma_message(LOG_ERR, 
		   "Decoded packet with RTP timestamp %x and didn't"
		   "see any good frames", pak->rtp_pak_ts);
      remove_packet_rtp_queue(pak, 1);
      return;
    }
  }
  if (m_fmtp.auxiliary_data_size_length > 0) {
    m_header_bitstream.byte_align();
    uint32_t aux_len;
    m_header_bitstream.getbits(m_fmtp.auxiliary_data_size_length, &aux_len);
    aux_len = (aux_len + 7) / 8;
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, "Adding %d bytes for aux data size", aux_len);
#endif
    isma_frame_data_t *p;
    p = m_frame_data_head;
    while (p != NULL) {
      if (p->pak == pak) {
	p->frame_ptr += aux_len;
      }
      p = p->frame_data_next;
    }
  }
    
  remove_packet_rtp_queue(pak, 0);
}

uint64_t CIsmaAudioRtpByteStream::start_next_frame (unsigned char **buffer, 
						    uint32_t *buflen)
{
  uint64_t timetick;

  if (m_frame_data_on != NULL) {
    uint32_t next_ts;
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, "Advancing to next pak data - old ts %x", 
		 m_frame_data_on->rtp_timestamp);
#endif
    if (m_frame_data_on->last_in_pak != 0) {
      // We're done with all the data in this packet - get rid
      // of the rtp packet.
      if (m_frame_data_on->is_fragment == 1) {
	// if fragmented, need to get rid of all paks pointed to
	isma_frag_data_t *q =  m_frame_data_on->frag_data;
	while (q != NULL) {
	  rtp_packet *pak = q->pak;
	  q->pak = NULL;
	  if (pak != NULL)
	    xfree(pak);
	  q = q->frag_data_next;
#ifdef DEBUG_ISMA_AAC
	  isma_message(LOG_DEBUG, "removing pak - frag %d", pak->rtp_pak_seq);
#endif
	}
      } else {
	rtp_packet *pak = m_frame_data_on->pak;
	m_frame_data_on->pak = NULL;
	xfree(pak);
#ifdef DEBUG_ISMA_AAC
	isma_message(LOG_DEBUG, "removing pak %d", pak->rtp_pak_seq);
#endif
      }
    }
    /*
     * Remove the frame data head pointer, and put it on the free list
     */
    isma_frame_data_t *p = NULL;
    SDL_LockMutex(m_rtp_packet_mutex);
    p = m_frame_data_on;
    m_frame_data_on = NULL;
    next_ts = p->rtp_timestamp;
    p->frame_data_next = m_frame_data_free;
    m_frame_data_free = p;
    // free all frag_data for this frame
    if (p->is_fragment == 1) {
      isma_frag_data_t * q = p->frag_data;
      while (q != NULL) {
	p->frag_data = q->frag_data_next;
	free(q);
	q = p->frag_data;
      } 
    }
    SDL_UnlockMutex(m_rtp_packet_mutex);

    /* 
     * Now, look for the next timestamp - process a bunch of new
     * rtp packets, if we have to...
     */
    next_ts += m_rtp_ts_add;
    if (m_frame_data_head == NULL ||
	m_frame_data_head->rtp_timestamp != next_ts) {
      // process next pak in list.  Process until next timestamp is found, 
      // or 500 msec worth of data is found (in which case, go with first)
      do {
	process_packet_header();
      } while (m_head != NULL && 
	       ((m_frame_data_head == NULL) || 
		(m_frame_data_head->rtp_timestamp != next_ts)) &&  
	       (m_frame_data_free != NULL));
    }
#ifdef DEBUG_ISMA_AAC
    else {
      // m_frame_data_head is correct
      isma_message(LOG_DEBUG, "frame_data_head is correct");
    }
#endif
  } else {
    // first time.  Process a bunch of packets, go with first one...
    // asdf - will want to eventually add code to drop the first couple
    // of packets if they're not consecutive.
    do {
      process_packet_header();
    } while (m_frame_data_free != NULL);
  }
  /*
   * Init up the offsets
   */
  if (m_frame_data_head != NULL) {
    SDL_LockMutex(m_rtp_packet_mutex);
    m_frame_data_on = m_frame_data_head;
    m_frame_data_head = m_frame_data_head->frame_data_next;
    SDL_UnlockMutex(m_rtp_packet_mutex);

    if (m_frame_data_on->is_fragment == 1) {	  

      m_frag_reass_size = 0;
      isma_frag_data_t *ptr;
      ptr = m_frame_data_on->frag_data;
      while (ptr != NULL) {
	if (m_frag_reass_size + ptr->frag_len > m_frag_reass_size_max) {
	  m_frag_reass_size_max += max(4096, ptr->frag_len);
	  m_frag_reass_buffer = (unsigned char *)realloc(m_frag_reass_buffer, 
							 m_frag_reass_size_max);
	}
	memmove(m_frag_reass_buffer + m_frag_reass_size, 
		ptr->frag_ptr,
		ptr->frag_len);
	m_frag_reass_size += ptr->frag_len;
	ptr = ptr->frag_data_next;
      }
      *buffer = m_frag_reass_buffer;
      *buflen = m_frag_reass_size;
    } else { 
      *buffer = (unsigned char *)m_frame_data_on->frame_ptr;
      *buflen = m_frame_data_on->frame_len;
    }
  } else {
    *buffer = NULL;
  }
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  if (*buffer != NULL) {
    fwrite(*buffer, *buflen,  1, m_outfile);
  }
#endif
  timetick = rtp_ts_to_msec(m_frame_data_on != NULL ? 
			    m_frame_data_on->rtp_timestamp : m_ts, 
			    m_wrap_offset);
  if (m_frame_data_on != NULL)
    m_ts =  m_frame_data_on->rtp_timestamp;
  // We're going to have to handle wrap better...
#ifdef DEBUG_ISMA_AAC
  isma_message(LOG_DEBUG, "start next frame %p %d ts %x "LLU, 
	       *buffer, *buflen, m_ts, timetick);
#endif
  return (timetick);
}

void CIsmaAudioRtpByteStream::used_bytes_for_frame (uint32_t bytes)
{
}

void CIsmaAudioRtpByteStream::flush_rtp_packets (void)
{
  isma_frame_data_t *p;
  SDL_LockMutex(m_rtp_packet_mutex);
  if (m_frame_data_on != NULL) {
    m_frame_data_on->frame_data_next = m_frame_data_head;
    m_frame_data_head = m_frame_data_on;
    m_frame_data_on = NULL;
  }
  if (m_frame_data_head != NULL) {
    p = m_frame_data_head;
    while (p->frame_data_next != NULL) {
#ifdef DEBUG_ISMA_AAC
      isma_message(LOG_DEBUG, "reset removing pak %d", p->pak->rtp_pak_seq);
#endif
      if (p->last_in_pak != 0) {
	if (p->is_fragment == 1) {
	  // get rid of frag data 
	  isma_frag_data_t * q = NULL;
	  while ((q = p->frag_data) != NULL) {
	    p->frag_data = q->frag_data_next;
	    free(q);
	  } 
	}
	xfree(p->pak);
      }
      p = p->frame_data_next;
    }
    p->frame_data_next = m_frame_data_free;
    m_frame_data_free = m_frame_data_head;
    m_frame_data_head = NULL;
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  CRtpByteStreamBase::flush_rtp_packets();
}

void CIsmaAudioRtpByteStream::reset (void)
{
  isma_message(LOG_INFO, "Hit isma rtp reset");
  CRtpByteStreamBase::reset();
}


int CIsmaAudioRtpByteStream::have_no_data (void)
{
  return (m_head == NULL && m_frame_data_head == NULL);
}

