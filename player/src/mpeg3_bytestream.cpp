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
//#define DEBUG_MPEG_FRAME 1

/**************************************************************************
 * Quicktime stream base class functions
 **************************************************************************/
CMpeg3VideoByteStream::CMpeg3VideoByteStream (mpeg2ps_t *file, int stream)
  : COurInByteStream("mpeg3 video")
{
#ifdef OUTPUT_TO_FILE
  m_output_file = fopen("raw.mp3v", "w");
#endif
  m_file = file;
  m_stream = stream;

  m_eof = 0;
  m_frame_rate = mpeg2ps_get_video_stream_framerate(m_file, m_stream);

  m_max_time = mpeg2ps_get_max_time_msec(m_file);
  m_max_time /= 1000.0;
  mpeg3f_message(LOG_DEBUG, 
		 "Mpeg3 video frame rate %g max time is %g", 
		 m_frame_rate,  m_max_time);
  m_changed_time = 0;
}

CMpeg3VideoByteStream::~CMpeg3VideoByteStream()
{
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
  // nothing here - we let the main routine close the mpeg2ps_t
}

int CMpeg3VideoByteStream::eof(void)
{
  return m_eof != 0;
}



void CMpeg3VideoByteStream::set_timebase (uint64_t mtime)
{
  m_eof = 0;
  
  mpeg2ps_seek_video_frame(m_file, m_stream, mtime);
}

void CMpeg3VideoByteStream::reset (void) 
{
  set_timebase(0);
}

bool CMpeg3VideoByteStream::start_next_frame (uint8_t **buffer, 
					      uint32_t *buflen,
					      frame_timestamp_t *pts,
					      void **ud)
{
  m_eof = mpeg2ps_get_video_frame(m_file, m_stream, 
				  buffer,
				  buflen,
				  NULL,
				  TS_MSEC,
				  &pts->msec_timestamp) == false;
				  
  if (m_eof) {
    return false;
  }

  pts->timestamp_is_pts = false;
#ifdef DEBUG_MPEG_FRAME
  mpeg3f_message(LOG_DEBUG, "start next frame %d "U64, *buflen, 
		 pts->msec_timestamp);
#endif
  return true;
}

void CMpeg3VideoByteStream::used_bytes_for_frame (uint32_t bytes_used)
{
  // nothing here...
}

bool CMpeg3VideoByteStream::skip_next_frame (frame_timestamp_t *pts, 
					    int *pSync,
					    uint8_t **buffer, 
					    uint32_t *buflen,
					    void **ud)
{
  uint8_t ftype;
  ftype = 0;
  m_eof = mpeg2ps_get_video_frame(m_file, m_stream, 
				  buffer, 
				  buflen,
				  &ftype,
				  TS_MSEC,
				  &pts->msec_timestamp) == false;
  if (m_eof)
    return true;
  *pSync = ftype == 1 ? 1 : 0;
  return true;
}

void CMpeg3VideoByteStream::play (uint64_t start)
{
  player_debug_message("mpeg3 play "U64, start);
  m_play_start_time = start;

  set_timebase(start);
}


double CMpeg3VideoByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};



CMpeg3AudioByteStream::CMpeg3AudioByteStream (mpeg2ps_t *file, int stream)
  : COurInByteStream("mpeg3 audio")
{
#ifdef OUTPUT_TO_FILE
  m_output_file = fopen("raw.mp3a", "w");
#endif
  m_file = file;
  m_stream = stream;
  m_eof = 0;
  m_freq = mpeg2ps_get_audio_stream_sample_freq(m_file, m_stream);
  m_max_time = mpeg2ps_get_max_time_msec(m_file);
  m_max_time /= 1000.0;
  mpeg3f_message(LOG_DEBUG, "audio max time is %g", m_max_time);
  m_changed_time = 0;
  m_frame_on = 0;
}

CMpeg3AudioByteStream::~CMpeg3AudioByteStream()
{
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
}

int CMpeg3AudioByteStream::eof(void)
{
  return m_eof != 0;
}



void CMpeg3AudioByteStream::set_timebase (uint64_t mtime)
{
  m_eof = 0;
  mpeg2ps_seek_audio_frame(m_file, m_stream, mtime);
}

void CMpeg3AudioByteStream::reset (void) 
{
  set_timebase(0);
}

bool CMpeg3AudioByteStream::start_next_frame (uint8_t **buffer, 
					      uint32_t *buflen,
					      frame_timestamp_t *pts,
					      void **ud)
{
  uint64_t ts;
  uint32_t freq_ts;
  if (m_eof) {
    return false;
  }

  m_eof = mpeg2ps_get_audio_frame(m_file,    
				  m_stream,
				  buffer,
				  buflen,
				  TS_MSEC,
				  &freq_ts,
				  &ts) == false;
  
  if (m_eof) return false;

#ifdef OUTPUT_TO_FILE
  fwrite(*buffer, buflen, 1, m_output_file);
#endif

  pts->audio_freq_timestamp = freq_ts;
  pts->audio_freq = m_freq;
  pts->msec_timestamp = ts;
  pts->timestamp_is_pts = false;
#ifdef DEBUG_MPEG_FRAME
  mpeg3f_message(LOG_DEBUG, "audiostart %u %u "U64, *buflen, freq_ts, ts);
#endif
  return true;
}

void CMpeg3AudioByteStream::used_bytes_for_frame (uint32_t bytes_used)
{

}

bool CMpeg3AudioByteStream::skip_next_frame (frame_timestamp_t *pts, 
					    int *pSync,
					    uint8_t **buffer, 
					    uint32_t *buflen,
					    void **ud)
{
  bool ret;
  ret = start_next_frame(buffer, buflen, pts, NULL);
  *pSync = 1;
  //*pSync = m_frame_on_has_sync;
  return ret;
}

void CMpeg3AudioByteStream::play (uint64_t start)
{
  m_play_start_time = start;

  set_timebase(start);
}


double CMpeg3AudioByteStream::get_max_playtime (void) 
{
  return (m_max_time);
};

/* end file qtime_bytestream.cpp */
