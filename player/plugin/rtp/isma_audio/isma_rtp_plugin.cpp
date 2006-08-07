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

#include "isma_rtp_plugin.h"
#include "mp4util/mpeg4_audio_config.h"
#include "rtp/rtp.h"
//#define DEBUG_ISMA_AAC

#define isma_message iptr->m_vft->log_msg
static const char *ismartp="ismartp";

static rtp_check_return_t check (lib_message_func_t msg, 
				 format_list_t *fmt, 
				 uint8_t rtp_payload_type,
				 CConfigSet *pConfig)
{

  if (fmt == NULL || fmt->rtpmap_name == NULL) 
    return RTP_PLUGIN_NO_MATCH;

  if (strcasecmp(fmt->rtpmap_name, "mpeg4-generic") != 0) {
    return RTP_PLUGIN_NO_MATCH;
  }

  fmtp_parse_t *fmtp;
  int len;
  fmtp = parse_fmtp_for_mpeg4(fmt->fmt_param, msg);
  if (fmtp == NULL) return RTP_PLUGIN_NO_MATCH;

  len = fmtp->size_length;
  free_fmtp_parse(fmtp);
  if (len == 0) {
    return RTP_PLUGIN_NO_MATCH;
  }
  return RTP_PLUGIN_MATCH;
}

static isma_frame_data_t *get_frame_data (isma_rtp_data_t *iptr) 
{
  isma_frame_data_t *pak;
  if (iptr->m_frame_data_free == NULL) {
    pak = (isma_frame_data_t *)malloc(sizeof(isma_frame_data_t));
    if (pak == NULL) return NULL;
  } else {
    pak = iptr->m_frame_data_free;
    iptr->m_frame_data_free = pak->frame_data_next;
  }
  pak->frame_data_next = NULL;
  pak->last_in_pak = 0;
  return (pak);
}
/*
 * Isma rtp bytestream has a potential set of headers at the beginning
 * of each rtp frame.  This can interleave frames in different packets
 */
rtp_plugin_data_t *isma_rtp_plugin_create (format_list_t *media_fmt,
					   uint8_t rtp_payload_type, 
					   rtp_vft_t *vft,
					   void *ifptr)
{
  isma_rtp_data_t *iptr;
  fmtp_parse_t *fmtp;

  iptr = MALLOC_STRUCTURE(isma_rtp_data_t);
  memset(iptr, 0, sizeof(isma_rtp_data_t));
  iptr->m_vft = vft;
  iptr->m_ifptr = ifptr;

  iptr->m_rtp_packet_mutex = SDL_CreateMutex();
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  iptr->m_outfile = fopen("isma.aac", "w");
#endif
  iptr->m_frame_data_head = NULL;
  iptr->m_frame_data_on = NULL;
  iptr->m_frame_data_free = NULL;
  isma_frame_data_t *p;
  for (iptr->m_frame_data_max = 0; iptr->m_frame_data_max < 25; iptr->m_frame_data_max++) {
    p = (isma_frame_data_t *)malloc(sizeof(isma_frame_data_t));
    p->frame_data_next = iptr->m_frame_data_free;
    iptr->m_frame_data_free = p;
  }

  fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param, iptr->m_vft->log_msg);

  mpeg4_audio_config_t audio_config;
  decode_mpeg4_audio_config(fmtp->config_binary,
			    fmtp->config_binary_len,
			    &audio_config, 
			    false);
  if (audio_object_type_is_aac(&audio_config)) {
    iptr->m_rtp_ts_add = audio_config.codec.aac.frame_len_1024 != 0 ? 1024 : 960;
  } else {
    iptr->m_rtp_ts_add = audio_config.codec.celp.samples_per_frame;
    isma_message(LOG_DEBUG, ismartp, "celp spf is %d", iptr->m_rtp_ts_add);
  }
  iptr->m_rtp_ts_add = (iptr->m_rtp_ts_add * media_fmt->rtpmap_clock_rate) /
    audio_config.frequency;
  isma_message(LOG_DEBUG, ismartp,
	       "Rtp ts add is %d (%d %d)", iptr->m_rtp_ts_add,
	       media_fmt->rtpmap_clock_rate, 
	       audio_config.frequency);
  iptr->m_fmtp = fmtp;
  iptr->m_min_first_header_bits = iptr->m_fmtp->size_length + iptr->m_fmtp->index_length;
  iptr->m_min_header_bits = iptr->m_fmtp->size_length + iptr->m_fmtp->index_delta_length;
  if (iptr->m_fmtp->CTS_delta_length > 0) {
    iptr->m_min_header_bits++;
    iptr->m_min_first_header_bits++;
  }
  if (iptr->m_fmtp->DTS_delta_length > 0) {
    iptr->m_min_header_bits++;
    iptr->m_min_first_header_bits++;
  }
  isma_message(LOG_DEBUG, ismartp, "min headers are %d %d", iptr->m_min_first_header_bits,
	       iptr->m_min_header_bits);

  iptr->m_min_header_bits += iptr->m_fmtp->auxiliary_data_size_length;
  iptr->m_min_first_header_bits += iptr->m_fmtp->auxiliary_data_size_length;
  iptr->m_frag_reass_buffer = NULL;
  iptr->m_frag_reass_size_max = 0;
  return (&iptr->plug);
}


static void isma_rtp_destroy (rtp_plugin_data_t *pifptr)
{
  isma_rtp_data_t *iptr = (isma_rtp_data_t *)pifptr;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  fclose(iptr->m_outfile);
#endif
  isma_frame_data_t *p;
  
  if (iptr->m_frag_reass_buffer != NULL) {
    free(iptr->m_frag_reass_buffer);
    iptr->m_frag_reass_buffer = NULL;
  }
  if (iptr->m_frame_data_on != NULL) {
    iptr->m_frame_data_on->frame_data_next = iptr->m_frame_data_head;
    iptr->m_frame_data_head = iptr->m_frame_data_on;
    iptr->m_frame_data_on = NULL;
  }
  while (iptr->m_frame_data_free != NULL) {
    p = iptr->m_frame_data_free;
    iptr->m_frame_data_free = p->frame_data_next;
    free(p);
  }
  while (iptr->m_frame_data_head != NULL) {
    p = iptr->m_frame_data_head;
    // if fragmented frame, free all frag_data
    if (p->is_fragment == 1) {
      isma_frag_data_t * q = p->frag_data;
      while (q != NULL) {
	p->frag_data = q->frag_data_next;
	free(q);
	q = p->frag_data;
      }
    }
    iptr->m_frame_data_head = p->frame_data_next;
    free(p);
  }
  if (iptr->m_fmtp != NULL) {
    free_fmtp_parse(iptr->m_fmtp);
  }
  free(iptr);
}

static int insert_frame_data (isma_rtp_data_t *iptr, 
			      isma_frame_data_t *frame_data)
{
  SDL_LockMutex(iptr->m_rtp_packet_mutex);
  if (iptr->m_frame_data_head == NULL) {
    iptr->m_frame_data_head = frame_data;
  } else {
    int32_t diff;
    isma_frame_data_t *p, *q;
    q = NULL;
    p = iptr->m_frame_data_head;

    do {
      diff = frame_data->rtp_timestamp - p->rtp_timestamp;
      if (diff == 0) {
	isma_message(LOG_ERR, ismartp, "Duplicate timestamp of %x found in RTP packet",
		     frame_data->rtp_timestamp);
	isma_message(LOG_DEBUG, ismartp,
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
	frame_data->frame_data_next = iptr->m_frame_data_free;
	iptr->m_frame_data_free = frame_data;

	SDL_UnlockMutex(iptr->m_rtp_packet_mutex);
	return 1;
      } else if (diff < 0) {
	if (q == NULL) {
	  frame_data->frame_data_next = iptr->m_frame_data_head;
	  iptr->m_frame_data_head = frame_data;
	} else {
	  q->frame_data_next = frame_data;
	  frame_data->frame_data_next = p;
	}
	SDL_UnlockMutex(iptr->m_rtp_packet_mutex);
	return 0;
      }
      q = p;
      p = p->frame_data_next;
    } while (p != NULL);
    // insert at end;
    q->frame_data_next = frame_data;

  }
  SDL_UnlockMutex(iptr->m_rtp_packet_mutex);
  return 0;
}

static void get_au_header_bits (isma_rtp_data_t *iptr)
{
  uint32_t temp = 0;
  if (iptr->m_fmtp->CTS_delta_length > 0) {
    iptr->m_header_bitstream.getbits(1, &temp);
    if (temp > 0) {
      iptr->m_header_bitstream.getbits(iptr->m_fmtp->CTS_delta_length, &temp);
    }
  }
  if (iptr->m_fmtp->DTS_delta_length > 0) {
    iptr->m_header_bitstream.getbits(1, &temp);
    if (temp > 0) {
      iptr->m_header_bitstream.getbits(iptr->m_fmtp->DTS_delta_length, &temp);
    }
  }
}

// check where need to lock 
static void cleanup_frag (isma_rtp_data_t *iptr, 
			  isma_frame_data_t * frame_data)
{
  // free all frag_data for this frame
  isma_frag_data_t * p = NULL;
  while ((p = frame_data->frag_data) != NULL) {
    frame_data->frag_data = p->frag_data_next;
    free(p);
  } 
  // now put frame_data back on free list
  SDL_LockMutex(iptr->m_rtp_packet_mutex);
  frame_data->frame_data_next = iptr->m_frame_data_free;
  iptr->m_frame_data_free = frame_data;
  SDL_UnlockMutex(iptr->m_rtp_packet_mutex);
  return;
}

// Frame is fragmented.
// Process next RTP paks until have the entire frame.
// Paks will be in order in the queue, but maybe some missing?
// So if process pkt w/ Mbit set (last of fragm) and total < frameLength
// ignore pkt.
// Insert frame data only after got all fragments for the frame.
static int process_fragment (isma_rtp_data_t *iptr, 
			     rtp_packet *pak, 
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
      cleanup_frag(iptr, frame_data);
      isma_message(LOG_ERR, ismartp, "Error processing frag: early mBit");
      return (1); 
    }
    if (pak == NULL) {
      cleanup_frag(iptr, frame_data);
      isma_message(LOG_ERR, ismartp, "Error processing frag: not enough packets");
      return (1);
    }
    // check if ts and rtp seq numbers are ok, and lengths match
    if (ts != pak->rtp_pak_ts) {
      cleanup_frag(iptr, frame_data);
      isma_message(LOG_ERR, ismartp, 
		   "Error processing frag: wrong ts: ts= %x, pak->ts = %x", 
		   ts, pak->rtp_pak_ts);
      return (1);
    }
    if (seq != pak->rtp_pak_seq) {
      cleanup_frag(iptr, frame_data);
      isma_message(LOG_ERR, ismartp, "Error processing frag: wrong seq num");
      return (1);
    }
    // insert fragment info
    isma_frag_data_t *p = (isma_frag_data_t *)
      malloc(sizeof(isma_frag_data_t));
    if (p == NULL) {
      isma_message(LOG_ERR, ismartp, "Error processing frag: can't malloc");
      iptr->m_vft->free_pak(pak);
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
    iptr->m_header_bitstream.init(&pak->rtp_data[sizeof(uint16_t)], header_len);
    // frag_ptr should just point to beginning of data in pkt
    uint32_t header_len_bytes = ((header_len + 7) / 8) + sizeof(uint16_t);
    cur->frag_ptr =  &pak->rtp_data[header_len_bytes];
    cur->frag_len = pak->rtp_data_len - header_len_bytes;
    // if aux data, move frag pointer
    if (iptr->m_fmtp->auxiliary_data_size_length > 0) {
      iptr->m_header_bitstream.byte_align();
      uint32_t aux_len = 0;
      iptr->m_header_bitstream.getbits(iptr->m_fmtp->auxiliary_data_size_length, &aux_len);
      aux_len = (aux_len + 7) / 8;
      cur->frag_ptr += aux_len;
      cur->frag_len -= aux_len;
    }
    total_len += cur->frag_len;
#ifdef DEBUG_ISMA_RTP_FRAGS
    isma_message(LOG_DEBUG, ismartp, 
		 "rtp seq# %d, fraglen: %d, ts: %x", 
		 pak->seq, cur->frag_len, pak->ts);
#endif	
    seq = pak->rtp_pak_seq + 1;
    if (pak->rtp_pak_m) 
      read_mBit = 1;
    iptr->m_vft->remove_from_list(iptr->m_ifptr, pak);
    pak = iptr->m_vft->get_next_pak(iptr->m_ifptr, NULL, 0);
  } while (total_len < frame_data->frame_len);

  // insert frame and return
  int error = insert_frame_data(iptr, frame_data);
  frame_data->last_in_pak = 1; // only one frame in pak
  // no need to remove pkt from queue, done at the end of do-while
  return (error);
}

static void process_packet_header (isma_rtp_data_t *iptr)
{
  rtp_packet *pak;
  uint32_t frame_len;
  uint16_t header_len;
  uint32_t retvalue;

  pak = iptr->m_vft->get_head_and_check(iptr->m_ifptr, false, 0);
  if (pak == NULL) {
    return;
  }
#ifdef DEBUG_ISMA_AAC
  isma_message(LOG_DEBUG, ismartp, 
	       "processing pak seq %d ts %x len %d", 
	       pak->rtp_pak_seq, pak->rtp_pak_ts, pak->rtp_data_len);
#endif
  // This pak has not had it's header processed
  // length in bytes
  if (pak->rtp_data_len == 0) {
    iptr->m_vft->free_pak(pak);
    isma_message(LOG_ERR, ismartp, "RTP audio packet with data length of 0");
    return;
  }

  header_len = ntohs(*(unsigned short *)pak->rtp_data);
  if (header_len < iptr->m_min_first_header_bits) {
    // bye bye, frame...
    iptr->m_vft->free_pak(pak);
    isma_message(LOG_ERR, ismartp, "ISMA rtp - header len %d less than min %d", 
		 header_len, iptr->m_min_first_header_bits);
    return;
  }

  iptr->m_header_bitstream.init(&pak->rtp_data[sizeof(uint16_t)],
			  header_len);
  if (iptr->m_header_bitstream.getbits(iptr->m_fmtp->size_length, &frame_len) != 0) 
    return;
  iptr->m_header_bitstream.getbits(iptr->m_fmtp->index_length, &retvalue);
  get_au_header_bits(iptr);
#ifdef DEBUG_ISMA_AAC
  uint64_t msec = iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, pak->rtp_pak_ts, 
					      pak->pd.rtp_pd_timestamp,
					      1);
  isma_message(LOG_DEBUG, ismartp, 
	       "1st - header len %u frame len %u ts %x "U64, 
	       header_len, frame_len, pak->rtp_pak_ts, msec);
#endif
  if (frame_len == 0) {
    iptr->m_vft->free_pak(pak);
    return;
  }
  uint8_t *frame_ptr;
  isma_frame_data_t *frame_data;
  uint32_t ts;
  ts = pak->rtp_pak_ts;
  frame_data = get_frame_data(iptr);
  frame_data->pak = pak;
  // frame pointer is after header_len + header_len size.  Header_len
  // is in bits - add 7, divide by 8 to get padding correctly.
  frame_data->frame_ptr = &pak->rtp_data[((header_len + 7) / 8) 
				    + sizeof(uint16_t)];
  frame_data->frame_len = frame_len;
  frame_data->rtp_timestamp = ts;

  // Check if frame is fragmented
  // frame_len plus the length of the 2 headers
  uint32_t frag_check = frame_len + sizeof(uint16_t);
  frag_check += iptr->m_fmtp->size_length / 8;
  if ((iptr->m_fmtp->size_length % 8) != 0) frag_check++;
  if (frag_check > pak->rtp_data_len) {
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, ismartp, "Frame is fragmented");
#endif
    frame_data->is_fragment = 1;
    int err = process_fragment(iptr, pak, frame_data);
    if (err == 1)
      isma_message(LOG_ERR, ismartp, "Error in processing the fragment");
    return; 
  }
  else {
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, ismartp, "Frame is not fragmented");
#endif
    frame_data->is_fragment = 0;
    frame_data->frag_data = NULL;
  }
  int error = insert_frame_data(iptr, frame_data);
  frame_ptr = frame_data->frame_ptr + frame_data->frame_len;
  while (iptr->m_header_bitstream.bits_remain() >= iptr->m_min_header_bits) {
    uint32_t stride = 0;
    iptr->m_header_bitstream.getbits(iptr->m_fmtp->size_length, &frame_len);
    iptr->m_header_bitstream.getbits(iptr->m_fmtp->index_delta_length, &stride);
    get_au_header_bits(iptr);
    ts += (iptr->m_rtp_ts_add * (1 + stride));
#ifdef DEBUG_ISMA_AAC
    msec = iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				       pak->rtp_pak_ts, 
				       pak->pd.rtp_pd_timestamp,
				       1);
    isma_message(LOG_DEBUG, ismartp, 
		 "Stride %d len %d ts %x "U64,
		 stride, frame_len, ts, msec);
#endif
    frame_data = get_frame_data(iptr);
    frame_data->pak = pak;
    frame_data->is_fragment = 0;
    frame_data->frag_data = NULL;
    frame_data->frame_ptr = frame_ptr;
    frame_data->frame_len = frame_len;
    frame_ptr += frame_len;
    frame_data->rtp_timestamp = ts;
    error |= insert_frame_data(iptr, frame_data);
  }
  if (error == 0 && frame_data != NULL) { 
    frame_data->last_in_pak = 1;
  }
  else {
    isma_frame_data_t *p, *last = NULL;
    p = iptr->m_frame_data_head;
    while (p != NULL) {
      if (p->pak == pak) last = p;
      p = p->frame_data_next;
    }
    if (last != NULL) {
      last->last_in_pak = 1;
      isma_message(LOG_WARNING, ismartp, "error at end - marked ts %x", last->rtp_timestamp);
    } else {
      // Didn't find pak in list.  Weird
      isma_message(LOG_ERR, ismartp, 
		   "Decoded packet with RTP timestamp %x and didn't"
		   "see any good frames", pak->rtp_pak_ts);
      iptr->m_vft->free_pak(pak);
      return;
    }
  }
  if (iptr->m_fmtp->auxiliary_data_size_length > 0) {
    iptr->m_header_bitstream.byte_align();
    uint32_t aux_len = 0;
    iptr->m_header_bitstream.getbits(iptr->m_fmtp->auxiliary_data_size_length, &aux_len);
    aux_len = (aux_len + 7) / 8;
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, ismartp, "Adding %d bytes for aux data size", aux_len);
#endif
    isma_frame_data_t *p;
    p = iptr->m_frame_data_head;
    while (p != NULL) {
      if (p->pak == pak) {
	p->frame_ptr += aux_len;
      }
      p = p->frame_data_next;
    }
  }
}

static bool start_next_frame (rtp_plugin_data_t *pifptr, 
			      uint8_t **buffer, 
			      uint32_t *buflen,
			      frame_timestamp_t *ts,
			      void **userdata)
{
  isma_rtp_data_t *iptr = (isma_rtp_data_t *)pifptr;
  uint64_t timetick;

  if (iptr->m_frame_data_on != NULL) {
    uint32_t next_ts;
#ifdef DEBUG_ISMA_AAC
    isma_message(LOG_DEBUG, ismartp, "Advancing to next pak data - old ts %x", 
		 iptr->m_frame_data_on->rtp_timestamp);
#endif
    if (iptr->m_frame_data_on->last_in_pak != 0) {
      // We're done with all the data in this packet - get rid
      // of the rtp packet.
      if (iptr->m_frame_data_on->is_fragment == 1) {
	// if fragmented, need to get rid of all paks pointed to
	isma_frag_data_t *q =  iptr->m_frame_data_on->frag_data;
	while (q != NULL) {
	  rtp_packet *pak = q->pak;
	  q->pak = NULL;
	  if (pak != NULL) {
	    iptr->m_vft->free_pak(pak);
	  }
	  q = q->frag_data_next;
#ifdef DEBUG_ISMA_AAC
	  isma_message(LOG_DEBUG, ismartp, "removing pak - frag %d", pak->rtp_pak_seq);
#endif
	}
      } else {
	rtp_packet *pak = iptr->m_frame_data_on->pak;
	iptr->m_frame_data_on->pak = NULL;
	iptr->m_vft->free_pak(pak);
#ifdef DEBUG_ISMA_AAC
	isma_message(LOG_DEBUG, ismartp, "removing pak %d", pak->rtp_pak_seq);
#endif
      }
    }
    /*
     * Remove the frame data head pointer, and put it on the free list
     */
    isma_frame_data_t *p = NULL;
    SDL_LockMutex(iptr->m_rtp_packet_mutex);
    p = iptr->m_frame_data_on;
    iptr->m_frame_data_on = NULL;
    next_ts = p->rtp_timestamp;
    p->frame_data_next = iptr->m_frame_data_free;
    iptr->m_frame_data_free = p;
    // free all frag_data for this frame
    if (p->is_fragment == 1) {
      isma_frag_data_t * q = p->frag_data;
      while (q != NULL) {
	p->frag_data = q->frag_data_next;
	free(q);
	q = p->frag_data;
      } 
    }
    SDL_UnlockMutex(iptr->m_rtp_packet_mutex);

    /* 
     * Now, look for the next timestamp - process a bunch of new
     * rtp packets, if we have to...
     */
    next_ts += iptr->m_rtp_ts_add;
    if (iptr->m_frame_data_head == NULL ||
	iptr->m_frame_data_head->rtp_timestamp != next_ts) {
      // process next pak in list.  Process until next timestamp is found, 
      // or 500 msec worth of data is found (in which case, go with first)
      do {
	process_packet_header(iptr);
      } while (iptr->m_vft->get_next_pak(iptr->m_ifptr, NULL, 0) != NULL && 
	       ((iptr->m_frame_data_head == NULL) || 
		(iptr->m_frame_data_head->rtp_timestamp != next_ts)) &&  
	       (iptr->m_frame_data_free != NULL));
    }
#ifdef DEBUG_ISMA_AAC
    else {
      // iptr->m_frame_data_head is correct
      isma_message(LOG_DEBUG, ismartp, "frame_data_head is correct");
    }
#endif
  } else {
    // first time.  Process a bunch of packets, go with first one...
    // asdf - will want to eventually add code to drop the first couple
    // of packets if they're not consecutive.
    do {
      process_packet_header(iptr);
    } while ((iptr->m_vft->get_next_pak(iptr->m_ifptr, NULL, 0) != NULL) &&
	     (iptr->m_frame_data_free != NULL));
  }
  /*
   * Init up the offsets
   */
  if (iptr->m_frame_data_head != NULL) {
    SDL_LockMutex(iptr->m_rtp_packet_mutex);
    iptr->m_frame_data_on = iptr->m_frame_data_head;
    iptr->m_frame_data_head = iptr->m_frame_data_head->frame_data_next;
    SDL_UnlockMutex(iptr->m_rtp_packet_mutex);

    if (iptr->m_frame_data_on->is_fragment == 1) {	  

      iptr->m_frag_reass_size = 0;
      isma_frag_data_t *ptr;
      ptr = iptr->m_frame_data_on->frag_data;
      while (ptr != NULL) {
	if (iptr->m_frag_reass_size + ptr->frag_len > iptr->m_frag_reass_size_max) {
	  iptr->m_frag_reass_size_max += MAX(4096, ptr->frag_len);
	  iptr->m_frag_reass_buffer = 
	    (uint8_t *)realloc(iptr->m_frag_reass_buffer, 
			       iptr->m_frag_reass_size_max);
	}
	memmove(iptr->m_frag_reass_buffer + iptr->m_frag_reass_size, 
		ptr->frag_ptr,
		ptr->frag_len);
	iptr->m_frag_reass_size += ptr->frag_len;
	ptr = ptr->frag_data_next;
      }
      *buffer = iptr->m_frag_reass_buffer;
      *buflen = iptr->m_frag_reass_size;
    } else { 
      *buffer = iptr->m_frame_data_on->frame_ptr;
      *buflen = iptr->m_frame_data_on->frame_len;
    }
  } else {
    *buffer = NULL;
  }
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  if (*buffer != NULL) {
    fwrite(*buffer, *buflen,  1, iptr->m_outfile);
  }
#endif
  timetick = 
    iptr->m_vft->rtp_ts_to_msec(iptr->m_ifptr, 
				iptr->m_frame_data_on != NULL ? 
				iptr->m_frame_data_on->rtp_timestamp : 
				iptr->m_ts, 
				iptr->m_frame_data_on ?
				iptr->m_frame_data_on->pak->pd.rtp_pd_timestamp : 0,
				0);

  if (iptr->m_frame_data_on != NULL)
    iptr->m_ts =  iptr->m_frame_data_on->rtp_timestamp;
  
  // We're going to have to handle wrap better...
#ifdef DEBUG_ISMA_AAC
  isma_message(LOG_DEBUG, ismartp, "start next frame %p %d ts "X64" "U64, 
	       *buffer, *buflen, iptr->m_ts, timetick);
#endif
  ts->audio_freq_timestamp = iptr->m_ts;
  ts->msec_timestamp = timetick;
  ts->timestamp_is_pts = true;
  return (true);
}

static void used_bytes_for_frame (rtp_plugin_data_t *pifptr, uint32_t bytes)
{
}

static void reset (rtp_plugin_data_t *pifptr)
{
}

static void flush_rtp_packets (rtp_plugin_data_t *pifptr)
{
  isma_rtp_data_t *iptr = (isma_rtp_data_t *)pifptr;

  isma_frame_data_t *p;
  SDL_LockMutex(iptr->m_rtp_packet_mutex);
  if (iptr->m_frame_data_on != NULL) {
    iptr->m_frame_data_on->frame_data_next = iptr->m_frame_data_head;
    iptr->m_frame_data_head = iptr->m_frame_data_on;
    iptr->m_frame_data_on = NULL;
  }
  if (iptr->m_frame_data_head != NULL) {
    p = iptr->m_frame_data_head;
    while (p->frame_data_next != NULL) {
#ifdef DEBUG_ISMA_AAC
      isma_message(LOG_DEBUG, ismartp, "reset removing pak %d", p->pak->rtp_pak_seq);
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
	iptr->m_vft->free_pak(p->pak);
      }
      p = p->frame_data_next;
    }
    p->frame_data_next = iptr->m_frame_data_free;
    iptr->m_frame_data_free = iptr->m_frame_data_head;
    iptr->m_frame_data_head = NULL;
  }
  SDL_UnlockMutex(iptr->m_rtp_packet_mutex);
}

static bool have_frame (rtp_plugin_data_t *pifptr)
{
  isma_rtp_data_t *iptr = (isma_rtp_data_t *)pifptr;
  if (iptr->m_frame_data_head != NULL) return true;

  return (iptr->m_vft->get_next_pak(iptr->m_ifptr, NULL, 0) != NULL);
}

RTP_PLUGIN("mpeg4-generic", 
	   check,
	   isma_rtp_plugin_create,
	   isma_rtp_destroy,
	   start_next_frame, 
	   used_bytes_for_frame,
	   reset, 
	   flush_rtp_packets,
	   have_frame,
	   NULL,
	   0);
