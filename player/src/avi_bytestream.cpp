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
#include "systems.h"
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
  m_buffer = (unsigned char *) malloc(m_max_frame_size * sizeof(char));
  m_buffer_on = m_buffer;
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

uint64_t CAviVideoByteStream::start_next_frame (unsigned char **buffer, 
						uint32_t *buflen)
{
  uint64_t ret;
  
  read_frame(m_frame_on);
  ret = m_frame_on;
  ret *= 1000;
  ret /= m_frame_rate;
  *buffer = m_buffer_on;
  *buflen = m_this_frame_size;
  m_frame_on++;
  if (m_frame_on > m_frames_max) m_eof = 1;
  return (ret);
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
    m_buffer = (unsigned char *)realloc(m_buffer, next_frame_size * sizeof(char) + 4);
  }
  m_this_frame_size = next_frame_size;
  m_buffer_on = m_buffer;
  AVI_read_frame(m_parent->get_file(), (char *)m_buffer_on);
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CAviVideoByteStream::set_start_time (uint64_t start)
{
  m_play_start_time = start;

  start *= m_frame_rate;
  start /= 1000;
#if 0
  player_debug_message("avi video frame " LLD , start);
#endif
  // we've got the position;
  video_set_timebase((uint32_t)start);
}

/**************************************************************************
 * Quicktime Audio stream functions
 **************************************************************************/
void CAviAudioByteStream::audio_set_timebase (long frame)
{
  m_eof = 0;
  m_frame_on = frame;
  read_frame(frame);
}

void CAviAudioByteStream::reset (void) 
{
  audio_set_timebase(0);
}

uint64_t CAviAudioByteStream::start_next_frame (unsigned char **buffer, 
						uint32_t *buflen)
{
  uint64_t ret;
  ret = m_frame_on;
  ret *= m_samples_per_frame;
  ret *= 1000;
  ret /= m_frame_rate;
#if 0
    player_debug_message("Start next frame "LLU " offset %u %u", 
			 ret, m_byte_on, m_this_frame_size);
#endif
  return (ret);
}


void CAviAudioByteStream::used_bytes_for_frame (uint32_t bytes)
{
  m_total += bytes;
}

void CAviAudioByteStream::read_frame (uint32_t frame_to_read)
{
  m_parent->lock_file_mutex();

  m_buffer_on = m_buffer;
  unsigned char *buff = (unsigned char *)m_buffer_on;
  if (m_add_len_to_stream) {
    buff += 2;
  }
  long temp;
  AVI_set_audio_frame(m_parent->get_file(), 
		      m_frame_on,
		      &temp);
  
  m_this_frame_size = temp;
  if (m_this_frame_size > m_max_frame_size) {
    m_max_frame_size = m_this_frame_size;
    m_buffer = (unsigned char *)realloc(m_buffer, m_max_frame_size * sizeof(char));
    // Okay - I could have used a goto, but it really grates...
    m_buffer_on = m_buffer;
    buff = (unsigned char *)m_buffer_on;
    if (m_add_len_to_stream) {
      buff += 2;
    }
  }
  AVI_read_audio(m_parent->get_file(),
		 (char *)buff,
		 m_this_frame_size);

  if (m_add_len_to_stream) {
    m_buffer_on[0] = (unsigned char)(m_this_frame_size >> 8);
    m_buffer_on[1] = (unsigned char)(m_this_frame_size & 0xff);
    m_this_frame_size += 2;
  }
#if 0
  player_debug_message("qta frame size %u", m_this_frame_size);
#endif
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CAviAudioByteStream::set_start_time (uint64_t start)
{
  m_play_start_time = start;
  
  start *= m_frame_rate;
  start /= 1000 * m_samples_per_frame;
#if 0
  player_debug_message("qtime audio frame " LLD, start);
#endif
  // we've got the position;
  audio_set_timebase((uint32_t)start);
  m_play_start_time = start;
}

/* end file qtime_bytestream.cpp */
