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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * qtime_bytestream.cpp - convert quicktime file to a bytestream
 */

#include "mpeg4ip.h"
#include "mpeg3_bytestream.h"
#include "player_util.h"
#include "mp4av/mp4av.h"
#include <math.h>
//#define DEBUG_MP4_FRAME 1

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CMpeg3VideoByteStream::CMpeg3VideoByteStream (mpeg3_t *file, int stream)
  : COurInByteStream("mpeg3 video")
{
#ifdef OUTPUT_TO_FILE
  m_output_file = fopen("raw.mp3v", "w");
#endif
  m_file = file;
  m_stream = stream;

  m_eof = 0;
  m_frames_max = mpeg3_video_frames(m_file, m_stream);
  m_frame_rate = mpeg3_frame_rate(m_file, m_stream);

  m_max_time = m_frames_max;
  m_max_time /= m_frame_rate;
  mpeg3f_message(LOG_DEBUG, 
		 "Mpeg3 video frame rate %g frames %ld max time is %g", 
		 m_frame_rate, m_frames_max, m_max_time);
  m_changed_time = 0;
}

CMpeg3VideoByteStream::~CMpeg3VideoByteStream()
{
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
  mpeg3_close(m_file);
}

int CMpeg3VideoByteStream::eof(void)
{
  return m_eof != 0;
}



#include "mpeg3protos.h"
void CMpeg3VideoByteStream::set_timebase (double time)
{
  double calc;
  m_eof = 0;
  m_frame_on = 0;
  //mpeg3_seek_video_percentage(m_file, m_stream, time / m_max_time);
  
  calc = (time * m_frame_rate);
  calc = ceil(calc);
  m_frame_on = (long)(calc);
  mpeg3_set_frame(m_file, m_frame_on, m_stream);
  mpeg3f_message(LOG_DEBUG, "video seek frame is %ld %g %g", m_frame_on,
		 time, m_max_time);
}

void CMpeg3VideoByteStream::reset (void) 
{
  set_timebase(0.0);
  m_this_frame_size = 0;
}

uint64_t CMpeg3VideoByteStream::start_next_frame (uint8_t **buffer, 
						  uint32_t *buflen,
						  void **ud)
{
  double ts;
  uint64_t time;
  if (m_eof) {
    return 0;
  }
  uint8_t *buf;
  long blen;

  int ret = mpeg3_read_video_chunk_resize(m_file,
					  (unsigned char **)&buf,
					  &blen,
					  m_stream);

  if (ret != 0) {
    m_eof = 1;
    return 0;
  }
  if (buffer != NULL) {
    *buffer = buf;
    *buflen = blen;
#if 0
    mpeg3f_message(LOG_DEBUG, "frame %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		   m_frame_on,
		   buf[0],
		   buf[1],
		   buf[2],
		   buf[3],
		   buf[4],
		   buf[5],
		   buf[6],
		   buf[7],
		   buf[8]);
#endif
  }

  ts = m_frame_on;
  ts /= m_frame_rate;
  ts *= 1000.0;
  time = (uint64_t)ts;
  //mpeg3f_message(LOG_DEBUG, "start next frame %ld %llu", blen, time);
  m_frame_on++;
  return time;
}

void CMpeg3VideoByteStream::used_bytes_for_frame (uint32_t bytes_used)
{
  mpeg3_read_video_chunk_cleanup(m_file, m_stream);
}

int CMpeg3VideoByteStream::skip_next_frame (uint64_t *pts, 
					    int *pSync,
					    uint8_t **buffer, 
					    uint32_t *buflen,
					    void **ud)
{
  uint64_t ts;
  mpeg3_read_video_chunk_cleanup(m_file, m_stream);
  ts = start_next_frame(buffer, buflen, NULL);
  *pts = ts;
  if (*buffer != NULL)
    *pSync = MP4AV_Mpeg3FindGopOrPictHdr(*buffer, *buflen, NULL);
  else 
    *pSync = 0;
  //*pSync = m_frame_on_has_sync;
  return (1);
}

void CMpeg3VideoByteStream::play (uint64_t start)
{
  m_play_start_time = start;

  double ts;
  ts = UINT64_TO_DOUBLE(start);
  ts /= 1000.0;

  set_timebase(ts);
}


double CMpeg3VideoByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};



CMpeg3AudioByteStream::CMpeg3AudioByteStream (mpeg3_t *file, int stream)
  : COurInByteStream("mpeg3 audio")
{
#ifdef OUTPUT_TO_FILE
  m_output_file = fopen("raw.mp3a", "w");
#endif
  m_file = file;
  m_stream = stream;
  m_buffer = NULL;
  m_buffersize_max = 0;
  m_this_frame_size = 0;
  m_eof = 0;
  m_samples_per_frame = mpeg3_audio_samples_per_frame(m_file, m_stream);
  m_freq = mpeg3_sample_rate(m_file, m_stream);
  m_max_time = mpeg3_audio_get_number_of_frames(m_file, m_stream);
  m_max_time *= mpeg3_audio_samples_per_frame(m_file, m_stream);
  m_max_time /= mpeg3_sample_rate(m_file, m_stream);
  m_chans = mpeg3_audio_channels(m_file, m_stream);
  mpeg3f_message(LOG_DEBUG, "audio max time is %g", m_max_time);
  m_changed_time = 0;
  m_frame_on = 0;
}

CMpeg3AudioByteStream::~CMpeg3AudioByteStream()
{
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
  CHECK_AND_FREE(m_buffer);
  mpeg3_close(m_file);
}

int CMpeg3AudioByteStream::eof(void)
{
  return m_eof != 0;
}



void CMpeg3AudioByteStream::set_timebase (double time)
{
  double calc;
  m_eof = 0;
  m_frame_on = 0;
  mpeg3_seek_audio_percentage(m_file, m_stream, time / m_max_time);
  
  calc = (time * m_freq) / m_samples_per_frame;
  if (calc > 0) {
    calc = ceil(calc);
  } else {
    calc = 0;
  }
  m_frame_on = (long)(calc);
  mpeg3f_message(LOG_DEBUG, "audio seek frame is %ld %g %g", m_frame_on,
		 time, m_max_time);
}

void CMpeg3AudioByteStream::reset (void) 
{
  set_timebase(0.0);
  m_this_frame_size = 0;
}

uint64_t CMpeg3AudioByteStream::start_next_frame (uint8_t **buffer, 
						  uint32_t *buflen,
						  void **ud)
{
  uint64_t ts;
  if (m_eof) {
    return 0;
  }

  int ret = mpeg3_read_audio_frame(m_file, 
				   (unsigned char **)&m_buffer, 
				   &m_this_frame_size,
				   &m_buffersize_max,
				   m_stream);

  if (ret < 0) {
    m_eof = 1;
    return 0;
  }
  if (m_buffer != NULL) {
    *buffer = m_buffer;
    *buflen = m_this_frame_size;
#ifdef OUTPUT_TO_FILE
    fwrite(m_buffer, m_this_frame_size, 1, m_output_file);
#endif
  }

  uint32_t spf;

  if (m_samples_per_frame == 0) {
    spf = m_this_frame_size / (m_chans * 2);
  } else {
    spf = m_samples_per_frame;
  }
  if (m_freq == 0) {
    return 0;
  }
  ts = m_frame_on;
  ts *= spf;
  ts *= M_64;
  ts /= m_freq;
  m_frame_on++;
  //mpeg3f_message(LOG_DEBUG, "audiostart %ld %llu %d", m_frame_on, ts, m_this_frame_size);
  return ts;
}

void CMpeg3AudioByteStream::used_bytes_for_frame (uint32_t bytes_used)
{

}

int CMpeg3AudioByteStream::skip_next_frame (uint64_t *pts, 
					    int *pSync,
					    uint8_t **buffer, 
					    uint32_t *buflen,
					    void **ud)
{
  uint64_t ts;
  ts = start_next_frame(buffer, buflen, NULL);
  *pts = ts;
  *pSync = 0;
  //*pSync = m_frame_on_has_sync;
  return (1);
}

void CMpeg3AudioByteStream::play (uint64_t start)
{
  m_play_start_time = start;

  double ts;

  ts = UINT64_TO_DOUBLE(start);
  ts /= 1000.0;

  set_timebase(ts);
}


double CMpeg3AudioByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};

/* end file qtime_bytestream.cpp */
