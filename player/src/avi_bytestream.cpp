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
 * avi_bytestream.cpp - convert quicktime file to a bytestream
 */
#include "mpeg4ip.h"
#include "avi_bytestream.h"
#include "player_util.h"

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CAviByteStreamBase::CAviByteStreamBase (CAviFile *parent, const char *which)
  : COurInByteStream(which)
{
  m_frame_on = 0;
  m_frame_in_buffer = 0xffffffff;
  m_parent = parent;
  m_eof = 0;
  m_max_frame_size = 16 * 1024;
  m_buffer = (uint8_t *) malloc(m_max_frame_size * sizeof(char));
}

CAviByteStreamBase::~CAviByteStreamBase()
{
  if (m_buffer) {
    free(m_buffer);
    m_buffer = NULL;
  }
}

int CAviByteStreamBase::eof(void)
{
  return m_eof != 0;
}

/**************************************************************************
 * Quicktime video stream functions
 **************************************************************************/
void CAviVideoByteStream::video_set_timebase (long frame)
{
  m_eof = 0;
  m_frame_on = frame;
  read_frame(frame);
}
void CAviVideoByteStream::reset (void) 
{
  video_set_timebase(0);
}

bool CAviVideoByteStream::start_next_frame (uint8_t **buffer, 
					    uint32_t *buflen,
					    frame_timestamp_t *ts,
					    void **ud)
{
  double ftime;
  
  read_frame(m_frame_on);
  ftime = (double)m_frame_on;
  ftime *= 1000.0;
  ftime /= m_frame_rate;
  ts->msec_timestamp = (uint64_t)ftime;
  ts->timestamp_is_pts = false;
  *buffer = m_buffer;
  *buflen = m_this_frame_size;
  m_frame_on++;
  if (m_frame_on > m_frames_max) m_eof = 1;
  return (true);
}

void CAviVideoByteStream::used_bytes_for_frame (uint32_t bytes)
{
  m_total += bytes;
}

/*
 * read_frame for video - this will try to read the next frame - it
 * tries to be smart about reading it 1 time if we've already read it
 * while bookmarking
 */
void CAviVideoByteStream::read_frame (uint32_t frame_to_read)
{
  uint32_t next_frame_size;

  if (m_frame_in_buffer == frame_to_read) {
    m_byte_on = 0;
    return;
  }

  // Haven't already read the next frame,  so - get the size, see if
  // it fits, then read it into the appropriate buffer
  m_parent->lock_file_mutex();
  
  long temp;
  AVI_set_video_position(m_parent->get_file(), m_frame_on, &temp);
  next_frame_size = temp;

  if (next_frame_size > m_max_frame_size) {
    m_max_frame_size = next_frame_size;
    m_buffer = (uint8_t *)realloc(m_buffer, next_frame_size * sizeof(char) + 4);
  }
  m_this_frame_size = next_frame_size;
  AVI_read_frame(m_parent->get_file(), (char *)m_buffer);
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CAviVideoByteStream::play (uint64_t start)
{
  m_play_start_time = start;

  double time = UINT64_TO_DOUBLE(start);
  time *= m_frame_rate;
  time /= 1000;
#if 0
  player_debug_message("avi video frame " D64 , start);
#endif
  // we've got the position;
  video_set_timebase((uint32_t)time);
}

/**************************************************************************
 * Quicktime Audio stream functions
 **************************************************************************/
void CAviAudioByteStream::audio_set_timebase (long frame)
{
  m_file_pos = 0;
  m_buffer_on = m_this_frame_size = 0;
}

void CAviAudioByteStream::reset (void) 
{
  audio_set_timebase(0);
}

bool CAviAudioByteStream::start_next_frame (uint8_t **buffer, 
					    uint32_t *buflen,
					    frame_timestamp_t *ts,
					    void **ud)
{
  int value;
  if (m_buffer_on < m_this_frame_size) {
    value = m_this_frame_size - m_buffer_on;
    memmove(m_buffer,
	    m_buffer + m_buffer_on,
	    m_this_frame_size - m_buffer_on);
    m_this_frame_size -= m_buffer_on;
  } else {
    value = 0;
    m_this_frame_size = 0;
  }
  m_buffer_on = 0;
  m_parent->lock_file_mutex();
  AVI_set_audio_position(m_parent->get_file(), m_file_pos);

  int ret;
  ret = AVI_read_audio(m_parent->get_file(), 
		       (char *)m_buffer + value, 
		       m_max_frame_size - m_this_frame_size);
  m_parent->unlock_file_mutex();

  //player_debug_message("return from avi read %d", ret);
  m_this_frame_size += ret;
  m_file_pos += ret;
  if (m_file_pos >= AVI_audio_bytes(m_parent->get_file())) {
    m_eof = 1;
  }

  *buffer = m_buffer;
  *buflen = m_this_frame_size;
#if 0
  uint64_t ret;
  ret = m_frame_on;
  ret *= m_samples_per_frame;
  ret *= 1000;
  ret /= m_frame_rate;
    player_debug_message("Start next frame "U64 " offset %u %u", 
			 ret, m_byte_on, m_this_frame_size);
  return (ret);
#endif
  ts->msec_timestamp = 0;
  ts->audio_freq_timestamp = 0;
  ts->audio_freq = AVI_audio_rate(m_parent->get_file());
  ts->timestamp_is_pts = false;
  return true;
}


void CAviAudioByteStream::used_bytes_for_frame (uint32_t bytes)
{
  //player_debug_message("Used %d audio bytes", bytes);
  m_buffer_on += bytes;
  if (m_buffer_on > m_this_frame_size) m_buffer_on = m_this_frame_size;
}

void CAviAudioByteStream::play (uint64_t start)
{
  m_play_start_time = start;
  
#if 0  
  start *= m_frame_rate;
  start /= 1000 * m_samples_per_frame;
  player_debug_message("qtime audio frame " D64, start);
#endif
  // we've got the position;
  audio_set_timebase((uint32_t)start);
  m_play_start_time = start;
}

/* end file qtime_bytestream.cpp */
