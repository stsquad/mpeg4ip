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
#include "mpeg2t_bytestream.h"
#include "player_util.h"
//#define DEBUG_MPEG2T_FRAME 1
//#define DEBUG_MPEG2T_PSTS 1
/**************************************************************************
 * 
 **************************************************************************/
CMpeg2tByteStream::CMpeg2tByteStream (mpeg2t_es_t *es_pid,
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
  m_es_pid = es_pid;
  m_has_video = has_video;
  m_timestamp_loaded = 0;
  m_frame = NULL;
}

CMpeg2tByteStream::~CMpeg2tByteStream()
{
  mpeg2t_stream_t *sptr;
  sptr = (mpeg2t_stream_t *)mpeg2t_es_get_userdata(m_es_pid); 
  mpeg2t_stop_saving_frames(m_es_pid); // eliminate any callbacks
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
  CHECK_AND_FREE(m_frame);
}

int CMpeg2tByteStream::eof(void)
{
  return 0; // no eof
}

int CMpeg2tByteStream::have_no_data (void)
{
  return (m_es_pid->list == NULL);
}

void CMpeg2tByteStream::reset (void) 
{
  mpeg2t_frame_t *p;
  do {
    p = mpeg2t_get_es_list_head(m_es_pid);
    if (p != NULL)
      free(p);
  } while (p != NULL);
}

void CMpeg2tByteStream::pause (void)
{
  mpeg2t_stream_t *sptr;
  sptr = (mpeg2t_stream_t *)mpeg2t_es_get_userdata(m_es_pid);
  mpeg2t_stop_saving_frames(m_es_pid);
  reset();
  sptr->m_buffering = 1;
}

void CMpeg2tByteStream::play (uint64_t start_time)
{
  m_play_start_time = start_time;
  mpeg2t_start_saving_frames(m_es_pid);
}

uint64_t CMpeg2tByteStream::start_next_frame (uint8_t **buffer, 
					      uint32_t *buflen,
					      void **ud)
{
  uint64_t ret;
  CHECK_AND_FREE(m_frame);
    
  m_frame = mpeg2t_get_es_list_head(m_es_pid);
  if (m_frame != NULL) {
    if (get_timestamp_for_frame(m_frame, ret) >= 0) {
      *buffer = m_frame->frame;
      *buflen = m_frame->frame_len;
#ifdef DEBUG_MPEG2T_FRAME
      player_debug_message("%s - len %d time %llu", 
			   m_name, *buflen, ret);
#endif
      return (ret);
    }
  }
  return 0;
}

void CMpeg2tByteStream::used_bytes_for_frame (uint32_t bytes_used)
{
  // nothing here yet...
}

int CMpeg2tByteStream::skip_next_frame (uint64_t *pts, 
					int *pSync,
					uint8_t **buffer, 
					uint32_t *buflen,
					void **userdata)
{
  uint64_t ts;
  ts = start_next_frame(buffer, buflen, NULL);
  *pts = ts;
  *pSync = 0; //m_frame_on_has_sync;
  if (*buffer == NULL) return 0;
  return (1);
}
// left off here...

double CMpeg2tByteStream::get_max_playtime (void) 
{
  // we shouldn't know - we might need to do something when
  // we're running sdp...
  return (0.0);
};

int CMpeg2tVideoByteStream::get_timestamp_for_frame (mpeg2t_frame_t *fptr,
						     uint64_t &outts)
{
  uint64_t ts;
  double value = 1000.0 / m_es_pid->frame_rate;
  uint64_t frame_time = (uint64_t)value;
#ifdef DEBUG_MPEG2T_PSTS
  player_debug_message("video frame len %d have psts %d ts %llu", 
		       fptr->frame_len, fptr->have_ps_ts, fptr->ps_ts);
#endif
  if (fptr->have_ps_ts == 0) {
    // We don't have a timestamp on this - just increment from
    // the previous timestamp.
    if (m_timestamp_loaded == 0) return -1;
    if (m_es_pid->info_loaded == 0) return -1;

    outts = m_prev_ts + frame_time;
    m_have_prev_frame_type = 1;
    m_prev_frame_type = fptr->frame_type;
    m_prev_ts = outts;
    return 0;
  }
  m_timestamp_loaded = 1;
  ts = fptr->ps_ts;
  if (m_have_prev_frame_type) {
    if (fptr->frame_type == 3) {
      // B frame
      outts = ts + frame_time;
    } else {
      outts = m_prev_ts + frame_time;
    }
  } else {
#if 0
    outts = calc_this_ts_from_future(frame_type, ts);
#else
    player_error_message( "no psts and no prev frame");
    outts = ts;
#endif
  }

  m_have_prev_frame_type = 1;
  m_prev_frame_type = fptr->frame_type;
  m_prev_ts = outts;
  
  return 0;
}

int CMpeg2tAudioByteStream::get_timestamp_for_frame (mpeg2t_frame_t *fptr,
						     uint64_t &ts)
{
#ifdef DEBUG_MPEG2T_PSTS
  player_debug_message("audio frame len %d have psts %d ts %llu", 
		       fptr->frame_len, fptr->have_ps_ts, fptr->ps_ts);
#endif
  if (fptr->have_ps_ts != 0) {
    m_timestamp_loaded = 1;
    m_last_timestamp = fptr->ps_ts;
    m_frames_since_last_timestamp = 0;
    ts = m_last_timestamp;
  }

  if (m_timestamp_loaded == 0) return -1;
  if (m_es_pid->info_loaded == 0) return -1;

  m_frames_since_last_timestamp++;

  double value = (m_frames_since_last_timestamp * m_es_pid->sample_per_frame) /
    m_es_pid->sample_freq;
  uint64_t val = (uint64_t)value;
  ts = m_last_timestamp + val;
  return 0;
}
  
/* end file mpeg2t_bytestream.cpp */
