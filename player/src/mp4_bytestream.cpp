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

#include "mpeg4ip.h"
#include "mp4_bytestream.h"
#include "player_util.h"
//#define DEBUG_MP4_FRAME 1
//#define DEBUG_H264_NALS 1

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CMp4ByteStream::CMp4ByteStream (CMp4File *parent,
				MP4TrackId track,
				const char *type,
				bool has_video)
  : COurInByteStream(type)
{
#ifdef ISMACRYP_DEBUG
  my_enc_file = fopen("encbuffer.raw", "w");
  my_unenc_file = fopen("unencbuffer.raw", "w");
  my_unenc_file2 = fopen("unencbuffer2.raw", "w");
#endif
#ifdef OUTPUT_TO_FILE
  char buffer[80];
  strcpy(buffer, type);
  strcat(buffer, ".raw");
  m_output_file = fopen(buffer, "w");
#endif
  m_track = track;
  m_frame_on = 1;
  m_parent = parent;
  m_eof = false;
  MP4FileHandle fh = parent->get_file();
  m_frames_max = MP4GetTrackNumberOfSamples(fh, m_track);
  mp4f_message(LOG_DEBUG, "%s - %u samples", type, m_frames_max);
  m_max_frame_size = MP4GetTrackMaxSampleSize(fh, m_track) + 4; 
  m_sample_freq = MP4GetTrackTimeScale(fh, m_track);
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
  m_max_time = UINT64_TO_DOUBLE(max_ts);
  m_max_time /= 1000.0;
  mp4f_message(LOG_DEBUG, 
	       "MP4 %s max time is "U64" %g", type, max_ts, m_max_time);
  read_frame(1, NULL);
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
#ifdef ISMACRYP_DEBUG
  fclose(my_enc_file);
  fclose(my_unenc_file);
  fclose(my_unenc_file2);
#endif
} 

int CMp4ByteStream::eof(void)
{
  return m_eof;
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
	m_eof = true;
	mp4f_message(LOG_DEBUG, "%s last frame %u %u", 
		     m_name, next_frame, m_frames_max);
    } else {
      read_frame(next_frame, NULL);
    }
  }
}

void CMp4ByteStream::set_timebase (MP4SampleId frame)
{
  m_eof = false;
  if (frame == 0) frame = 1;
  m_frame_on = frame;
  read_frame(m_frame_on, NULL);
}

void CMp4ByteStream::reset (void) 
{
  set_timebase(1);
}

bool CMp4ByteStream::start_next_frame (uint8_t **buffer, 
				       uint32_t *buflen,
				       frame_timestamp_t *pts,
				       void **ud)
{

  if (m_frame_on >= m_frames_max) {
    mp4f_message(LOG_DEBUG, "%s snf end %u %u", m_name, 
		 m_frame_on, m_frames_max);
    m_eof = true;
  }

  read_frame(m_frame_on, pts);

#if 0
  mp4f_message(LOG_DEBUG, "%s - Reading frame %d ts "U64" - len %u %02x %02x %02x %02x", 
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
  return (true);
}

void CMp4ByteStream::used_bytes_for_frame (uint32_t bytes_used)
{
  m_byte_on += bytes_used;
  m_total += bytes_used;
  check_for_end_of_frame();
}

bool CMp4ByteStream::skip_next_frame (frame_timestamp_t *pts, 
				     int *pSync,
				     uint8_t **buffer, 
				     uint32_t *buflen,
				     void **ud)
{
  bool ret;
  ret = start_next_frame(buffer, buflen, pts, ud);
  *pSync = m_frame_on_has_sync;
  return ret;
}

/*
 * read_frame for video - this will try to read the next frame - it
 * tries to be smart about reading it 1 time if we've already read it
 * while bookmarking
 */
void CMp4ByteStream::read_frame (uint32_t frame_to_read,
				 frame_timestamp_t *pts)
{
#ifdef DEBUG_MP4_FRAME 
  mp4f_message(LOG_DEBUG, "%s - Reading frame %d", m_name, frame_to_read);
#endif
  if (m_frame_in_buffer == frame_to_read) {
#ifdef DEBUG_MP4_FRAME
    mp4f_message(LOG_DEBUG, 
		 "%s - frame in buffer %u %u "U64, m_name, 
		 m_byte_on, m_this_frame_size, m_frame_on_ts);
#endif
    m_byte_on = 0;
    m_frame_on_ts = m_frame_in_buffer_ts;
    m_frame_on_has_sync = m_frame_in_buffer_has_sync;
    if (pts != NULL) {
      pts->msec_timestamp = m_frame_on_ts;
      pts->audio_freq_timestamp = m_frame_on_sample_ts;
      pts->audio_freq = m_sample_freq;
      pts->timestamp_is_pts = false;
    }
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
    mp4f_message(LOG_ALERT, "Couldn't read frame from mp4 file - frame %d %d", 
		 frame_to_read, m_track);
    m_eof = true;
    m_parent->unlock_file_mutex();
    return;
  }
  memset(m_buffer + m_this_frame_size, 0, sizeof(uint32_t));
  //*(uint32_t *)(m_buffer + m_this_frame_size) = 0; // add some 0's
#ifdef OUTPUT_TO_FILE
  fwrite(m_buffer, m_this_frame_size, 1, m_output_file);
#endif
  uint64_t ts;

  ts = MP4ConvertFromTrackTimestamp(m_parent->get_file(),
				    m_track,
				    sampleTime,
				    MP4_MSECS_TIME_SCALE);
  //if (isSyncSample == TRUE && m_has_video != 0 ) player_debug_message("%s has sync sample "U64, m_name, ts);
#if 0
  mp4f_message(LOG_DEBUG, "%s frame %u sample time "U64 " converts to time "U64, 
	       m_name, frame_to_read, sampleTime, ts);
#endif
  if (pts != NULL) {
    pts->msec_timestamp = ts;
    pts->audio_freq_timestamp = sampleTime;
    pts->audio_freq = m_sample_freq;
    pts->timestamp_is_pts = false;
  }
  m_frame_on_sample_ts = sampleTime;
  m_frame_in_buffer_ts = ts;
  m_frame_on_ts = ts;
  m_frame_in_buffer_has_sync = m_frame_on_has_sync = isSyncSample;
		
  m_parent->unlock_file_mutex();
  m_byte_on = 0;
}

void CMp4ByteStream::play (uint64_t start)
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
  mp4f_message(LOG_DEBUG, "%s searching timestamp "U64" gives "U64,
	       m_name, start, mp4_ts);
  mp4f_message(LOG_DEBUG, "%s values are sample time "U64" ts "U64,
	       m_name, sampleTime, ts);
#endif
  set_timebase(mp4_sampleId);
}


double CMp4ByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};

bool CMp4EncAudioByteStream::start_next_frame (uint8_t **buffer, 
					       uint32_t *buflen,
					       frame_timestamp_t *pts,
					       void **ud)
{
  bool ret = CMp4AudioByteStream::start_next_frame(buffer, buflen, pts, ud);
  u_int8_t *temp_buffer = NULL;
  u_int32_t temp_this_frame_size = 0;
#ifdef ISMACRYP_DEBUG
  fwrite(*buffer, *buflen, 1, my_enc_file);
#endif
  ismacryp_rc_t ismacryprc;
  ismacryprc = ismacrypDecryptSampleRemoveHeader(m_ismaCryptSId, 
					*buflen,
					*buffer,
					&temp_this_frame_size,
					&temp_buffer);

  if ( ismacryprc != ismacryp_rc_ok ) {
    mp4f_message(LOG_ERR, "%s  1. decrypt error code:  %u" ,
	       m_name, ismacryprc);
    CHECK_AND_FREE(temp_buffer);
    // can't copy anything to buffer in this case.
    return ret; 
  }

#ifdef ISMACRYP_DEBUG
  fwrite(temp_buffer, temp_this_frame_size, 1, my_unenc_file);
#endif
  *buflen = temp_this_frame_size;
  memset(*buffer, 0, *buflen * sizeof(u_int8_t));
  memcpy(*buffer, temp_buffer, temp_this_frame_size);
#ifdef ISMACRYP_DEBUG
  fwrite(*buffer, *buflen, 1, my_unenc_file2);
#endif
  CHECK_AND_FREE(temp_buffer);
  return ret; 
}

bool CMp4EncVideoByteStream::start_next_frame (uint8_t **buffer, 
					       uint32_t *buflen,
					       frame_timestamp_t *ts,
					       void **ud)
{
  bool ret = CMp4VideoByteStream::start_next_frame(buffer, buflen, ts, ud);
  u_int8_t *temp_buffer = NULL;
  u_int32_t temp_this_frame_size = 0;
  ismacryp_rc_t ismacryprc;
  ismacryprc = ismacrypDecryptSampleRemoveHeader(m_ismaCryptSId, 
					*buflen,
					*buffer,
					&temp_this_frame_size,
					&temp_buffer);

  if (ismacryprc != ismacryp_rc_ok ) {
    mp4f_message(LOG_ERR, "%s  2. decrypt error code:  %u" ,
	       m_name, ismacryprc);
    CHECK_AND_FREE(temp_buffer);
    // can't copy anything to buffer in this case.
    return ret; 
  }

  *buflen = temp_this_frame_size;
  memset(*buffer, 0, *buflen * sizeof(u_int8_t));
  memcpy(*buffer, temp_buffer, temp_this_frame_size);
  CHECK_AND_FREE(temp_buffer);
  return ret; 
}

bool CMp4H264VideoByteStream::start_next_frame (uint8_t **buffer, 
						uint32_t *buflen, 
						frame_timestamp_t *ts,
						void **ud)
{
  bool ret;
  uint32_t len = 1, read_offset = 0;
  uint32_t nal_len;
  uint32_t write_offset = 0;
  ret = CMp4VideoByteStream::start_next_frame(buffer, 
					      buflen, 
					      ts, 
					      ud);
  if (*buffer != NULL && *buflen != 0) {
    if (m_buflen_size == 0) {
      m_parent->lock_file_mutex();
      MP4GetTrackH264LengthSize(m_parent->get_file(), m_track, &m_buflen_size);
      m_parent->unlock_file_mutex();
    }
#ifdef DEBUG_H264_NALS
    mp4f_message(LOG_DEBUG, "new frame - len %u", *buflen);
#endif
    do {
      nal_len = read_nal_size(*buffer + read_offset);
#ifdef DEBUG_H264_NALS
      mp4f_message(LOG_DEBUG, "nal offset %u size %u", read_offset, 
		   nal_len);
#endif
      len += nal_len + 3;
      read_offset += nal_len + m_buflen_size;
    } while (read_offset < *buflen);
    if (len > m_translate_buffer_size) {
      m_translate_buffer = (uint8_t *)realloc(m_translate_buffer, len);
      m_translate_buffer_size = len;
#ifdef DEBUG_H264_NALS
      mp4f_message(LOG_DEBUG, "buflen alloced as %u", len);
#endif
    }
    read_offset = 0;
    do {
      nal_len = read_nal_size(*buffer + read_offset);
#ifdef DEBUG_H264_NALS
      mp4f_message(LOG_DEBUG, "write offset %u, read offset %u len %u",
		   write_offset, read_offset, nal_len);
#endif
      m_translate_buffer[write_offset] = 0;
      m_translate_buffer[write_offset + 1] = 0;
      if (write_offset == 0) {
	// make sure that the first nal has a extra 0 (0 0 0 1 header)
	m_translate_buffer[2] = 0;
	write_offset = 1;
      }
      m_translate_buffer[write_offset + 2] = 1;
      memcpy(m_translate_buffer + write_offset + 3, 
	     *buffer + read_offset + m_buflen_size,
	     nal_len);
      write_offset += nal_len + 3;
      read_offset += nal_len + m_buflen_size;
    } while (read_offset < *buflen);
    *buffer = m_translate_buffer;
    *buflen = write_offset;
  }
  return ret;
}

bool CMp4EncH264VideoByteStream::start_next_frame (uint8_t **buffer, 
						uint32_t *buflen, 
						frame_timestamp_t *ts,
						void **ud)
{
  bool ret;
  uint32_t len = 1, read_offset = 0;
  uint32_t nal_len;
  uint32_t write_offset = 0;

  u_int8_t *temp_buffer = NULL;
  u_int32_t temp_this_frame_size = 0;
  ismacryp_rc_t ismacryprc;

  ret = CMp4VideoByteStream::start_next_frame(buffer, 
					      buflen, 
					      ts, 
					      ud);

  //
  // STEP 1 - isma decrypt. this returns ismacryp_rc_ok
  // whether the correct key is loaded or not.
  // nal is tested to determine if frame was decrypted OK 
  //
  ismacryprc = ismacrypDecryptSampleRemoveHeader(m_ismaCryptSId, 
					*buflen,
					*buffer,
					&temp_this_frame_size,
					&temp_buffer);

  if (ismacryprc != ismacryp_rc_ok ) {
    mp4f_message(LOG_ERR, "%s  2. decrypt error code:  %u" ,
	       m_name, ismacryprc);
    CHECK_AND_FREE(temp_buffer);
    // can't copy anything to buffer in this case.
    return ret; 
  }

  *buflen = temp_this_frame_size;
  memset(*buffer, 0, *buflen * sizeof(u_int8_t));
  memcpy(*buffer, temp_buffer, temp_this_frame_size);
  CHECK_AND_FREE(temp_buffer);

  // 
  // 	STEP 2 - H264 
  // 
  //    check the nal_len against frame size to validate
  //    the decryption and reject attempt to decrypt
  //    using wrong key 
  //
  if (*buffer != NULL && *buflen != 0) {
    if (m_buflen_size == 0) {
      m_parent->lock_file_mutex();
      MP4GetTrackH264LengthSize(m_parent->get_file(), m_track, &m_buflen_size);
      m_parent->unlock_file_mutex();
    }
#ifdef DEBUG_H264_NALS
    mp4f_message(LOG_DEBUG, "new frame - len %u", *buflen);
#endif
    do {
      nal_len = read_nal_size(*buffer + read_offset);
#ifdef DEBUG_H264_NALS
      mp4f_message(LOG_DEBUG, "nal offset %u fsize %u size %u", 
        read_offset, *buflen, nal_len);
#endif
      // test if nal is sensible.  this fails when 
      // wrong key is used to decrypt.  this test
      // avoids segfault below when bogus nal_len is used
      // to realloc m_translate_buffer
      if (nal_len != (*buflen - m_buflen_size))
      {
    	mp4f_message(LOG_ERR, "%s  3. buflen %u nal_len %u" ,
	       m_name, *buflen, nal_len);
        return (false);
      }

      len += nal_len + 3;
      read_offset += nal_len + m_buflen_size;
    } while (read_offset < *buflen);
    if (len > m_translate_buffer_size) {
      m_translate_buffer = (uint8_t *)realloc(m_translate_buffer, len);
      m_translate_buffer_size = len;
#ifdef DEBUG_H264_NALS
      mp4f_message(LOG_DEBUG, "buflen alloced as %u", len);
#endif
    }
    read_offset = 0;
    do {
      nal_len = read_nal_size(*buffer + read_offset);
#ifdef DEBUG_H264_NALS
      mp4f_message(LOG_DEBUG, "write offset %u, read offset %u len %u",
		   write_offset, read_offset, nal_len);
#endif
      m_translate_buffer[write_offset] = 0;
      m_translate_buffer[write_offset + 1] = 0;
      if (write_offset == 0) {
	// make sure that the first nal has a extra 0 (0 0 0 1 header)
	m_translate_buffer[2] = 0;
	write_offset = 1;
      }
      m_translate_buffer[write_offset + 2] = 1;
      memcpy(m_translate_buffer + write_offset + 3, 
	     *buffer + read_offset + m_buflen_size,
	     nal_len);
      write_offset += nal_len + 3;
      read_offset += nal_len + m_buflen_size;
    } while (read_offset < *buflen);
    *buffer = m_translate_buffer;
    *buflen = write_offset;
  }
  return ret;
}

bool CMp4TextByteStream::start_next_frame (uint8_t **buffer, 
					   uint32_t *buflen,
					   frame_timestamp_t *ptr,
					   void **ud)
{
  bool ret;

  ret = CMp4ByteStream::start_next_frame(buffer, buflen, ptr, ud);

  if (ret == false) return false;

  if (buflen == 0) return ret;

  *ud = (void *)strdup(m_base_url);

  return true;
}
/* end file mp4_bytestream.cpp */
