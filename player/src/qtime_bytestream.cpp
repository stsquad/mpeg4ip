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

#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "qtime_bytestream.h"

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CQTByteStreamBase::CQTByteStreamBase (CQtimeFile *parent,
				      int track,
				      const char *type)
  : COurInByteStream()
{
  m_track = track;
  m_frame_on = 0;
  m_bookmark = 0;
  m_parent = parent;
  m_eof = 0;
  m_max_frame_size = 16 * 1024;
  m_buffer = (unsigned char *) malloc(m_max_frame_size * sizeof(char));
  m_bookmark_buffer = (unsigned char *)malloc(m_max_frame_size * sizeof(char));
  m_buffer_on = m_buffer;
  m_type = type;
  m_frame_in_buffer = 0xffffffff;
  m_frame_in_bookmark = 0xffffffff;
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
    uint32_t next_frame;
    next_frame = m_frame_in_buffer + 1;
#if 0
    player_debug_message("%s - next frame %d %d", 
			 m_type, 
			 next_frame, 
			 m_bookmark);
#endif
    if (next_frame >= m_frames_max) {
      if (m_bookmark == 0)
	m_eof = 1;
    } else {
#if 0
      if (m_bookmark == 0)
      player_debug_message("Reading frame %u - bookmark %d", 
			   next_frame,
			   m_bookmark);
#endif
      read_frame(next_frame);
    }
  }
}
unsigned char CQTByteStreamBase::get (void)
{
  unsigned char ret;
#if 0
  player_debug_message("Getting byte %u frame %u %u bookmark %d", 
		       m_byte_on, m_frame_on, m_this_frame_size, m_bookmark);
#endif
  if (m_eof) {
    throw("END OF DATA");
  }
  ret = m_buffer_on[m_byte_on];
  m_byte_on++;
  m_total++;
  check_for_end_of_frame();
  return (ret);
}

unsigned char CQTByteStreamBase::peek (void) 
{
  return (m_buffer_on[m_byte_on]);
}

void CQTByteStreamBase::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark = 1;
    m_bookmark_byte_on = m_byte_on;
    m_total_bookmark = m_total;
    m_bookmark_this_frame_size = m_this_frame_size;
  } else {
    m_bookmark = 0;
    m_byte_on = m_bookmark_byte_on;
    m_buffer_on = m_buffer;
    m_total = m_total_bookmark;
    m_this_frame_size = m_bookmark_this_frame_size;
  }
}

ssize_t CQTByteStreamBase::read (unsigned char *buffer, size_t bytestoread)
{
  size_t inbuffer;
  ssize_t readbytes = 0;
  do {
    if (m_eof) {
      throw("END OF DATA");
    }
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
  read_frame(m_frame_on);
}
void CQTVideoByteStream::reset (void) 
{
  video_set_timebase(0);
}

uint64_t CQTVideoByteStream::start_next_frame (void)
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
	if ((m_buffer_on[ix] == 0) &&
	    (m_buffer_on[ix + 1] == 0) &&
	    (m_buffer_on[ix + 2] == 1)) {
	  player_debug_message("correct start code %x", m_buffer_on[ix + 3]);
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
   //player_debug_message("Returning %llu", ret);
 } else {
   ret = m_frame_on;
   ret *= 1000;
   ret /= m_frame_rate;
 }
  read_frame(m_frame_on);

  m_frame_on++;
  return (ret);
}

/*
 * read_frame for video - this will try to read the next frame - it
 * tries to be smart about reading it 1 time if we've already read it
 * while bookmarking
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
  if (m_bookmark_read_frame != 0 && 
      m_frame_in_bookmark == frame_to_read) {
#ifdef DEBUG_QTIME_VIDEO_FRAME
    player_debug_message("frame in bookmark");
#endif
    // Had we already read it ?
    if (m_bookmark == 0) {
      // Yup - looks like it - just adjust the buffers and return
      m_bookmark_read_frame = 0;
      unsigned char *temp = m_buffer;
      m_buffer = m_bookmark_buffer;
      m_bookmark_buffer = temp;
      m_buffer_on = m_buffer;
      m_frame_in_buffer = m_frame_in_bookmark;
      m_frame_in_bookmark = 0xfffffff;
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
					 frame_to_read,
					 m_track);
  if (next_frame_size > m_max_frame_size) {
    m_max_frame_size = next_frame_size;
    m_buffer = (unsigned char *)realloc(m_buffer, 
					next_frame_size * sizeof(char));
    m_bookmark_buffer = (unsigned char *)realloc(m_bookmark_buffer, 
						 next_frame_size * sizeof(char));
  }
  m_this_frame_size = next_frame_size;
  quicktime_set_video_position(m_parent->get_file(), frame_to_read, m_track);
  if (m_bookmark == 0) {
    m_buffer_on = m_buffer;
    m_frame_in_buffer = frame_to_read;
#ifdef DEBUG_QTIME_VIDEO_FRAME
    player_debug_message("reading into buffer %u", m_this_frame_size);
#endif
  } else {
    // bookmark...
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_read_frame = 1;
    m_bookmark_read_frame_size = next_frame_size;
    m_frame_in_bookmark = frame_to_read;
#ifdef DEBUG_QTIME_VIDEO_FRAME
    player_debug_message("Reading into bookmark %u", m_this_frame_size);
#endif
  }
  quicktime_read_frame(m_parent->get_file(),
		       m_buffer_on,
		       m_track);
#ifdef DEBUG_QTIME_VIDEO_FRAME
  player_debug_message("Buffer %d %02x %02x %02x %02x", 
		       frame_to_read,
	 m_buffer_on[0],
	 m_buffer_on[1],
	 m_buffer_on[2],
	 m_buffer_on[3]);
#endif
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CQTVideoByteStream::set_start_time (uint64_t start)
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
      //player_debug_message("frame %d %llu", ix, cmp);
      if (cmp >= start) {
	player_debug_message("Searched through - frame %d is %llu", 
			     ix, start);
	break;
      }
    } else {
      m_parent->unlock_file_mutex();
      ix = start * m_frame_rate;
      ix /= 1000;
      video_set_timebase(ix);
      return;
    }
  }
#if 0
  player_debug_message("qtime video frame " LLD , start);
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
  m_bookmark_read_frame = 0;
  m_frame_on = frame;
  m_bookmark = 0;
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

uint64_t CQTAudioByteStream::start_next_frame (void)
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
  ret *= 1000;
  ret /= m_frame_rate;
#ifdef DEBUG_QTIME_AUDIO_FRAME
  player_debug_message("audio - start frame %d %d", m_frame_on, m_frames_max);
#endif
#if 0
  player_debug_message("audio Start next frame "LLU " offset %u %u", 
		       ret, m_byte_on, m_this_frame_size);
#endif
  read_frame(m_frame_on);
  m_frame_on++;
  return (ret);
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
  if ((m_bookmark_read_frame != 0) &&
      (m_frame_in_bookmark == frame_to_read)) {
    if (m_bookmark == 0) {
      m_bookmark_read_frame = 0;
      unsigned char *temp = m_buffer;
      m_buffer = m_bookmark_buffer;
      m_bookmark_buffer = temp;
      m_buffer_on = m_buffer;
      m_frame_in_buffer = m_frame_in_bookmark;
      m_frame_in_bookmark = 0xffffffff;
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
    m_frame_in_buffer = frame_to_read;
  } else {
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_read_frame = 1;
  }
  unsigned char *buff = m_buffer_on;
  quicktime_set_audio_position(m_parent->get_file(), 
			       frame_to_read, 
			       m_track);
  m_this_frame_size = quicktime_read_audio_frame(m_parent->get_file(),
						 buff,
						 m_max_frame_size,
						 m_track);
  if (m_this_frame_size < 0) {
    m_max_frame_size = -m_this_frame_size;
    m_buffer = (unsigned char *)realloc(m_buffer, m_max_frame_size * sizeof(char));
    m_bookmark_buffer = (unsigned char *)realloc(m_bookmark_buffer,
						 m_max_frame_size * sizeof(char));
    // Okay - I could have used a goto, but it really grates...
    if (m_bookmark == 0) {
      m_buffer_on = m_buffer;
      m_frame_in_buffer = frame_to_read;
    } else {
      m_buffer_on = m_bookmark_buffer;
      m_bookmark_read_frame = 1;
      m_frame_in_bookmark = frame_to_read;
    }
    buff = m_buffer_on;
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
  if (m_bookmark == 0) {
    // Why not read 2 frames for the price of 1 ?
    m_bookmark_read_frame = 1;
    buff = m_bookmark_buffer;
    m_bookmark_read_frame_size = 
      quicktime_read_audio_frame(m_parent->get_file(),
				 buff,
				 m_max_frame_size,
				 m_track);
    if (m_bookmark_read_frame_size <= 0) {
      m_bookmark_read_frame = 0;
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
  audio_set_timebase((uint32_t)start);
  m_play_start_time = start;
}

/* end file qtime_bytestream.cpp */
