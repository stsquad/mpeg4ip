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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * rfc3119_bytestream.cpp - an implemention of rfc 3119
 */
#include "mpeg4ip.h"
#include <rtp/rtp.h>
#include <rtp/memory.h>
#include <sdp/sdp.h> // for NTP_TO_UNIX_TIME
#include "rfc3119_bytestream.h"
#include <mp3util/MP3Internals.hh>

//#define DEBUG_3119 1
//#define DEBUG_3119_INTERLEAVE 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mpa_message, "mparobust")
#else
#define mpa_message(loglevel, fmt...) message(loglevel, "mparobust", fmt)
#endif

CRfc3119RtpByteStream::CRfc3119RtpByteStream (unsigned int rtp_pt,
					      format_list_t *fmt,
					      int ondemand,
					      uint64_t tps,
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
  CRtpByteStreamBase("mparobust", fmt, rtp_pt, ondemand, tps, head, tail, 
		     rtp_seq_set, rtp_base_seq, rtp_ts_set, rtp_base_ts,
		     rtcp_received, ntp_frac, ntp_sec, rtp_ts)
{
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  m_outfile = fopen("isma.aac", "w");
#endif
  m_adu_data_free = NULL;
  m_deinterleave_list = NULL;
  m_ordered_adu_list = NULL;
  m_pending_adu_list = NULL;

  adu_data_t *p;
  for (int ix = 0; ix < 25; ix++) {
    p = MALLOC_STRUCTURE(adu_data_t);
    p->next_adu = m_adu_data_free;
    m_adu_data_free = p;
  }
  m_rtp_ts_add = 0;
  m_recvd_first_pak = 0;
  m_got_next_idx = 0;
  m_mp3_frame = NULL;
  m_mp3_frame_size = 0;
}

CRfc3119RtpByteStream::~CRfc3119RtpByteStream (void)
{
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(m_outfile);
#endif
  adu_data_t *p;
#if 0
  if (m_frag_reass_buffer != NULL) {
    free(m_frag_reass_buffer);
    m_frag_reass_buffer = NULL;
  }
#endif

  dump_adu_list(m_deinterleave_list);
  m_deinterleave_list = NULL;
  dump_adu_list(m_ordered_adu_list);
  m_ordered_adu_list = NULL;
  dump_adu_list(m_pending_adu_list);
  m_pending_adu_list = NULL;

  while (m_adu_data_free != NULL) {
    p = m_adu_data_free;
    m_adu_data_free = p->next_adu;
    free(p);
  }
}

void CRfc3119RtpByteStream::insert_interleave_adu (adu_data_t *adu)
{
  adu_data_t *p, *q;
#ifdef DEBUG_3119_INTERLEAVE
  mpa_message(LOG_DEBUG, "inserting interleave %d %d %d", adu->aduDataSize,
	      adu->cyc_ct, adu->interleave_idx);
#endif
  SDL_LockMutex(m_rtp_packet_mutex);
  
  if (m_deinterleave_list == NULL) {
    m_got_next_idx = 0;
    m_deinterleave_list = adu;
  } else {
    q = NULL;
    p = m_deinterleave_list;
    if (adu->cyc_ct != p->cyc_ct) {
      m_got_next_idx = 1;
      do {
	q = p;
	p = p->next_adu;
      } while (p != NULL && p->cyc_ct != adu->cyc_ct);
      if (p == NULL) {
	q->next_adu = adu;
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return;
      } 
    }
    while (p != NULL && p->interleave_idx < adu->interleave_idx) {
      q = p;
      p = p->next_adu;
    }
    q->next_adu = adu;
    adu->next_adu = p;
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
}

int CRfc3119RtpByteStream::insert_processed_adu (adu_data_t *adu)
{
#ifdef DEBUG_3119
  mpa_message(LOG_DEBUG, "inserting processed %d", adu->aduDataSize);
#endif
  SDL_LockMutex(m_rtp_packet_mutex);
  if (m_ordered_adu_list == NULL) {
    m_ordered_adu_list = adu;
  } else {
    int32_t diff;
    adu_data_t *p, *q;
    q = NULL;
    p = m_ordered_adu_list;

    do {
      diff = adu->timestamp - p->timestamp;
      if (diff == 0) {
	mpa_message(LOG_ERR, "Duplicate timestamp of "U64" found in RTP packet",
		    adu->timestamp);
	mpa_message(LOG_DEBUG, 
		     "Seq number orig %d new %d", 
		     p->pak->rtp_pak_seq, adu->pak->rtp_pak_seq); 
	mpa_message(LOG_ERR, "Dumping all processed packets");

	
	// put frame_data on free list
	if (adu->last_in_pak == 0) {
	  free_adu(adu);
	} else {
	  mpa_message(LOG_ERR, "Im lazy - duplicate ADU leads to memory leak");
	}
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return -1;
      } else if (diff < 0) {
#ifdef DEBUG_3119
	mpa_message(LOG_DEBUG, "Inserting in front of ts "U64, p->timestamp);
#endif
	if (q == NULL) {
	  adu->next_adu = m_ordered_adu_list;
	  m_ordered_adu_list = adu;
	} else {
	  q->next_adu = adu;
	  adu->next_adu = p;
	}
	SDL_UnlockMutex(m_rtp_packet_mutex);
	return 0;
      }
      q = p;
      p = p->next_adu;
    } while (p != NULL);
    // insert at end;
    q->next_adu = adu;

  }
  SDL_UnlockMutex(m_rtp_packet_mutex);
  return 0;
}

void CRfc3119RtpByteStream::process_packet (void)
{
  rtp_packet *pak;
  uint64_t pak_ts;
  uint32_t freq_ts;
  uint32_t adu_offset;
  adu_data_t *prev_adu;
  int thisAduSize;

  do {
    /*
     * Take packet off list.  Loop through, looking for ADU headersize
     * packets
     */
    pak = m_head;
    if (pak != NULL) {
      remove_packet_rtp_queue(m_head, 0);
      freq_ts = pak->rtp_pak_ts;
      pak_ts = rtp_ts_to_msec(pak->rtp_pak_ts, 
			      pak->pd.rtp_pd_timestamp,
			      m_wrap_offset);
      adu_offset = 0;
      prev_adu = NULL;

#ifdef DEBUG_3119
      mpa_message(LOG_DEBUG, "Process pak seq %d", pak->rtp_pak_seq);
#endif

      while (pak != NULL && adu_offset < pak->rtp_data_len) {
	uint8_t *ptr;
	ptr = (uint8_t *)(pak->rtp_data + adu_offset);
	if ((*ptr & 0x80) == 0x80) {
	  // first one has a c field of 1 - that's bad
	  xfree(pak);
	  pak = NULL;
	  adu_offset = 0;
	} else {
	  /*
	   * Get a new adu - start loading the data
	   */
	  adu_data_t *adu = get_adu_data();

	  adu->pak = pak;
	  adu->next_adu = NULL;
	  adu->last_in_pak = 0;
	  adu->freeframe = 0;
	  if (adu_offset == 0) {
	    adu->first_in_pak = 1;
	    adu->timestamp = pak_ts;
	    adu->freq_timestamp = freq_ts;
	  } else {
	    adu->first_in_pak = 0;
	  }
	  /*
	   * Read the ADU header.  It will be 1 byte or 2
	   */
	  if ((*ptr & 0x40) == 0) {
	    // 1 byte header
	    adu->frame_ptr = ptr + 1;
	    adu->aduDataSize = *ptr & 0x3f;
	    adu_offset++;
	  } else {
	    // 2 byte header
	    adu->aduDataSize = ((*ptr & 0x3f) << 8) + ptr[1];
	    adu->frame_ptr = ptr + 2;
	    adu_offset += 2;
	  }
	  /*
	   * See if we're past the end of the packet
	   */
	  thisAduSize = adu->aduDataSize;
	  if (adu_offset + adu->aduDataSize > pak->rtp_data_len) {
	    // have a fragment here...
	    // malloc a frame pointer, and copy the rest... 
	    uint16_t seq = pak->rtp_pak_seq;
	    uint8_t *to, *from;
	    to = (uint8_t *)malloc(adu->aduDataSize);
	    int copied = pak->rtp_data_len - adu_offset;
	    memcpy(to, adu->frame_ptr, copied);
	    if (prev_adu != NULL) {
	      prev_adu->last_in_pak = 1;
	    } else {
	      xfree(pak);
	    }
	    adu->frame_ptr = to;
	    adu->freeframe = 1;
	    to += copied;
	    while (m_head != NULL && 
		   m_head->rtp_pak_seq == seq + 1 && 
		   (*m_head->rtp_data & 0x80) == 0x80 && 
		   copied < adu->aduDataSize) {
	      uint32_t bytes;
	      pak = m_head;
	      remove_packet_rtp_queue(m_head, 0);
	      ptr = (uint8_t *)pak->rtp_data;
	      if ((*ptr & 0x40) == 0) {
		// 1 byte header
		bytes = *ptr & 0x3f;
		from = ptr++;
	      } else {
		// 2 byte header
		bytes = ((*ptr & 0x3f) << 8) + ptr[1];
		from = ptr + 2;
	      }
	      bytes = pak->rtp_data_len - (from - pak->rtp_data);
#ifdef DEBUG_3119
	      mpa_message(LOG_DEBUG, "frag process pak seq %d %d %d %d", pak->rtp_pak_seq, bytes, copied, adu->aduDataSize);
#endif
	      memcpy(to, from, bytes);
	      copied += bytes;
	      to += bytes;
	      seq++;
	      adu_offset = 0;
	      pak = NULL;
	    }
	    if (copied < adu->aduDataSize) {
	      free_adu(adu);
	      continue;
	    }
	  } else {
	    /*
	     * adu_offset now points past the end of this adu
	     */
	    adu_offset += adu->aduDataSize;
	    if (adu_offset >= pak->rtp_data_len) {
	      adu->last_in_pak = 1;
	    }
	  }

	  /*
	   * Calculate the interleave index and the cyc_ct count
	   * Fill in those values with all 1's
	   */
	  adu->interleave_idx = *adu->frame_ptr;
	  adu->cyc_ct = *(adu->frame_ptr + 1) >> 5;
	  adu->frame_ptr[0] = 0xff;
	  adu->frame_ptr[1] |= 0xe0;

	  /*
	   * Decode the mp3 header
	   */
	  adu->mp3hdr = MP4AV_Mp3HeaderFromBytes(adu->frame_ptr);
	  adu->framesize = MP4AV_Mp3GetFrameSize(adu->mp3hdr);

	  /*
	   * Headersize, sideInfoSize and aduDataSize 
	   */
	  adu->headerSize = 4;
	  adu->sideInfoSize = MP4AV_Mp3GetSideInfoSize(adu->mp3hdr);

	  adu->aduDataSize -= (adu->headerSize + adu->sideInfoSize);

	  /*
	   * Calculate the timestamp add
	   */
	  if (m_rtp_ts_add == 0) {
	    m_rtp_ts_add = 
	      ( 1000 * MP4AV_Mp3GetHdrSamplingWindow(adu->mp3hdr));
	    m_rtp_freq_ts_add = m_rtp_ts_add;
	    m_freq = MP4AV_Mp3GetHdrSamplingRate(adu->mp3hdr);
	    m_rtp_ts_add /= m_freq;
	  }
	
	  // here we make a few decisions as to where the packet should
	  // go.
	  // Look at first packet.  If it's got interleave, we're interleaved
	  // if not, set up the timestamp, put it in the ordered_adu_list.
	  // (Don't set recvd_first pak).  If it's the 2nd, and not interleaved,
	  // leave the first, go on non-interleaved
	  // If the first isn't, and the 2nd is, move the first to the
	  // m_deinterleave_list, put the 2nd on the deinterleave list, 
	  // continue.
	  // If not the first, put on the interleaved or deinterleaved list.
	  int insert_interleaved = 0;

	  if (m_recvd_first_pak == 0) {
	    if (adu->interleave_idx == 0xff &&
		adu->cyc_ct == 0x7) {
	      /*
	       * Have an packet all 1's.  It's either the last of the
	       * sequence for interleave, or indicates no interleave.
	       * Put it on the ordered list if there's nothing there.
	       * If there is something on the ordered list, we're not doing
	       * interleaved.
	       */
	      if (m_ordered_adu_list == NULL) {
		insert_interleaved = 0;
	      } else {
		// this is the 2nd packet with the regular
		// header - we're not doing interleaved
		insert_interleaved = 0;
		m_recvd_first_pak = 1;
		m_have_interleave = 0;
	      }
	    } else {
	      /*
	       * Have an interleaved packet
	       * If there was something on the ordered list, move it to 
	       * the deinterleaved list
	       */
	      m_recvd_first_pak = 1;
	      if (m_ordered_adu_list != NULL) {
		m_deinterleave_list = m_ordered_adu_list;
		m_ordered_adu_list = NULL;
	      } 
	      m_have_interleave = 1;
	      insert_interleaved = 1;
	      mpa_message(LOG_INFO, "mpa robust format is interleaved");
	    }
	  } else {
	    insert_interleaved = m_have_interleave;
	  }

	  adu->backpointer =  MP4AV_Mp3GetAduOffset(adu->frame_ptr, 
						    thisAduSize);

	  if (insert_interleaved) {
	    insert_interleave_adu(adu);
	  } else {
	    // calculate timestamp
	    if (adu->first_in_pak == 0) {
	      adu->timestamp = prev_adu->timestamp + m_rtp_ts_add;
	      adu->freq_timestamp = prev_adu->freq_timestamp + m_rtp_freq_ts_add;
	    }
	    insert_processed_adu(adu);
	  }
	  prev_adu = adu;
	}
      } 
    } else if (m_have_interleave && m_deinterleave_list != NULL) {
      m_got_next_idx = 1;
    }

    // done with packet.  If we're interleaved, move things down
    // to ordered ADU list.
    // continue until we've got the cycct index number. 
    // For non-interleaved, skip this step
    if (m_have_interleave != 0) {
      if (m_got_next_idx == 1) {
	// okay - we received the next index - we can move everything 
	// from the interleaved list to the processed list.
	adu_data_t *p, *q;
	int cur_cyc_ct;
	int ts_index;
	uint64_t ts = 0;
	uint32_t freq_ts = 0;
	m_got_next_idx = 0;
	q = NULL;
	p = m_deinterleave_list;
	ts_index = -1;
	cur_cyc_ct = p->cyc_ct;
	do {
	  if (ts_index == -1 && p->first_in_pak) {
	    ts = p->timestamp;
	    freq_ts = p->freq_timestamp;
	    ts_index = p->interleave_idx;
	  }
	  q = p;
	  p = p->next_adu;
	} while (p != NULL && p->cyc_ct == cur_cyc_ct);
	q->next_adu = NULL;
	m_ordered_adu_list = m_deinterleave_list;
	m_deinterleave_list = p;

	// We've figured out that at least 1 packet has a valid timestamp.
	// Figure out the starting timestamp, then go ahead and set the
	// timestamps for the rest.
	// make sure ts_index is not -1 here
     
	p = m_ordered_adu_list;
	ts -= ((ts_index - p->interleave_idx) * m_rtp_ts_add);
	freq_ts -= ((ts_index - p->interleave_idx) * m_rtp_freq_ts_add);
	ts_index = p->interleave_idx;
	while (p != NULL) {
	  if (p->first_in_pak == 0) {
	    // calculate ts
	    p->timestamp = ts + (p->interleave_idx - ts_index) * m_rtp_ts_add;
	    p->freq_timestamp = freq_ts + (p->interleave_idx - ts_index) * 
	      m_rtp_freq_ts_add;
	  } else {
	    ts = p->timestamp;
	    freq_ts = p->freq_timestamp;
	    ts_index = p->interleave_idx;
	  }
#ifdef DEBUG_3119_INTERLEAVE
	  mpa_message(LOG_DEBUG, "cyc %d index %d fip %d ts "U64" %d", 
		      p->cyc_ct,
		      p->interleave_idx,
		      p->first_in_pak,
		      p->timestamp,
		      m_rtp_ts_add);
#endif
	  p = p->next_adu;
	}
      } // end moving from deinterleave to processed list
    }
  } while (m_head != NULL && m_ordered_adu_list == NULL);
}

int CRfc3119RtpByteStream::needToGetAnADU (void)
{
  
  if (m_pending_adu_list == NULL) 
    return 1;

  int endOfHeadFrame;
  int frameOffset = 0;
  adu_data_t *p;
 
  p = m_pending_adu_list;

  endOfHeadFrame = p->framesize - p->headerSize - p->sideInfoSize;

  while (p != NULL) {
    int framesize = p->framesize;
    int endOfData = frameOffset - p->backpointer + p->aduDataSize;
    
#ifdef DEBUG_3119
    mpa_message(LOG_DEBUG, "add fr %d hd %d si %d bp %d adu %d", 
		framesize,
		p->headerSize,
		p->sideInfoSize,
		p->backpointer, p->aduDataSize);
    mpa_message(LOG_DEBUG, "foffset %d endOfData %d eohf %d", 
		frameOffset, endOfData, endOfHeadFrame);
#endif
    if (endOfData >= endOfHeadFrame) {
      return 0;
    }
    frameOffset += framesize - p->headerSize - p->sideInfoSize;
    p = p->next_adu;
  }
  return 1;
}

void CRfc3119RtpByteStream::add_and_insertDummyADUsIfNecessary (void)
{
  adu_data_t *tailADU, *prevADU;
  int prevADUend;

  tailADU = m_ordered_adu_list;
  m_ordered_adu_list = m_ordered_adu_list->next_adu;
  tailADU->next_adu = NULL;
  
#ifdef DEBUG_3119
  mpa_message(LOG_DEBUG, "tail fr %d bp %d si %d", 
	      tailADU->framesize,
	      tailADU->backpointer, 
	      tailADU->sideInfoSize);
#endif

  SDL_LockMutex(m_rtp_packet_mutex);
  prevADU = m_pending_adu_list;
  while (prevADU != NULL && prevADU->next_adu != NULL) 
    prevADU = prevADU->next_adu;

  if (prevADU != NULL) {
    prevADU->next_adu = tailADU;
  } else {
    m_pending_adu_list = tailADU;
  }
  SDL_UnlockMutex(m_rtp_packet_mutex);

  while (1) {
    if (m_pending_adu_list != tailADU) {
      prevADUend = prevADU->framesize + 
	prevADU->backpointer -
	prevADU->headerSize -
	prevADU->sideInfoSize;
      if (prevADU->aduDataSize > prevADUend) {
	prevADUend = 0;
      } else {
	prevADUend -= prevADU->aduDataSize;
      }
#ifdef DEBUG_3119
      mpa_message(LOG_DEBUG, "fr %d bp %d si %d", 
		  prevADU->framesize,
		  prevADU->backpointer, 
		  prevADU->sideInfoSize);
      mpa_message(LOG_DEBUG, "prevADUend is %d, prev size %d tail bpointer %d", 
		  prevADUend, prevADU->aduDataSize,
		  tailADU->backpointer);
#endif
    } else {
      prevADUend = 0;
    }

    if (tailADU->backpointer > prevADUend) {
      uint64_t ts;
      uint32_t freq_ts;
#ifdef DEBUG_3119
      mpa_message(LOG_DEBUG, "Adding tail ts is "D64, tailADU->timestamp);
#endif
      SDL_LockMutex(m_rtp_packet_mutex);
      if (prevADU == NULL) {
	prevADU = get_adu_data();
	ts = m_pending_adu_list->timestamp - m_rtp_ts_add;
	freq_ts = m_pending_adu_list->freq_timestamp - m_rtp_freq_ts_add;
	prevADU->next_adu = m_pending_adu_list;
	m_pending_adu_list = prevADU;
#ifdef DEBUG_3119
	mpa_message(LOG_DEBUG, "Adding zero frame front, ts "D64, ts);
#endif
      } else {

	for (adu_data_t *p = m_pending_adu_list; 
	     p != tailADU;
	     p = p->next_adu) {
#ifdef DEBUG_3119
	  mpa_message(LOG_DEBUG, "Adjusting "D64" to "D64, p->timestamp,
		      p->timestamp - m_rtp_ts_add);
#endif
	  p->timestamp -= m_rtp_ts_add;
	  p->freq_timestamp -= m_rtp_freq_ts_add;

	}
	ts = tailADU->timestamp - m_rtp_ts_add;
	freq_ts = tailADU->freq_timestamp - m_rtp_freq_ts_add;
	prevADU->next_adu = get_adu_data();
	prevADU = prevADU->next_adu;
#ifdef DEBUG_3119
	mpa_message(LOG_DEBUG, "Adding zero frame middle "D64, ts);
#endif
      }

      prevADU->next_adu = tailADU;
      SDL_UnlockMutex(m_rtp_packet_mutex);

      prevADU->pak = NULL;
      prevADU->frame_ptr = 
	(uint8_t *)malloc(tailADU->framesize);
      prevADU->aduDataSize = 0;
      prevADU->timestamp = ts;
      prevADU->freq_timestamp = freq_ts;
      prevADU->first_in_pak = 0;
      prevADU->last_in_pak = 0;
      prevADU->freeframe = 1;
      prevADU->headerSize = tailADU->headerSize;
      prevADU->sideInfoSize = tailADU->sideInfoSize;

      memcpy(prevADU->frame_ptr, 
	     tailADU->frame_ptr, 
	     prevADU->headerSize + prevADU->sideInfoSize);

      ZeroOutMP3SideInfo((unsigned char *)prevADU->frame_ptr, 
			 tailADU->framesize,
			 prevADUend);
      prevADU->mp3hdr = MP4AV_Mp3HeaderFromBytes(prevADU->frame_ptr);
      prevADU->framesize = MP4AV_Mp3GetFrameSize(prevADU->mp3hdr);
      prevADU->backpointer = MP4AV_Mp3GetAduOffset(prevADU->frame_ptr,
						   prevADU->framesize);
    } else {
      return;
    }
  }
}

bool CRfc3119RtpByteStream::start_next_frame (uint8_t **buffer, 
					      uint32_t *buflen,
					      frame_timestamp_t *ts,
					      void **ud)
{
  adu_data_t *p;

  // Free up last used...
  if (m_pending_adu_list != NULL) {
    SDL_LockMutex(m_rtp_packet_mutex);
    p = m_pending_adu_list->next_adu;
    free_adu(m_pending_adu_list);
    m_pending_adu_list = p;
    SDL_UnlockMutex(m_rtp_packet_mutex);
  }

  /*
   * load up ordered_adu_list here...
   */
  while (m_head != NULL && m_ordered_adu_list == NULL) {
    process_packet();
  }

  /*
   * Start moving things down to the pending list.  If it's NULL, 
   * and the ordered list has data that's not layer 3, just move the
   * first element of the ordered list to the pending list, and use that.
   */
  if (m_pending_adu_list == NULL) {
    if (MP4AV_Mp3GetHdrLayer(m_ordered_adu_list->mp3hdr) != 1) {
      //if (m_ordered_adu_list->mp3hdr.layer != 3)
      SDL_LockMutex(m_rtp_packet_mutex);
      m_pending_adu_list = m_ordered_adu_list;
      m_ordered_adu_list = m_ordered_adu_list->next_adu;
      m_pending_adu_list->next_adu = NULL;
      SDL_UnlockMutex(m_rtp_packet_mutex);
      /*
       * mpeg1 layer 1 or 2
       * No mpa robust - adu contains complete frame
       */
#ifdef DEBUG_3119
      mpa_message(LOG_DEBUG, "Not layer 3");
#endif
      *buffer = m_pending_adu_list->frame_ptr;
      *buflen = m_pending_adu_list->framesize;
      ts->msec_timestamp = m_pending_adu_list->timestamp;
      ts->audio_freq_timestamp = m_pending_adu_list->freq_timestamp;
      ts->audio_freq = m_freq;
      ts->timestamp_is_pts = false;
      return true;
    }
  }

  /*
   * Now we need to handle the mpeg1 layer 3 ADU frame to MP3
   * frame conversion
   */
  while (needToGetAnADU()) {
    if (m_ordered_adu_list == NULL) {
      // failure - we need to add, and can't
#ifdef DEBUG_3119
      mpa_message(LOG_DEBUG, "need to get an adu and ordered list is NULL");
#endif
      process_packet();
      if (m_ordered_adu_list == NULL) {
	dump_adu_list(m_pending_adu_list);
	m_pending_adu_list = NULL;
	*buffer = NULL;
	*buflen = 0;
	return false;
      }
    }
    add_and_insertDummyADUsIfNecessary();
  }

  uint32_t endOfHeadFrame;
  uint32_t frameOffset = 0;
  uint32_t toOffset = 0;

  /*
   * We should have enough on the pending list to fill up a frame
   */
  endOfHeadFrame = m_pending_adu_list->framesize;
  if (m_mp3_frame == NULL ||
      m_mp3_frame_size < endOfHeadFrame) {
    m_mp3_frame_size = endOfHeadFrame * 2;
    m_mp3_frame = (uint8_t *)realloc(m_mp3_frame, m_mp3_frame_size);
  }

  int copy;
  copy = m_pending_adu_list->headerSize + m_pending_adu_list->sideInfoSize;
  memcpy(m_mp3_frame, m_pending_adu_list->frame_ptr, copy);
  uint8_t *start_of_data;

  endOfHeadFrame -= copy;
  memset(m_mp3_frame + copy, 0, endOfHeadFrame);
  start_of_data = m_mp3_frame + copy;

  p = m_pending_adu_list;
  while (toOffset < endOfHeadFrame) {
    int startOfData = frameOffset - p->backpointer;
    if (startOfData > (int)endOfHeadFrame) {
      break;
    }
    int endOfData = startOfData + p->aduDataSize;
    if (endOfData > (int)endOfHeadFrame) {
      endOfData = endOfHeadFrame;
    }

    int fromOffset;
    if (startOfData <= (int)toOffset) {
      fromOffset = toOffset - startOfData;
      startOfData = toOffset;
      if (endOfData < startOfData) {
	endOfData = startOfData;
      }
    } else {
      fromOffset = 0;
      toOffset = startOfData;
    }
    int bytesUsedHere = endOfData - startOfData;

    memcpy(start_of_data + toOffset,
	   p->frame_ptr + p->headerSize + p->sideInfoSize + fromOffset,
	   bytesUsedHere);
    toOffset += bytesUsedHere;
    frameOffset += p->framesize - p->headerSize - p->sideInfoSize;
    p = p->next_adu;
  }

#ifdef DEBUG_3119
  mpa_message(LOG_DEBUG, "ts "U64", framesize %d", 
	      m_pending_adu_list->timestamp, m_pending_adu_list->framesize);
#endif
  *buffer = m_mp3_frame;
  *buflen = m_pending_adu_list->framesize;
  ts->msec_timestamp = m_pending_adu_list->timestamp;
  ts->audio_freq_timestamp = m_pending_adu_list->freq_timestamp;
  ts->audio_freq = m_freq;
  ts->timestamp_is_pts = false;

  return true;
}

void CRfc3119RtpByteStream::used_bytes_for_frame (uint32_t bytes)
{
  // we don't care - we'll move to the next frame ourselves.
}

void CRfc3119RtpByteStream::flush_rtp_packets (void)
{
  SDL_LockMutex(m_rtp_packet_mutex);
 
  dump_adu_list(m_deinterleave_list);
  m_deinterleave_list = NULL;
  dump_adu_list(m_ordered_adu_list);
  m_ordered_adu_list = NULL;
  dump_adu_list(m_pending_adu_list);
  m_pending_adu_list = NULL;

  SDL_UnlockMutex(m_rtp_packet_mutex);
  CRtpByteStreamBase::flush_rtp_packets();
}

void CRfc3119RtpByteStream::reset (void)
{
  CRtpByteStreamBase::reset();
}


bool CRfc3119RtpByteStream::have_frame (void)
{
  return (m_head != NULL); // || m_pending_adu_list == NULL);
}

void CRfc3119RtpByteStream::free_adu (adu_data_t *adu)
{
  if (adu->freeframe != 0) {
    free(adu->frame_ptr);
    adu->frame_ptr = NULL;
    adu->freeframe = 0;
  }
  if (adu->last_in_pak != 0 &&
      adu->pak != NULL) {
    xfree(adu->pak);
    adu->pak = NULL;
  }
  adu->next_adu = m_adu_data_free;
  m_adu_data_free = adu;
}
