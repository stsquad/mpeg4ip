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
#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"
#include "media_utils.h"
#include "audio.h"
#include "video.h"
#include <rtp/memory.h>

//#define DEBUG_DECODE 1
//#define DEBUG_DECODE_MSGS 1

/*
 * parse_decode_message - handle messages to the decode task
 */
void CPlayerMedia::parse_decode_message (int &thread_stop, int &decoding)
{
  CMsg *newmsg;

  if ((newmsg = m_decode_msg_queue.get_message()) != NULL) {
#ifdef DEBUG_DECODE_MSGS
    player_debug_message("Here's the decode message %d",newmsg->get_value());
#endif
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

  while (thread_stop == 0) {
    // waiting here for decoding or thread stop
    ret = SDL_SemWait(m_decode_thread_sem);
#ifdef DEBUG_DECODE
    player_debug_message("Decode thread awake");
#endif
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
	  m_video_sync->set_wait_sem(m_decode_thread_sem);
	  codec = start_video_codec(m_codec_type,
				    m_video_sync, 
				    m_byte_stream,
				    m_media_fmt,
				    m_video_info,
				    m_user_data,
				    m_user_data_size);
	} else {
	  m_audio_sync->set_wait_sem(m_decode_thread_sem);
	  codec = start_audio_codec(m_codec_type,
				    m_audio_sync, 
				    m_byte_stream, 
				    m_media_fmt,
				    m_audio_info,
				    m_user_data,
				    m_user_data_size);
	}
	if (codec == NULL) {
	  while (thread_stop == 0 && decoding) {
	    SDL_Delay(100);
	    if (m_rtp_byte_stream) {
	      m_rtp_byte_stream->flush_rtp_packets();
	    }
	    parse_decode_message(thread_stop, decoding);
	  }
	} else {
	  m_byte_stream->set_codec(codec);
	}
      } else {
	codec->do_pause();
	// to do - compare with m_rtp_proto
      }
    }
    /*
     * this is our main decode loop
     */
#ifdef DEBUG_DECODE
    player_debug_message("Into decode loop");
#endif
    while ((thread_stop == 0) && decoding) {
      parse_decode_message(thread_stop, decoding);
      if (thread_stop != 0)
	continue;
      if (decoding == 0) {
	codec->do_pause();
	continue;
      }
      if (m_byte_stream->eof()) {
	player_debug_message("%s hit eof", m_is_video ? "video" : "audio");
	if (m_audio_sync) m_audio_sync->set_eof();
	if (m_video_sync) m_video_sync->set_eof();
	decoding = 0;
	continue;
      }
      if (m_byte_stream->have_no_data()) {
	// Indicate that we're waiting, and wait for a message from RTP
	// task.
	m_decode_thread_waiting = 1;
#ifdef DEBUG_DECODE
	player_debug_message("decode thread %s waiting", m_media_info->media);
#endif
	ret = SDL_SemWait(m_decode_thread_sem);
	m_decode_thread_waiting = 0;
	continue;
      }

      uint64_t ourtime;
      // Tell bytestream we're starting the next frame - they'll give us
      // the time.
      ourtime = m_byte_stream->start_next_frame();
#if 0
      if (is_video() &&
	  (m_parent->get_session_state() == SESSION_PLAYING)) {
	uint64_t current_time = m_parent->get_playing_time();
	if (current_time >= ourtime) {
	  player_debug_message("Candidate for skip decode %llu our %llu", 
			       current_time, ourtime);
	  if (m_byte_stream->can_skip_frame() != 0) {
	    uint64_t diff = 0;
	    do {
	      ourtime = m_byte_stream->start_next_frame();
	      if (ourtime > current_time) diff = ourtime - current_time;
	    } while (!m_byte_stream->eof() && 
		     diff < 500LLU);
	    if (m_byte_stream->eof()) continue;
	    player_debug_message("Skipping ahead to %llu", ourtime);
	  }
	}
      }
#endif
#ifdef DEBUG_DECODE
      player_debug_message("Decoding %c frame " LLX, m_is_video ? 'v' : 'a', ou2rtime);
#endif
#ifdef TIME_DECODE
      clock_t start, end;
      start = clock();
#endif
      ret = codec->decode(ourtime, m_streaming != 0);
#ifdef TIME_DECODE
      end = clock();
      if (ret > 0) {
	diff = end - start;
	if (diff > max) max = diff;
	avg += diff;
	avg_cnt++;
	if ((avg_cnt % 100) == 0) {
	  player_debug_message("%s Decode avg time is " LLD " max " LLD, 
			       m_codec_type,
				 avg / avg_cnt, max);
	  max = 0;
	}
      }
#endif
    }
  }
#ifdef TIME_DECODE
  if (avg_cnt != 0)
    player_debug_message("Decode avg time is " LLD " max " LLD, 
			 avg / avg_cnt,
			 max);
#endif
  delete codec;
  return (0);
}

/* end file player_media_decode.cpp */
