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
  m_is_fragment = 0;
  m_frag_data = NULL;
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  m_outfile = fopen("isma.aac", "w");
#endif
  m_frame_data_head = NULL;
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
  player_debug_message("Rtp ts add is %d (%d %d)", m_rtp_ts_add,
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
  isma_frame_data_t *p;
  
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


// given an offset, find out which fragment it is in
// return ptr to the fragment
isma_frag_data_t *CIsmaAudioRtpByteStream::go_to_frag (void)
{
  isma_frag_data_t *p = m_frag_data;
  uint32_t offset = m_offset_in_frame;
  while (p != NULL) {
	if (offset < p->frag_len) {
	  return (p);
	}
	offset -= p->frag_len;
	p = p->frag_data_next;
  }
  if (m_bookmark_set != 1) {
    throw THROW_ISMA_RTP_FRAGMENT_PAST_END;
  }
  return (NULL);
}

// given an offset, find out which fragment it is in
// return ptr to that location in the fragment
char *CIsmaAudioRtpByteStream::go_to_offset (void)
{
  char *frag_ptr = NULL;
  isma_frag_data_t *p = m_frag_data;
  size_t offset = m_offset_in_frame;
  while (p != NULL) {
	if (offset < p->frag_len) {
	  frag_ptr = p->frag_ptr + offset;
	  break;
	}
	offset -= p->frag_len;
	p = p->frag_data_next;
  }
  if (frag_ptr == NULL && m_bookmark_set != 1) {
    throw THROW_ISMA_RTP_FRAGMENT_PAST_END;
  }
  return (frag_ptr);
}

// get one char
unsigned char CIsmaAudioRtpByteStream::get (void)
{
  unsigned char ret;

  if (m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw THROW_RTP_NULL_WHEN_START;
  }

  if (m_offset_in_frame >= m_frame_len) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    throw THROW_ISMA_RTP_DECODE_PAST_EOF;
  }

  // check if frame is fragmented
  if (m_is_fragment == 1) {
	char *offset_ptr = go_to_offset();	
	ret = (offset_ptr != NULL) ? *offset_ptr : 0;
  } else {
	ret = m_frame_ptr[m_offset_in_frame];
  }
  m_offset_in_frame++;
  m_total++;
  //check_for_end_of_pak();
  return (ret);
}

unsigned char CIsmaAudioRtpByteStream::peek (void) 
{
  if (m_is_fragment == 1) {
	if (m_frame_ptr == NULL)
	  return 0;
	char *offset_ptr = go_to_offset();	
	return ((offset_ptr != NULL) ? *offset_ptr : 0);
  } else 
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



void CIsmaAudioRtpByteStream::read_frag (unsigned char *buffer, 
										 size_t bytes_to_read)
{
  unsigned char *cur = buffer;
  isma_frag_data_t *frag_data = go_to_frag(); 
  char *offset = go_to_offset();
  //int len = frag_data->frag_len - (offset - frag_data->frag_ptr);
  int len = min(bytes_to_read, 
				frag_data->frag_len - (offset - frag_data->frag_ptr));
  while (bytes_to_read > 0) {
	if (frag_data == NULL) {
	  // error 
	  init();
	  throw THROW_RTP_NULL_WHEN_START;
	}
	memcpy(cur, &offset, len);
	cur += len;
	frag_data = frag_data->frag_data_next;
	bytes_to_read -= len;
	len = frag_data->frag_len;
	offset = frag_data->frag_ptr;
  } 
  return;
}

ssize_t CIsmaAudioRtpByteStream::read (unsigned char *buffer, 
									  size_t bytes_to_read)
{
  size_t inbuffer;

  if (m_frame_ptr == NULL) {
    if (m_bookmark_set == 1) {
      return (0);
    }
    init();
    throw THROW_RTP_NULL_WHEN_START;
  }

  inbuffer = m_frame_len - m_offset_in_frame;
  if (inbuffer < bytes_to_read) {
    bytes_to_read = inbuffer;
  }
  if (m_is_fragment == 1) 
	read_frag(buffer, bytes_to_read);
  else
	memcpy(buffer, &m_frame_ptr[m_offset_in_frame], bytes_to_read);
  m_offset_in_frame += bytes_to_read;
  m_total += bytes_to_read;
  return (bytes_to_read);
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
	player_error_message("Duplicate timestamp of %x found in RTP packet",
			     frame_data->rtp_timestamp);
	player_debug_message("Seq number orig %d new %d", 
			     p->pak->seq, frame_data->pak->seq); 
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
  uint16_t seq = pak->seq; 
  uint32_t ts = pak->ts;
  isma_frag_data_t *cur = NULL;
  int read_mBit = 0;
  uint32_t total_len = 0; 
  frame_data->is_fragment = 1;
  do {
	if (read_mBit == 1) {
	  // get rid of frame_data - last frag seen but total length wrong
	  cleanup_frag(frame_data);
	  player_error_message("Error processing frag: early mBit");
	  return (1); 
	}
	if (pak == NULL) {
	  cleanup_frag(frame_data);
	  player_error_message("Error processing frag: not enough packets");
	  return (1);
	}
	// check if ts and rtp seq numbers are ok, and lengths match
	if (ts != pak->ts) {
	  cleanup_frag(frame_data);
	  player_error_message("Error processing frag: wrong ts: ts= %x, "
						   "pak->ts = %x", ts, pak->ts);
	  return (1);
	}
	if (seq != pak->seq) {
	  cleanup_frag(frame_data);
	  player_error_message("Error processing frag: wrong seq num");
	  return (1);
	 }
	// insert fragment info
	isma_frag_data_t *p = (isma_frag_data_t *)
	                       malloc(sizeof(isma_frag_data_t));
	if (p == NULL) {
	   player_error_message("Error processing frag: can't malloc");
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
	uint16_t header_len = ntohs(*(unsigned short *)pak->data);
	m_header_bitstream.init(&pak->data[sizeof(uint16_t)], header_len);
	// frag_ptr should just point to beginning of data in pkt
	uint32_t header_len_bytes = ((header_len + 7) / 8) + sizeof(uint16_t);
	cur->frag_ptr =  &pak->data[header_len_bytes];
	cur->frag_len = pak->data_len - header_len_bytes;
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
	player_debug_message("rtp seq# %d, fraglen: %d, ts: %x", pak->seq, 
						 cur->frag_len, pak->ts);
#endif	
	seq = pak->seq + 1;
	if (pak->m) 
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
  if (m_header_bitstream.getbits(m_fmtp.size_length, &frame_len) != 0) 
    return;
  m_header_bitstream.getbits(m_fmtp.index_length, &retvalue);
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
  isma_frame_data_t *frame_data;
  uint32_t ts;
  ts = pak->ts;
  frame_data = get_frame_data();
  frame_data->pak = pak;
  // frame pointer is after header_len + header_len size.  Header_len
  // is in bits - add 7, divide by 8 to get padding correctly.
  frame_data->frame_ptr = &pak->data[((header_len + 7) / 8) 
									+ sizeof(uint16_t)];
  frame_data->frame_len = frame_len;
  frame_data->rtp_timestamp = ts;

  // Check if frame is fragmented
  // frame_len plus the length of the 2 headers
  if (int(frame_len +  2 * sizeof(uint16_t)) > pak->data_len) {
#ifdef DEBUG_ISMA_AAC
	player_debug_message("Frame is fragmented");
#endif
	frame_data->is_fragment = 1;
	int err = process_fragment(pak, frame_data);
	if (err == 1)
	  player_error_message("Error in processing the fragment");
	return; 
  }
  else {
#ifdef DEBUG_ISMA_AAC
	player_debug_message("Frame is not fragmented");
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
    player_debug_message("Stride %d len %d ts %x %llu", 
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
    uint32_t aux_len;
    m_header_bitstream.getbits(m_fmtp.auxiliary_data_size_length, &aux_len);
    aux_len = (aux_len + 7) / 8;
#ifdef DEBUG_ISMA_AAC
    player_debug_message("Adding %d bytes for aux data size", 
						 aux_len);
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

uint64_t CIsmaAudioRtpByteStream::start_next_frame (void)
{
  uint64_t timetick;

  if (m_frame_data_head != NULL) {
    uint32_t next_ts;
#ifdef DEBUG_ISMA_AAC
    player_debug_message("Advancing to next pak data - old ts %x", 
						 m_frame_data_head->rtp_timestamp);
#endif
    if (m_frame_data_head->last_in_pak != 0) {
	  if (m_frame_data_head->is_fragment == 1) {
		// if fragmented, need to get rid of all paks pointed to
		isma_frag_data_t *q =  m_frame_data_head->frag_data;
		while (q != NULL) {
		  rtp_packet *pak = q->pak;
		  q->pak = NULL;
		  if (pak != NULL)
			xfree(pak);
		  q = q->frag_data_next;
#ifdef DEBUG_ISMA_AAC
		  player_debug_message("removing pak - frag %d", pak->seq);
#endif
		}
	  } else {
		rtp_packet *pak = m_frame_data_head->pak;
		m_frame_data_head->pak = NULL;
		xfree(pak);
#ifdef DEBUG_ISMA_AAC
		player_debug_message("removing pak %d", pak->seq);
#endif
	  }
    }
    isma_frame_data_t *p = NULL;
    SDL_LockMutex(m_rtp_packet_mutex);
    p = m_frame_data_head;
    next_ts = p->rtp_timestamp;
    m_frame_data_head = p->frame_data_next;
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
    // Now, look for the next timestamp
    next_ts += m_rtp_ts_add;
    if (m_frame_data_head == NULL 
		|| m_frame_data_head->rtp_timestamp != next_ts) {
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
	  player_debug_message("frame_data_head is correct");
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
  m_offset_in_frame = 0;
  if (m_frame_data_head != NULL) {
	m_is_fragment =  m_frame_data_head->is_fragment;
	if (m_is_fragment == 1) {	  
	  m_frag_data = m_frame_data_head->frag_data;
	  if (m_frag_data != NULL) {
		m_frame_ptr = m_frag_data->frag_ptr;
		m_frame_len = m_frame_data_head->frame_len;
	  }
	  else {
		// we should never get here!
		m_frame_ptr = NULL;
		m_frame_len = 0;
		m_is_fragment = 0;
		m_frag_data = NULL;
		init();
		throw THROW_ISMA_INCONSISTENT;
	  }
	} else { 
	  m_frag_data = NULL;
	  m_frame_ptr = m_frame_data_head->frame_ptr;
	  m_frame_len = m_frame_data_head->frame_len;
	}
  } else {
    m_frame_ptr = NULL;
    m_frame_len = 0;
	m_is_fragment = 0;
	m_frag_data = NULL;
  }
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  if (m_frame_ptr != NULL) {
	if (m_is_fragment == 0)
	  fwrite(m_frame_ptr, m_frame_len, 1, m_outfile);
	else {
	  isma_frag_data_t *p = m_frag_data; 
	  while (p!= NULL) {
		fwrite(p->frag_ptr, p->frag_len, 1, m_outfile);
		p = p->frag_data_next;
	  }
	}
  }
#endif
  timetick = rtp_ts_to_msec(m_frame_data_head != NULL ? 
							m_frame_data_head->rtp_timestamp : m_ts, 
							m_wrap_offset);
  if (m_frame_data_head != NULL)
    m_ts =  m_frame_data_head->rtp_timestamp;
  // We're going to have to handle wrap better...
#ifdef DEBUG_ISMA_AAC
  player_debug_message("start next frame %p %d ts %x "LLU, 
					   m_frame_ptr, m_frame_len, m_ts, timetick);
#endif
  return (timetick);
}


void CIsmaAudioRtpByteStream::flush_rtp_packets (void)
{
  isma_frame_data_t *p;
  SDL_LockMutex(m_rtp_packet_mutex);
  if (m_frame_data_head != NULL) {
    p = m_frame_data_head;
    while (p->frame_data_next != NULL) {
#ifdef DEBUG_ISMA_AAC
      player_debug_message("reset removing pak %d", p->pak->seq);
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
  player_debug_message("Hit isma rtp reset");
  m_frame_ptr = NULL;
  m_offset_in_frame = 0;
  m_frame_len = 0;
  m_is_fragment = 0;
  m_frag_data = NULL;
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
  return (m_head == NULL && m_frame_data_head == NULL);
}

const char *CIsmaAudioRtpByteStream::get_throw_error (int error)
{
  if (error <= THROW_RTP_BASE_MAX) {
    return (CRtpByteStreamBase::get_throw_error(error));
  }
  switch (error) {
  case THROW_ISMA_RTP_FRAGMENT_PAST_END:
    return "Read past end of fragment";
  case THROW_ISMA_RTP_DECODE_PAST_EOF:
    return "Read past end of frame";
  case THROW_ISMA_INCONSISTENT:
    return "Inconsistent data - we can't recover";
  default:
    break;
  }
  player_debug_message("Isma RTP bytestream - unknown throw error %d", error);
  return "Unknown error";
}

int CIsmaAudioRtpByteStream::throw_error_minor (int error)
{
  if (error <= THROW_RTP_BASE_MAX) {
    return (CRtpByteStreamBase::throw_error_minor(error));
  }
  return 0;
}
