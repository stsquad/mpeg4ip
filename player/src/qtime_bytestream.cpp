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
#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "qtime_bytestream.h"

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CQTByteStreamBase::CQTByteStreamBase (CQtimeFile *parent,
				      CPlayerMedia *m,
				      int track)
  : COurInByteStream(m)
{
  m_track = track;
  m_frame_on = 0;
  m_bookmark = 0;
  m_parent = parent;
  m_eof = 0;
  m_max_frame_size = 16 * 1024;
  m_buffer = (char *) malloc(m_max_frame_size * sizeof(char));
  m_bookmark_buffer = (char *)malloc(m_max_frame_size * sizeof(char));
  m_buffer_on = m_buffer;
  m_bookmark_read_frame = 0;
}

CQTByteStreamBase::~CQTByteStreamBase()
{
  free(m_buffer);
  free(m_bookmark_buffer);
}

int CQTByteStreamBase::eof(void)
{
  return m_eof != 0;
}


void CQTByteStreamBase::check_for_end_of_frame (void)
{
  if (m_byte_on >= m_this_frame_size) {
    m_frame_on++;
    if (m_frame_on >= m_frames_max) {
      if (m_bookmark == 0)
	m_eof = 1;
    } else {
#if 0
      if (m_bookmark == 0)
      player_debug_message("Reading frame %u - bookmark %d", 
			   m_frame_on,
			   m_bookmark);
#endif
      read_frame();
    }
  }
}
char CQTByteStreamBase::get (void)
{
  char ret;
#if 0
  player_debug_message("Getting byte %u frame %u %u bookmark %d", 
		       m_byte_on, m_frame_on, m_this_frame_size, m_bookmark);
#endif
  ret = m_buffer_on[m_byte_on];
  m_byte_on++;
  m_total++;
  check_for_end_of_frame();
  return (ret);
}

char CQTByteStreamBase::peek (void) 
{
  return (m_buffer_on[m_byte_on]);
}

void CQTByteStreamBase::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark = 1;
    m_bookmark_byte_on = m_byte_on;
    m_bookmark_frame_on = m_frame_on;
    m_total_bookmark = m_total;
    m_bookmark_this_frame_size = m_this_frame_size;
  } else {
    m_bookmark = 0;
    m_byte_on = m_bookmark_byte_on;
    m_buffer_on = m_buffer;
    m_frame_on = m_bookmark_frame_on;
    m_total = m_total_bookmark;
    m_this_frame_size = m_bookmark_this_frame_size;
  }
}

size_t CQTByteStreamBase::read (char *buffer, size_t bytestoread)
{
  size_t inbuffer, readbytes = 0;
  do {
    inbuffer = m_this_frame_size - m_byte_on;
    if (inbuffer > bytestoread) {
      inbuffer = bytestoread;
    }
    memcpy(buffer, &m_buffer_on[m_byte_on], inbuffer);
    buffer += inbuffer;
    bytestoread -= inbuffer;
    m_byte_on += inbuffer;
    m_total += inbuffer;
    readbytes += inbuffer;
    check_for_end_of_frame();
  } while (bytestoread > 0 && m_eof == 0);
  return (readbytes);
}
/**************************************************************************
 * Quicktime video stream functions
 **************************************************************************/
void CQTVideoByteStream::video_set_timebase (long frame)
{
  m_eof = 0;
  m_bookmark_read_frame = 0;
  m_frame_on = frame;
  m_bookmark = 0;
  m_parent->lock_file_mutex();
  quicktime_set_video_position(m_parent->get_file(), 
			       frame, 
			       m_track);
  m_parent->unlock_file_mutex();
  read_frame();
}
void CQTVideoByteStream::reset (void) 
{
  video_set_timebase(0);
}

uint64_t CQTVideoByteStream::start_next_frame (void)
{
  uint64_t ret;
#if 0
  if (m_byte_on != 0) {
    player_debug_message("Start next frame "LLU " offset %u %u", 
			 m_total, m_byte_on, m_this_frame_size);
  }
#endif

  ret = m_frame_on;
  ret *= 1000;
  ret /= m_frame_rate;
  return (ret);
}

/*
 * read_frame for video - this will try to read the next frame - it
 * tries to be smart about reading it 1 time if we've already read it
 * while bookmarking
 */
void CQTVideoByteStream::read_frame (void)
{
  size_t next_frame_size;

  if (m_bookmark_read_frame != 0) {
    // Had we already read it ?
    if (m_bookmark == 0) {
      // Yup - looks like it - just adjust the buffers and return
      m_bookmark_read_frame = 0;
      char *temp = m_buffer;
      m_buffer = m_bookmark_buffer;
      m_bookmark_buffer = temp;
      m_buffer_on = m_buffer;
    } else {
      // Bookmarking, and had already read it.
      m_buffer_on = m_bookmark_buffer;
    }
    m_this_frame_size = m_bookmark_read_frame_size;
    m_byte_on = 0;
    return;
  }
  // Haven't already read the next frame,  so - get the size, see if
  // it fits, then read it into the appropriate buffer
  m_parent->lock_file_mutex();
  next_frame_size = quicktime_frame_size(m_parent->get_file(),
					 m_frame_on,
					 m_track);
  if (next_frame_size > m_max_frame_size) {
    m_max_frame_size = next_frame_size;
    m_buffer = (char *)realloc(m_buffer, next_frame_size * sizeof(char));
    m_bookmark_buffer = (char *)realloc(m_bookmark_buffer, 
					next_frame_size * sizeof(char));
  }
  m_this_frame_size = next_frame_size;
  if (m_bookmark == 0) {
    m_buffer_on = m_buffer;
  } else {
    // bookmark...
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_read_frame = 1;
    m_bookmark_read_frame_size = next_frame_size;
  }
  quicktime_read_frame(m_parent->get_file(),
		       (unsigned char *)m_buffer_on,
		       m_track);
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CQTVideoByteStream::set_start_time (uint64_t start)
{
  m_play_start_time = start;

  start *= m_frame_rate;
  start /= 1000;
#if 0
  player_debug_message("qtime video frame " LLD , start);
#endif
  // we've got the position;
  video_set_timebase((size_t)start);
}

/**************************************************************************
 * Quicktime Audio stream functions
 **************************************************************************/
void CQTAudioByteStream::audio_set_timebase (long frame)
{
  m_eof = 0;
  m_bookmark_read_frame = 0;
  m_frame_on = frame;
  m_bookmark = 0;
  m_parent->lock_file_mutex();
  quicktime_set_audio_position(m_parent->get_file(), 
			       frame, 
			       m_track);
  m_parent->unlock_file_mutex();
  read_frame();
}

void CQTAudioByteStream::reset (void) 
{
  audio_set_timebase(0);
}

uint64_t CQTAudioByteStream::start_next_frame (void)
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

void CQTAudioByteStream::read_frame (void)
{
  if (m_bookmark_read_frame != 0) {
    if (m_bookmark == 0) {
      m_bookmark_read_frame = 0;
      char *temp = m_buffer;
      m_buffer = m_bookmark_buffer;
      m_bookmark_buffer = temp;
      m_buffer_on = m_buffer;
    } else {
      m_buffer_on = m_bookmark_buffer;
    }
    m_this_frame_size = m_bookmark_read_frame_size;
    m_byte_on = 0;
    return;
  }
  m_parent->lock_file_mutex();

  if (m_bookmark == 0) {
    m_buffer_on = m_buffer;
  } else {
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_read_frame = 1;
  }
  unsigned char *buff = (unsigned char *)m_buffer_on;
  if (m_add_len_to_stream) {
    buff += 2;
  }
  m_this_frame_size = quicktime_read_audio_frame(m_parent->get_file(),
						 buff,
						 m_max_frame_size,
						 m_track);
  if (m_this_frame_size < 0) {
    m_max_frame_size = -m_this_frame_size;
    m_buffer = (char *)realloc(m_buffer, m_max_frame_size * sizeof(char));
    m_bookmark_buffer = (char *)realloc(m_bookmark_buffer,
					m_max_frame_size * sizeof(char));
    // Okay - I could have used a goto, but it really grates...
    if (m_bookmark == 0) {
      m_buffer_on = m_buffer;
    } else {
      m_buffer_on = m_bookmark_buffer;
      m_bookmark_read_frame = 1;
    }
    buff = (unsigned char *)m_buffer_on;
    if (m_add_len_to_stream) {
      buff += 2;
    }
    m_this_frame_size = quicktime_read_audio_frame(m_parent->get_file(),
						   buff,
						   m_max_frame_size,
						 m_track);
  }
  if (m_add_len_to_stream) {
    m_buffer_on[0] = m_this_frame_size >> 8;
    m_buffer_on[1] = m_this_frame_size & 0xff;
    m_this_frame_size += 2;
  }
#if 0
  player_debug_message("qta frame size %u", m_this_frame_size);
#endif
  if (m_bookmark == 0) {
    // Why not read 2 frames for the price of 1 ?
    m_bookmark_read_frame = 1;
    buff = (unsigned char *)m_bookmark_buffer;
    if (m_add_len_to_stream) buff += 2;
    m_bookmark_read_frame_size = 
      quicktime_read_audio_frame(m_parent->get_file(),
				 buff,
				 m_max_frame_size,
				 m_track);
    if (m_bookmark_read_frame_size <= 0) {
      m_bookmark_read_frame = 0;
    } else if (m_add_len_to_stream) {
      m_bookmark_buffer[0] = m_bookmark_read_frame_size >> 8;
      m_bookmark_buffer[1] = m_bookmark_read_frame_size & 0xff;
      m_bookmark_read_frame_size += 2;
#if 0
      player_debug_message("qta bframe size %u", m_bookmark_read_frame_size);
#endif
    }
  } else {
    m_bookmark_read_frame_size = m_this_frame_size;
  }
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CQTAudioByteStream::set_start_time (uint64_t start)
{
  m_play_start_time = start;
  
  start *= m_frame_rate;
  start /= 1000 * m_samples_per_frame;
#if 0
  player_debug_message("qtime audio frame " LLD, start);
#endif
  // we've got the position;
  audio_set_timebase((size_t)start);
  m_play_start_time = start;
}

/* end file qtime_bytestream.cpp */
