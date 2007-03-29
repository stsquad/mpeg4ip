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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * player_media_decode.cpp
 * decode task thread for a CPlayerMedia
 */
#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_sdp.h"
#include "player_util.h"
#include "media_utils.h"
#include "audio.h"
#include "video.h"
#include "rtp_bytestream.h"
#include "codec_plugin_private.h"
#include "our_config_file.h"
//#define DEBUG_DECODE 1
//#define DEBUG_DECODE_MSGS 1
/*
 * start_decoding - API function to call for telling the decoding
 * task it can start
 */
void CPlayerMedia::start_decoding (void)
{
  m_decode_msg_queue.send_message(MSG_START_DECODING, 
				  m_decode_thread_sem);
}

void CPlayerMedia::bytestream_primed (void)
{
  if (m_decode_thread_waiting != 0) {
    m_decode_thread_waiting = 0;
    SDL_SemPost(m_decode_thread_sem);
  }
}

void CPlayerMedia::wait_on_bytestream (void)
{
  m_decode_thread_waiting = 1;
#ifdef DEBUG_DECODE
  if (m_media_info)
    media_message(LOG_INFO, "decode thread %s waiting", m_media_info->media);
  else
    media_message(LOG_INFO, "decode thread waiting");
#endif
  SDL_SemWait(m_decode_thread_sem);
  m_decode_thread_waiting = 0;
} 

/*
 * parse_decode_message - handle messages to the decode task
 */
void CPlayerMedia::parse_decode_message (int &thread_stop, int &decoding)
{
  CMsg *newmsg;

  if ((newmsg = m_decode_msg_queue.get_message()) != NULL) {
#ifdef DEBUG_DECODE_MSGS
    media_message(LOG_DEBUG, "decode thread message %d",newmsg->get_value());
#endif
    switch (newmsg->get_value()) {
    case MSG_STOP_THREAD:
      thread_stop = 1;
      break;
    case MSG_PAUSE_SESSION:
      decoding = 0;
      if (m_sync != NULL) {
	m_sync->flush_decode_buffers();
      }
      if (m_rtp_byte_stream != NULL) {
	m_rtp_byte_stream->flush_rtp_packets();
      }
      break;
    case MSG_START_DECODING:
      if (m_sync != NULL) {
	m_sync->flush_decode_buffers();
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
  int ret = 0;
  int thread_stop = 0, decoding = 0;
  uint32_t decode_skipped_frames = 0;
  frame_timestamp_t ourtime, lasttime;
      // Tell bytestream we're starting the next frame - they'll give us
      // the time.
  uint8_t *frame_buffer;
  uint32_t frame_len;
  void *ud = NULL;
  
  uint32_t frames_decoded;
  uint64_t start_decode_time = 0;
  uint64_t last_decode_time = 0;
  bool have_start_time = false;
  bool have_frame_ts = false;
  bool found_out_of_range_ts = false;
  uint64_t bytes_decoded;

  lasttime.msec_timestamp = 0;
  frames_decoded = 0;
  bytes_decoded = 0;


  while (thread_stop == 0) {
    // waiting here for decoding or thread stop
    ret = SDL_SemWait(m_decode_thread_sem);
#ifdef DEBUG_DECODE
    media_message(LOG_DEBUG, "%s Decode thread awake",
		  get_name());
#endif
    parse_decode_message(thread_stop, decoding);

    if (decoding == 1) {
      // We've been told to start decoding - if we don't have a codec, 
      // create one
      m_sync->set_wait_sem(m_decode_thread_sem);
      if (m_plugin == NULL) {
	switch (m_sync_type) {
	case VIDEO_SYNC:
	  create_video_plugin(NULL, 
			      STREAM_TYPE_RTP,
			      NULL,
			      -1,
			      -1,
			      m_media_fmt,
			      NULL,
			      m_user_data,
			      m_user_data_size);
	  break;
	case AUDIO_SYNC:
	  create_audio_plugin(NULL,
			      STREAM_TYPE_RTP,
			      NULL,
			      -1, 
			      -1, 
			      m_media_fmt,
			      NULL,
			      m_user_data,
			      m_user_data_size);
	  break;
	case TIMED_TEXT_SYNC:
	  create_text_plugin(NULL,
			     STREAM_TYPE_RTP, 
			     NULL, 
			     m_media_fmt, 
			     m_user_data, 
			     m_user_data_size);
	  break;
	}
	if (m_plugin_data == NULL) {
	  m_plugin = NULL;
	} else {
	  media_message(LOG_DEBUG, "Starting %s codec from decode thread",
			m_plugin->c_name);
	}
      }
      if (m_plugin != NULL) {
	m_plugin->c_do_pause(m_plugin_data);
      } else {
	while (thread_stop == 0 && decoding) {
	  SDL_Delay(100);
	  if (m_rtp_byte_stream) {
	    m_rtp_byte_stream->flush_rtp_packets();
	  }
	  parse_decode_message(thread_stop, decoding);
	}
      }
    }
    /*
     * this is our main decode loop
     */
#ifdef DEBUG_DECODE
    media_message(LOG_DEBUG, "%s Into decode loop", get_name());
#endif
    while ((thread_stop == 0) && decoding) {
      parse_decode_message(thread_stop, decoding);
      if (thread_stop != 0)
	continue;
      if (decoding == 0) {
	m_plugin->c_do_pause(m_plugin_data);
	have_frame_ts = false;
	continue;
      }
      if (m_byte_stream->eof()) {
	media_message(LOG_INFO, "%s hit eof", get_name());
	if (m_sync) m_sync->set_eof();
	decoding = 0;
	continue;
      }
      if (m_byte_stream->have_frame() == false) {
	// Indicate that we're waiting, and wait for a message from RTP
	// task.
	wait_on_bytestream();
	continue;
      }

      frame_buffer = NULL;
      bool have_frame;
      memset(&ourtime, 0, sizeof(ourtime));
      have_frame = m_byte_stream->start_next_frame(&frame_buffer, 
						   &frame_len,
						   &ourtime,
						   &ud);
      if (have_frame == false) continue;
      /*
       * If we're decoding video, see if we're playing - if so, check
       * if we've fallen significantly behind the audio
       */
      if (get_sync_type() == VIDEO_SYNC &&
	  (m_parent->get_session_state() == SESSION_PLAYING) &&
	  have_frame_ts) {
	//media_message(LOG_DEBUG, "video decode "U64, ourtime.msec_timestamp);
	int64_t ts_diff = ourtime.msec_timestamp - lasttime.msec_timestamp;

	if (ts_diff > TO_D64(1000) ||
	    ts_diff < TO_D64(-1000)) {
	  // out of range timestamp - we'll want to not skip here
	  uint64_t current_time = m_parent->get_playing_time();
	  ts_diff = ourtime.msec_timestamp - current_time;
	  if (ts_diff > TO_D64(0) && ts_diff < TO_D64(5000)) {
	    // fall through
	  } else {
	  found_out_of_range_ts = true;
	  media_message(LOG_INFO, "found out of range ts "U64" last "U64" "D64,
			ourtime.msec_timestamp, 
			lasttime.msec_timestamp,
			ts_diff);
	  }
	} else {
	  uint64_t current_time = m_parent->get_playing_time();
	  if (found_out_of_range_ts) {
	    ts_diff = current_time - ourtime.msec_timestamp;
	    if (ts_diff > TO_D64(0) && ts_diff < TO_D64(5000)) {
	      found_out_of_range_ts = false;
	      media_message(LOG_INFO, 
			    "ts back in playing range "U64" "D64,
			    ourtime.msec_timestamp, ts_diff);
	    }
	  } else {
	    // regular time
	    if (current_time >= ourtime.msec_timestamp) {
	      media_message(LOG_INFO, 
			    "Candidate for skip decode "U64" our "U64, 
			    current_time, ourtime.msec_timestamp);
	      // If the bytestream can skip ahead, let's do so
	      if (m_byte_stream->can_skip_frame() != 0) {
		int ret;
		int hassync;
		int count;
		current_time += 200; 
		count = 0;
		// Skip up to the current time + 200 msec
		ud = NULL;
		do {
		  if (ud != NULL) free(ud);
		  frame_buffer = NULL;
		  ret = m_byte_stream->skip_next_frame(&ourtime, 
						       &hassync,
						       &frame_buffer, 
						       &frame_len,
						       &ud);
		  decode_skipped_frames++;
		} while (ret != 0 &&
			 !m_byte_stream->eof() && 
			 current_time > ourtime.msec_timestamp);
		if (m_byte_stream->eof() || ret == 0) continue;
		media_message(LOG_INFO, "Skipped ahead "U64 " to "U64, 
			      current_time - 200, ourtime.msec_timestamp);
		/*
		 * Ooh - fun - try to match to the next sync value - if not, 
		 * 15 frames
		 */
		do {
		  if (ud != NULL) free(ud);
		  ret = m_byte_stream->skip_next_frame(&ourtime, 
						       &hassync,
						       &frame_buffer, 
						       &frame_len,
						       &ud);
		  if (hassync < 0) {
		    uint64_t diff = ourtime.msec_timestamp - current_time;
		    if (diff > TO_U64(200)) {
		      hassync = 1;
		    }
		  }
		  decode_skipped_frames++;
		  count++;
		} while (ret != 0 &&
			 hassync <= 0 &&
			 count < 30 &&
			 !m_byte_stream->eof());
		if (m_byte_stream->eof() || ret == 0) continue;
#ifdef DEBUG_DECODE
		media_message(LOG_INFO, 
			      "Matched ahead - count %d, sync %d time "U64,
			      count, hassync, ourtime.msec_timestamp);
#endif
	      }
	    }
	  } // end regular time
	}
      }
      lasttime = ourtime;
      have_frame_ts = true;
#ifdef DEBUG_DECODE
      media_message(LOG_DEBUG, "Decoding %s frame " U64, get_name(),
		    ourtime.msec_timestamp);
#endif
      if (frame_buffer != NULL && frame_len != 0) {
	int sync_frame;
	ret = m_plugin->c_decode_frame(m_plugin_data,
				       &ourtime,
				       m_streaming,
				       &sync_frame,
				       frame_buffer, 
				       frame_len,
				       ud);
#ifdef DEBUG_DECODE
	media_message(LOG_DEBUG, "Decoding %s frame return %d", 
		      get_name(), ret);
#endif
	if (ret > 0) {
	  frames_decoded++;
	  if (have_start_time == false) {
	    have_start_time = true;
	    start_decode_time = ourtime.msec_timestamp;
	  }
	  last_decode_time = ourtime.msec_timestamp;
	  m_byte_stream->used_bytes_for_frame(ret);
	  bytes_decoded += ret;
	} else {
	  m_byte_stream->used_bytes_for_frame(frame_len);
	}

      }
    }
    // calculate frame rate for session
  }
  if (is_audio() == false)
    media_message(LOG_NOTICE, "Video decoder skipped %u frames", 
		  decode_skipped_frames);
  if (last_decode_time > start_decode_time) {
    double fps, bps;
    double secs;
    uint64_t total_time = last_decode_time - start_decode_time;
    secs = UINT64_TO_DOUBLE(total_time);
    secs /= 1000.0;
#if 0
    media_message(LOG_DEBUG, "last time "U64" first "U64, 
		  last_decode_time, start_decode_time);
#endif
    fps = frames_decoded;
    fps /= secs;
    bps = UINT64_TO_DOUBLE(bytes_decoded);
    bps *= 8.0 / secs;
    media_message(LOG_NOTICE, "%s - bytes "U64", seconds %g, fps %g bps %g",
		  get_name(),
		  bytes_decoded, secs, 
		  fps, bps);
  }
  if (m_plugin) {
    m_plugin->c_close(m_plugin_data);
    m_plugin_data = NULL;
  }
  return (0);
}

/* end file player_media_decode.cpp */
