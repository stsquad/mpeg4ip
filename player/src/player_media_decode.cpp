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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * player_media_decode.cpp
 * decode task thread for a CPlayerMedia
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"

#include <rtp/memory.h>
#include "mpeg4.h"
#include "codec/aa/aa.h"

/*
 * CPlayerMedia::advance_head - move to the next rtp packet in the
 * bytestream.  This really should be in the rtp bytestream.
 */
rtp_packet *CPlayerMedia::advance_head (int bookmark_set, const char **err) 
{
  rtp_packet *p;
  uint16_t nextseq;

  if (bookmark_set == 1) {
    /* 
     * If we're doing bookmark - make sure we keep the previous packets
     * around
     */
    if (SDL_mutexP(m_rtp_packet_mutex) == -1) {
      player_error_message("SDL Lock mutex failure in decode thread");
      return (NULL);
    }
    if (m_head == NULL || m_head == m_head->next) {
      SDL_mutexV(m_rtp_packet_mutex);
      *err = "bookmark - advance past end";
      return (NULL);
    }
    p = m_head->next;
    nextseq = m_head->seq + 1;
    if (nextseq != p->seq) {
      p = m_head;
      m_head = m_head->next;
      m_tail->next = m_head;
      m_head->prev = m_tail;
      xfree(p);
      *err = "bookmark Sequence number violation";
      p = NULL;
    }
    SDL_mutexV(m_rtp_packet_mutex);
    return (p);
  }

  if (SDL_mutexP(m_rtp_packet_mutex) == -1) {
      player_error_message("SDL Lock mutex failure in decode thread");
      return (NULL);
  }

  p = m_head;
  if (m_head->next == m_head) {
    m_head = NULL;
  } else {
    m_head = p->next;
    m_head->prev = m_tail;
    m_tail->next = m_head;
  }
  if (SDL_mutexV(m_rtp_packet_mutex) == -1) {
    player_error_message("SDL unlock mutex failure in decode thread");
    return (NULL);
  }
  nextseq = p->seq + 1;
  xfree(p);

  /*
   * Check the sequence number...
   */
  if (m_head && m_head->seq == nextseq) 
    return (m_head);

  if (m_head) {
    *err = "Sequence number violation";
  } else {
    *err = "No more data";
  }
  return (NULL);
}

/*
 * parse_decode_message - handle messages to the decode task
 */
void CPlayerMedia::parse_decode_message (int &thread_stop, int &decoding)
{
  CMsg *newmsg;

  if ((newmsg = m_decode_msg_queue.get_message()) != NULL) {
    //player_debug_message("Here's the decode message %d",newmsg->get_value());
    switch (newmsg->get_value()) {
    case MSG_STOP_THREAD:
      thread_stop = 1;
      break;
    case MSG_PAUSE_SESSION:
      decoding = 0;
      if (m_video_sync != NULL) {
	m_video_sync->flush_decode_buffers();
      }
      if (m_audio_sync != NULL) {
	m_audio_sync->flush_decode_buffers();
      }
      break;
    case MSG_START_DECODING:
      if (m_video_sync != NULL) {
	m_video_sync->flush_decode_buffers();
      }
      if (m_audio_sync != NULL) {
	m_audio_sync->flush_decode_buffers();
      }
      decoding = 1;
      break;
    }
    delete newmsg;
  }
}

/*
 * Main decode thread.
 */
int CPlayerMedia::decode_thread (void) 
{

  //  uint32_t msec_per_frame = 0;
  CCodecBase *codec = NULL;
  int ret = 0;
#ifdef TIME_DECODE
  int64_t avg = 0, diff;
  int64_t max = 0;
  int avg_cnt = 0;
#endif
  int thread_stop = 0, decoding = 0;
  uint codec_proto = 0;

  while (thread_stop == 0) {
    // waiting here for decoding or thread stop
    ret = SDL_SemWait(m_decode_thread_sem);
    //    player_debug_message("Decode thread awake");
    parse_decode_message(thread_stop, decoding);

    if (decoding == 1) {
      // We've been told to start decoding - if we don't have a codec, 
      // create one
      if (codec == NULL) {
	/*
	 * We need a better way to do this for multiple codecs
	 * We should probably set something in the media, then do
	 * some sort of look up.
	 */
	if (is_video()) {
	  m_video_sync->set_sync_sem(m_parent->get_sync_sem());
	  m_video_sync->set_wait_sem(m_decode_thread_sem);
	  codec = new CMpeg4Codec(m_video_sync, 
				  m_byte_stream,
				  m_media_fmt,
				  m_video_info);
	} else {
	  m_audio_sync->set_sync_sem(m_parent->get_sync_sem());
	  m_audio_sync->set_wait_sem(m_decode_thread_sem);
	  codec = new CAACodec(m_audio_sync, 
			       m_byte_stream, 
			       m_media_fmt,
			       m_audio_info);
	}
	codec_proto = m_rtp_proto;
      } else {
	codec->do_pause();
	// to do - compare with m_rtp_proto
      }
    }
    /*
     * this is our main decode loop
     */
    //player_debug_message("Into decode loop");
    while ((thread_stop == 0) && decoding) {
      parse_decode_message(thread_stop, decoding);
      if (thread_stop != 0)
	continue;
      if (decoding == 0) {
	codec->do_pause();
	if (m_rtp_byte_stream)
	  m_byte_stream->reset(); // zero out our rtp bytestream
	flush_rtp_packets();
	continue;
      }
      if (m_byte_stream->eof()) {
	player_debug_message("hit eof");
	if (m_audio_sync) m_audio_sync->set_eof();
	if (m_video_sync) m_video_sync->set_eof();
	decoding = 0;
	continue;
      }
      if (m_byte_stream->have_no_data()) {
	// Indicate that we're waiting, and wait for a message from RTP
	// task.
	m_decode_thread_waiting = 1;
	ret = SDL_SemWait(m_decode_thread_sem);
	m_decode_thread_waiting = 0;
	continue;
      }

      uint64_t ourtime;
      // Tell bytestream we're starting the next frame - they'll give us
      // the time.
      ourtime = m_byte_stream->start_next_frame();
      //player_debug_message("Decoding frame %llu", ourtime);
      do {
#ifdef TIME_DECODE
	clock_t start, end;
	start = clock();
#endif
	ret = codec->decode(ourtime, m_rtp_byte_stream != NULL);
#ifdef TIME_DECODE
	end = clock();
	if (ret > 0) {
	  diff = end - start;
	  if (diff > max) max = diff;
	  avg += diff;
	  avg_cnt++;
#if 0
	  if ((avg_cnt % 100) == 0) {
	    player_debug_message("Decode avg time is %lld", avg / avg_cnt);
	  }
#endif
	}
#endif
      } while (ret >= 0 && 
	       m_byte_stream->still_same_ts());
#if 0
      // This section will tell if the codec finishes
      if (m_rtp_byte_stream->m_offset_in_pak != 0) {
	player_debug_message("not at end - seq %x ts %x offset %d len %d", 
			     m_rtp_byte_stream->m_pak->seq,
			     m_rtp_byte_stream->m_pak->ts, 
			     m_rtp_byte_stream->m_offset_in_pak,
			     m_rtp_byte_stream->m_pak->data_len);
      }
#endif

    }
    //player_debug_message("decode- out of inner loop");
  }
#ifdef TIME_DECODE
  if (avg_cnt != 0)
    player_debug_message("Decode avg time is %lld max %lld", avg / avg_cnt,
			 max);
#endif
  delete codec;
  return (0);
}

/* end file player_media_decode.cpp */
