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
 * qtime_bytestream.cpp - convert quicktime file to a bytestream
 */
//#define DEBUG_QTIME_AUDIO_FRAME 1
//#define DEBUG_QTIME_VIDEO_FRAME 1
#include "mpeg4ip.h"
#include "qtime_bytestream.h"
#include "player_util.h"

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CQTByteStreamBase::CQTByteStreamBase (CQtimeFile *parent,
				      int track,
				      const char *name)
  : COurInByteStream(name)
{
  m_track = track;
  m_frame_on = 0;
  m_parent = parent;
  m_eof = 0;
  m_max_frame_size = 16 * 1024;
  m_buffer = (uint8_t *) malloc(m_max_frame_size * sizeof(char));
  m_frame_in_buffer = 0xffffffff;
}

CQTByteStreamBase::~CQTByteStreamBase()
{
  if (m_buffer != NULL) {
    free(m_buffer);
    m_buffer = NULL;
  }
}

int CQTByteStreamBase::eof(void)
{
  return m_eof != 0;
}


void CQTByteStreamBase::check_for_end_of_frame (void)
{
  if (m_byte_on >= m_this_frame_size) {
    uint32_t next_frame;
    next_frame = m_frame_in_buffer + 1;
#if 0
    player_debug_message("%s - next frame %d", 
			 m_name, 
			 next_frame);
#endif
    if (next_frame >= m_frames_max) {
      m_eof = 1;
    } else {
      read_frame(next_frame);
    }
  }
}

void CQTByteStreamBase::used_bytes_for_frame (uint32_t bytes_used)
{
  m_byte_on += bytes_used;
  m_total += bytes_used;
  check_for_end_of_frame();
}

/**************************************************************************
 * Quicktime video stream functions
 **************************************************************************/
void CQTVideoByteStream::video_set_timebase (long frame)
{
  m_eof = 0;
  m_frame_on = frame;
  m_parent->lock_file_mutex();
  quicktime_set_video_position(m_parent->get_file(), 
			       frame, 
			       m_track);
  m_parent->unlock_file_mutex();
  read_frame(m_frame_on);
}
void CQTVideoByteStream::reset (void) 
{
  video_set_timebase(0);
}

bool CQTVideoByteStream::start_next_frame (uint8_t **buffer, 
					   uint32_t *buflen,
					   frame_timestamp_t *ts,
					   void **ud)
{
  uint64_t ret;
  long start;
  int duration;

#if 0
  if (m_frame_on != 0) {
    player_debug_message("frame %d on %u %d", m_frame_on,
			 m_byte_on, m_this_frame_size);
    if (m_byte_on != 0 && m_byte_on != m_this_frame_size) {
      for (uint32_t ix = m_byte_on; ix < m_this_frame_size - 4; ix++) {
	if ((m_buffer[ix] == 0) &&
	    (m_buffer[ix + 1] == 0) &&
	    (m_buffer[ix + 2] == 1)) {
	  player_debug_message("correct start code %x", m_buffer[ix + 3]);
	  player_debug_message("offset %d %d %d", m_byte_on,
			       ix, m_this_frame_size);
	}
      }
    }
  }
#endif
  if (m_frame_on >= m_frames_max) {
    m_eof = 1;
  }
#ifdef DEBUG_QTIME_VIDEO_FRAME
  player_debug_message("start_next_frame %d", m_frame_on);
#endif
  if (quicktime_video_frame_time(m_parent->get_file(),
				 m_track,
				 m_frame_on,
				 &start,
				 &duration) != 0) {
    ret = start;
    ret *= 1000;
    ret /= m_time_scale;
   //player_debug_message("Returning "U64, ret);
 } else {
   ret = m_frame_on;
   ret *= 1000;
   ret /= m_frame_rate;
 }
  read_frame(m_frame_on);
  *buffer = m_buffer;
  *buflen = m_this_frame_size;

  m_frame_on++;
  ts->msec_timestamp = ret;
  ts->timestamp_is_pts = false;
  return (true);
}

bool CQTVideoByteStream::skip_next_frame (frame_timestamp_t *ptr, 
					 int *hasSync,
					 uint8_t **buffer, 
					 uint32_t *buflen,
					 void **ud)
{
  *hasSync = 0;
  return start_next_frame(buffer, buflen, ptr, ud);
}
/*
 * read_frame for video - this will try to read the next frame 
 */
void CQTVideoByteStream::read_frame (uint32_t frame_to_read)
{
  uint32_t next_frame_size;

  if (m_frame_in_buffer == frame_to_read) {
#ifdef DEBUG_QTIME_VIDEO_FRAME
    player_debug_message("frame in buffer %u %u", m_byte_on, m_this_frame_size);
#endif
    m_byte_on = 0;
    return;
  }

  // Haven't already read the next frame,  so - get the size, see if
  // it fits, then read it into the appropriate buffer
  m_parent->lock_file_mutex();
  next_frame_size = quicktime_frame_size(m_parent->get_file(),
					 frame_to_read,
					 m_track);
  if (next_frame_size > m_max_frame_size) {
    m_max_frame_size = next_frame_size + 4;
    m_buffer = (uint8_t *)realloc(m_buffer, 
					(next_frame_size + 4) * sizeof(char));
  }
  m_this_frame_size = next_frame_size;
  quicktime_set_video_position(m_parent->get_file(), frame_to_read, m_track);
  m_frame_in_buffer = frame_to_read;
#ifdef DEBUG_QTIME_VIDEO_FRAME
  player_debug_message("reading into buffer %u", m_this_frame_size);
#endif
  quicktime_read_frame(m_parent->get_file(),
		       (unsigned char *)m_buffer,
		       m_track);
#ifdef DEBUG_QTIME_VIDEO_FRAME
  player_debug_message("Buffer %d %02x %02x %02x %02x", 
		       frame_to_read,
	 m_buffer[0],
	 m_buffer[1],
	 m_buffer[2],
	 m_buffer[3]);
#endif
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CQTVideoByteStream::play (uint64_t start)
{
  m_play_start_time = start;
  uint32_t ix;
  long frame_start;
  int duration;


  m_parent->lock_file_mutex();
  for (ix = 0; ix < m_frames_max; ix++) {
    if (quicktime_video_frame_time(m_parent->get_file(),
				   m_track, 
				   ix, 
				   &frame_start, 
				   &duration) != 0) {
      uint64_t cmp;
      cmp = frame_start + duration;
      cmp *= 1000;
      cmp /= m_time_scale;
      //player_debug_message("frame %d "U64, ix, cmp);
      if (cmp >= start) {
	player_debug_message("Searched through - frame %d is "U64, 
			     ix, start);
	break;
      }
    } else {
      m_parent->unlock_file_mutex();
      ix = (uint32_t)((start * m_frame_rate) / 1000);
      video_set_timebase(ix);
      return;
    }
  }
#if 0
  player_debug_message("qtime video frame " D64 , start);
#endif
  // we've got the position;
  m_parent->unlock_file_mutex();
  video_set_timebase(ix);
}

void CQTVideoByteStream::config (long num_frames, float frate, int time_scale)
{
  m_frames_max = num_frames;
  m_frame_rate = (uint32_t)frate;
  m_time_scale = time_scale;

  long start;
  int duration;
  // Set up max play time, based on the timing of the last frame.
  if (quicktime_video_frame_time(m_parent->get_file(),
				 m_track,
				 m_frames_max - 1,
				 &start,
				 &duration) != 0) {
    player_debug_message("video frame time - %d %ld %d", 
			 m_frames_max, start, duration);
    m_max_time = (start + duration);
    m_max_time /= m_time_scale;
  } else {
    m_max_time = m_frames_max;
    m_max_time /= m_frame_rate;
  }
  player_debug_message("Max time is %g", m_max_time);
}

double CQTVideoByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};

/**************************************************************************
 * Quicktime Audio stream functions
 **************************************************************************/
void CQTAudioByteStream::audio_set_timebase (long frame)
{
#ifdef DEBUG_QTIME_AUDIO_FRAME
  player_debug_message("Setting qtime audio timebase to frame %ld", frame);
#endif
  m_eof = 0;
  m_frame_on = frame;
  m_parent->lock_file_mutex();
  quicktime_set_audio_position(m_parent->get_file(), 
			       frame, 
			       m_track);
  m_parent->unlock_file_mutex();
  read_frame(0);
}

void CQTAudioByteStream::reset (void) 
{
  audio_set_timebase(0);
}

bool CQTAudioByteStream::start_next_frame (uint8_t **buffer, 
					   uint32_t *buflen,
					   frame_timestamp_t *pts,
					   void **ud)
{
  uint64_t ret;
  if (m_frame_on >= m_frames_max) {
#ifdef DEBUG_QTIME_AUDIO_FRAME
    player_debug_message("Setting EOF from start_next_frame %d %d", 
			 m_frame_on, m_frames_max);
#endif
    m_eof = 1;
  }
  ret = m_frame_on;
  ret *= m_samples_per_frame;
  pts->audio_freq_timestamp = ret;
  pts->audio_freq = m_frame_rate;
  ret *= 1000;
  ret /= m_frame_rate;
#ifdef DEBUG_QTIME_AUDIO_FRAME
  player_debug_message("audio - start frame %d %d", m_frame_on, m_frames_max);
#endif
#if 0
  player_debug_message("audio Start next frame "U64 " offset %u %u", 
		       ret, m_byte_on, m_this_frame_size);
#endif
  read_frame(m_frame_on);
  *buffer = m_buffer;
  *buflen = m_this_frame_size;
  m_frame_on++;
  pts->msec_timestamp = ret;
  pts->timestamp_is_pts = false;
  return (true);
}

void CQTAudioByteStream::read_frame (uint32_t frame_to_read)
{
#ifdef DEBUG_QTIME_AUDIO_FRAME
  player_debug_message("audio read_frame %d", frame_to_read);
#endif

  if (m_frame_in_buffer == frame_to_read) {
    m_byte_on = 0;
    return;
  }
  m_parent->lock_file_mutex();

  m_frame_in_buffer = frame_to_read;

  unsigned char *buff = (unsigned char *)m_buffer;
  quicktime_set_audio_position(m_parent->get_file(), 
			       frame_to_read, 
			       m_track);
  m_this_frame_size = quicktime_read_audio_frame(m_parent->get_file(),
						 buff,
						 m_max_frame_size,
						 m_track);
  if (m_this_frame_size < 0) {
    m_max_frame_size = -m_this_frame_size;
    m_buffer = (uint8_t *)realloc(m_buffer, m_max_frame_size * sizeof(char));
    // Okay - I could have used a goto, but it really grates...
    m_frame_in_buffer = frame_to_read;
    buff = (unsigned char *)m_buffer;
    quicktime_set_audio_position(m_parent->get_file(), 
				 frame_to_read, 
				 m_track);
    m_this_frame_size = quicktime_read_audio_frame(m_parent->get_file(),
						   buff,
						   m_max_frame_size,
						 m_track);
  }
#if 0
  player_debug_message("qta frame size %u", m_this_frame_size);
#endif
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CQTAudioByteStream::play (uint64_t start)
{
  m_play_start_time = start;
  
  start *= m_frame_rate;
  start /= 1000 * m_samples_per_frame;
#if 0
  player_debug_message("qtime audio frame " D64, start);
#endif
  // we've got the position;
  audio_set_timebase((uint32_t)start);
  m_play_start_time = start;
}

/* end file qtime_bytestream.cpp */
