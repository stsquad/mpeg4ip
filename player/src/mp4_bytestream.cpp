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
#include "mp4_bytestream.h"
#include "player_util.h"
//#define DEBUG_MP4_FRAME 1

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CMp4ByteStream::CMp4ByteStream (CMp4File *parent,
				MP4TrackId track,
				const char *type,
				int has_video)
  : COurInByteStream()
{
#ifdef OUTPUT_TO_FILE
  char buffer[80];
  strcpy(buffer, type);
  strcat(buffer, ".raw");
  m_output_file = fopen(buffer, "w");
#endif
  m_track = track;
  m_frame_on = 1;
  m_bookmark = 0;
  m_parent = parent;
  m_eof = 0;
  MP4FileHandle fh = parent->get_file();
  m_frames_max = MP4GetTrackNumberOfSamples(fh, m_track);
  m_max_frame_size = MP4GetTrackMaxSampleSize(fh, m_track);
  m_buffer = (u_int8_t *) malloc(m_max_frame_size * sizeof(u_int8_t));
  m_bookmark_buffer = (u_int8_t *)malloc(m_max_frame_size * sizeof(char));
  m_buffer_on = m_buffer;
  m_type = type;
  m_has_video = has_video;
  m_frame_in_buffer = 0xffffffff;
  m_frame_in_bookmark = 0xffffffff;
  MP4Duration trackDuration;
  trackDuration = MP4GetTrackDuration(fh, m_track);
  uint64_t max_ts;
  max_ts = MP4ConvertFromTrackDuration(fh, 
				       m_track, 
				       trackDuration,
				       MP4_MSECS_TIME_SCALE);
#ifdef _WINDOWS
  m_max_time = (int64_t)(max_ts);
#else
  m_max_time = max_ts;
#endif
  m_max_time /= 1000.0;
  mp4f_message(LOG_DEBUG, 
	       "MP4 %s max time is "LLU" %g", type, max_ts, m_max_time);
  read_frame(1);
}

CMp4ByteStream::~CMp4ByteStream()
{
  free(m_buffer);
  free(m_bookmark_buffer);
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
}

int CMp4ByteStream::eof(void)
{
  return m_eof != 0;
}


void CMp4ByteStream::check_for_end_of_frame (void)
{
  if (m_byte_on >= m_this_frame_size) {
    uint32_t next_frame;
    next_frame = m_frame_in_buffer + 1;
#if 0
    mp4f_message(LOG_DEBUG, "%s - next frame %d %d", 
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
      mp4f_message(LOG_DEBUG, "Reading frame %u - bookmark %d", 
			   next_frame,
			   m_bookmark);
#endif
      read_frame(next_frame);
    }
  }
}
unsigned char CMp4ByteStream::get (void)
{
  unsigned char ret;
#if 0
  player_debug_message("Getting byte %u frame %u %u bookmark %d", 
		       m_byte_on, m_frame_on, m_this_frame_size, m_bookmark);
#endif
  if (m_eof) {
    throw THROW_MP4_END_OF_DATA;
  }
  ret = m_buffer_on[m_byte_on];
  m_byte_on++;
  m_total++;
  check_for_end_of_frame();
  return (ret);
}

unsigned char CMp4ByteStream::peek (void) 
{
  return (m_buffer_on[m_byte_on]);
}

void CMp4ByteStream::bookmark (int bSet)
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

ssize_t CMp4ByteStream::read (unsigned char *buffer, size_t bytestoread)
{
  size_t inbuffer;
  ssize_t readbytes = 0;
  do {
    if (m_eof) {
      throw THROW_MP4_END_OF_DATA;
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

const char *CMp4ByteStream::get_throw_error (int error)
{
  if (error == THROW_MP4_END_OF_DATA)
    return "MP4 - end of data";
  mp4f_message(LOG_INFO, "MP4 - unknown throw error %d", error);
  return "Unknown error";
}

int CMp4ByteStream::throw_error_minor (int error)
{
  return 0;
}

void CMp4ByteStream::set_timebase (MP4SampleId frame)
{
  m_eof = 0;
  m_bookmark_read_frame = 0;
  if (frame == 0) frame = 1;
  m_frame_on = frame;
  m_bookmark = 0;
  read_frame(m_frame_on);
}

void CMp4ByteStream::reset (void) 
{
  set_timebase(1);
}

uint64_t CMp4ByteStream::start_next_frame (void)
{

  if (m_frame_on >= m_frames_max) {
    m_eof = 1;
  }

  read_frame(m_frame_on);

  m_frame_on++;
  return (m_frame_on_ts);
}

int CMp4ByteStream::skip_next_frame (uint64_t *pts, int *pSync)
{
  uint64_t ts;
  ts = start_next_frame();
  *pts = ts;
  *pSync = m_frame_on_has_sync;
  return (1);
}
/*
 * read_frame for video - this will try to read the next frame - it
 * tries to be smart about reading it 1 time if we've already read it
 * while bookmarking
 */
void CMp4ByteStream::read_frame (uint32_t frame_to_read)
{
#ifdef DEBUG_MP4_FRAME 
  mp4f_message(LOG_DEBUG, "Reading frame %d", frame_to_read);
#endif
  if (m_frame_in_buffer == frame_to_read) {
#ifdef DEBUG_MP4_FRAME
    mp4f_message(LOG_DEBUG, 
		 "frame in buffer %u %u", m_byte_on, m_this_frame_size);
#endif
    m_byte_on = 0;
    m_frame_on_ts = m_frame_in_buffer_ts;
    m_frame_on_has_sync = m_frame_in_buffer_has_sync;
    return;
  }
  if (m_bookmark_read_frame != 0 && 
      m_frame_in_bookmark == frame_to_read) {
#ifdef DEBUG_MP4_FRAME
    mp4f_message(LOG_DEBUG, "frame in bookmark");
#endif
    // Had we already read it ?
    if (m_bookmark == 0) {
      // Yup - looks like it - just adjust the buffers and return
      m_bookmark_read_frame = 0;
      u_int8_t *temp = m_buffer;
      m_buffer = m_bookmark_buffer;
      m_bookmark_buffer = temp;
      m_buffer_on = m_buffer;
      m_frame_in_buffer = m_frame_in_bookmark;
      m_frame_in_buffer_ts = m_frame_in_bookmark_ts;
      m_frame_on_ts = m_frame_in_buffer_ts;
      m_frame_on_has_sync = m_frame_in_buffer_has_sync;
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

  if (m_bookmark == 0) {
    m_buffer_on = m_buffer;
    m_frame_in_buffer = frame_to_read;
  } else {
    // bookmark...
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_read_frame = 1;
    m_frame_in_bookmark = frame_to_read;
#ifdef DEBUG_QTIME_VIDEO_FRAME
    mp4f_message(LOG_DEBUG, "Reading into bookmark %u", m_this_frame_size);
#endif
  }
  MP4Timestamp sampleTime;
  MP4Duration sampleDuration, sampleRenderingOffset;
  bool isSyncSample = FALSE;
  bool ret;
  u_int8_t *temp;
  m_this_frame_size = m_max_frame_size;
  temp = m_buffer_on;
  ret = MP4ReadSample(m_parent->get_file(),
		      m_track,
		      frame_to_read,
		      &temp,
		      &m_this_frame_size,
		      &sampleTime,
		      &sampleDuration,
		      &sampleRenderingOffset,
		      &isSyncSample);
  if (ret == FALSE) {
    mp4f_message(LOG_ALERT, "Couldn't read frame from mp4 file - frame %d", 
		 frame_to_read);
    m_eof = 1;
    m_parent->unlock_file_mutex();
    return;
  }
#ifdef OUTPUT_TO_FILE
  fwrite(m_buffer_on, m_this_frame_size, 1, m_output_file);
#endif
#ifdef DEBUG_MP4_FRAME
    mp4f_message(LOG_DEBUG, "reading into buffer %u %u", 
		 frame_to_read, m_this_frame_size);
    mp4f_message(LOG_DEBUG, "Sample time "LLU, sampleTime);
    mp4f_message(LOG_DEBUG, "values %x %x %x %x", m_buffer_on[0], 
		 m_buffer_on[1], m_buffer_on[2], m_buffer_on[3]);
#endif

  uint64_t ts;

  ts = MP4ConvertFromTrackTimestamp(m_parent->get_file(),
				    m_track,
				    sampleTime,
				    MP4_MSECS_TIME_SCALE);
  //if (isSyncSample == TRUE && m_has_video != 0 ) player_debug_message("%s has sync sample %llu", m_type, ts);
#ifdef DEBUG_MP4_FRAME
  mp4f_message(LOG_DEBUG, "Converts to time "LLU, ts);
#endif
  if (m_bookmark == 0) {
    m_frame_in_buffer_ts = ts;
    m_frame_on_ts = ts;
    m_frame_in_buffer_has_sync = m_frame_on_has_sync = isSyncSample;
  } else {
    m_bookmark_read_frame_size = m_this_frame_size;
    m_frame_in_bookmark_ts = ts;
    m_frame_in_bookmark_has_sync = isSyncSample;
  }
		
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CMp4ByteStream::set_start_time (uint64_t start)
{
  m_play_start_time = start;

  MP4Timestamp mp4_ts;
  MP4SampleId mp4_sampleId;

  m_parent->lock_file_mutex();
  mp4_ts = MP4ConvertToTrackTimestamp(m_parent->get_file(),
				      m_track,
				      start,
				      MP4_MSECS_TIME_SCALE);
  mp4_sampleId = MP4GetSampleIdFromTime(m_parent->get_file(),
					m_track,
					mp4_ts, 
					TRUE);
  m_parent->unlock_file_mutex();
#ifdef DEBUG_MP4_FRAME
  mp4f_message(LOG_DEBUG, "Timestamp "LLU" converts to "LLU" sample %d", 
		       start, mp4_ts, mp4_sampleId);
#endif
  set_timebase(mp4_sampleId);
}


double CMp4ByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};

/* end file qtime_bytestream.cpp */
