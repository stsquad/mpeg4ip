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
  : COurInByteStream(type)
{
#ifdef OUTPUT_TO_FILE
  char buffer[80];
  strcpy(buffer, type);
  strcat(buffer, ".raw");
  m_output_file = fopen(buffer, "w");
#endif
  m_track = track;
  m_frame_on = 1;
  m_parent = parent;
  m_eof = 0;
  MP4FileHandle fh = parent->get_file();
  m_frames_max = MP4GetTrackNumberOfSamples(fh, m_track);
  m_max_frame_size = MP4GetTrackMaxSampleSize(fh, m_track) + 4;
  m_buffer = (u_int8_t *) malloc(m_max_frame_size * sizeof(u_int8_t));
  m_has_video = has_video;
  m_frame_in_buffer = 0xffffffff;
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
  if (m_buffer != NULL) {
    free(m_buffer);
    m_buffer = NULL;
  }
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
    mp4f_message(LOG_DEBUG, "%s - next frame %d", 
		 m_name, 
		 next_frame); 
#endif
    if (next_frame >= m_frames_max + 1) {
	m_eof = 1;
    } else {
      read_frame(next_frame);
    }
  }
}

void CMp4ByteStream::set_timebase (MP4SampleId frame)
{
  m_eof = 0;
  if (frame == 0) frame = 1;
  m_frame_on = frame;
  read_frame(m_frame_on);
}

void CMp4ByteStream::reset (void) 
{
  set_timebase(1);
}

uint64_t CMp4ByteStream::start_next_frame (unsigned char **buffer, 
					   uint32_t *buflen)
{

  if (m_frame_on >= m_frames_max) {
    m_eof = 1;
  }

  read_frame(m_frame_on);

#ifdef DEBUG_MP4_FRAME 
  mp4f_message(LOG_DEBUG, "%s - Reading frame %d ts %llu - len %u %02x %02x %02x %02x", 
	       m_name, m_frame_on, m_frame_on_ts, m_this_frame_size,
	       m_buffer[m_byte_on],
	       m_buffer[m_byte_on + 1],
	       m_buffer[m_byte_on + 2],
	       m_buffer[m_byte_on + 3]);
#endif

  m_frame_on++;
  if (buffer != NULL) {
    *buffer = m_buffer + m_byte_on;
    *buflen = m_this_frame_size;
  }
  return (m_frame_on_ts);
}

void CMp4ByteStream::used_bytes_for_frame (uint32_t bytes_used)
{
  m_byte_on += bytes_used;
  m_total += bytes_used;
  check_for_end_of_frame();
}

int CMp4ByteStream::skip_next_frame (uint64_t *pts, 
				     int *pSync,
				     unsigned char **buffer, 
				     uint32_t *buflen)
{
  uint64_t ts;
  ts = start_next_frame(buffer, buflen);
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
  mp4f_message(LOG_DEBUG, "%s - Reading frame %d", m_name, frame_to_read);
#endif
  if (m_frame_in_buffer == frame_to_read) {
#ifdef DEBUG_MP4_FRAME
    mp4f_message(LOG_DEBUG, 
		 "%s - frame in buffer %u %u", m_name, 
		 m_byte_on, m_this_frame_size);
#endif
    m_byte_on = 0;
    m_frame_on_ts = m_frame_in_buffer_ts;
    m_frame_on_has_sync = m_frame_in_buffer_has_sync;
    return;
  }
  // Haven't already read the next frame,  so - get the size, see if
  // it fits, then read it into the appropriate buffer
  m_parent->lock_file_mutex();

  m_frame_in_buffer = frame_to_read;

  MP4Timestamp sampleTime;
  MP4Duration sampleDuration, sampleRenderingOffset;
  bool isSyncSample = FALSE;
  bool ret;
  u_int8_t *temp;
  m_this_frame_size = m_max_frame_size;
  temp = m_buffer;
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
  memset(m_buffer + m_this_frame_size, 0, sizeof(uint32_t));
  //*(uint32_t *)(m_buffer + m_this_frame_size) = 0; // add some 0's
#ifdef OUTPUT_TO_FILE
  fwrite(m_buffer, m_this_frame_size, 1, m_output_file);
#endif
#if 0
  //    mp4f_message(LOG_DEBUG, "reading into buffer %u %u", 
  //		 frame_to_read, m_this_frame_size);
    //mp4f_message(LOG_DEBUG, "%s Sample time "LLU, m_name, sampleTime);
    //    mp4f_message(LOG_DEBUG, "%s values %x %x %x %x", m_name, m_buffer[0], 
    //	 m_buffer[1], m_buffer[2], m_buffer[3]);
#endif

  uint64_t ts;

  ts = MP4ConvertFromTrackTimestamp(m_parent->get_file(),
				    m_track,
				    sampleTime,
				    MP4_MSECS_TIME_SCALE);
  //if (isSyncSample == TRUE && m_has_video != 0 ) player_debug_message("%s has sync sample %llu", m_name, ts);
#if 0
  mp4f_message(LOG_DEBUG, "%s frame %u sample time "LLU " converts to time "LLU, 
	       m_name, frame_to_read, sampleTime, ts);
#endif
  m_frame_in_buffer_ts = ts;
  m_frame_on_ts = ts;
  m_frame_in_buffer_has_sync = m_frame_on_has_sync = isSyncSample;
		
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
  uint64_t ts;
  MP4Timestamp sampleTime;

  sampleTime = MP4GetSampleTime(m_parent->get_file(),
				m_track,
				mp4_sampleId);
  ts = MP4ConvertFromTrackTimestamp(m_parent->get_file(),
				    m_track,
				    sampleTime,
				    MP4_MSECS_TIME_SCALE);
  m_parent->unlock_file_mutex();
#ifdef DEBUG_MP4_FRAME
  mp4f_message(LOG_DEBUG, "%s searching timestamp "LLU" gives "LLU,
	       m_name, start, mp4_ts);
  mp4f_message(LOG_DEBUG, "%s values are sample time "LLU" ts "LLU,
	       m_name, sampleTime, ts);
#endif
  set_timebase(mp4_sampleId);
}


double CMp4ByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};

/* end file qtime_bytestream.cpp */
