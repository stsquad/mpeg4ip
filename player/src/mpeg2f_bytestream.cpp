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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg2f_bytestream.cpp - convert mpeg2 transport stream file to bytestream
 * using lib/mpeg2t to read file.
 */

#include "mpeg4ip.h"
#include "mpeg2f_bytestream.h"
#include "player_util.h"
#include "mp4av.h"
//#define DEBUG_MPEG2T_FRAME 1
//#define DEBUG_MPEG2T_PSTS 1

static inline bool convert_psts (mpeg2t_es_t *es_pid,
				 mpeg2t_frame_t *fptr,
				 uint64_t start_psts)
{
  uint64_t ps_ts;
#ifdef DEBUG_MPEG2T_PSTS
  uint64_t initial;
#endif
  // we want to 0 out the timestamps of the incoming transport
  // stream so that the on-demand shows the correct timestamps

  // see if we've set the initial timestamp
  // here, we want to check for a gross change in the psts
  if (fptr->have_ps_ts == 0 && fptr->have_dts == 0) return true;

  if (fptr->have_dts) {
    ps_ts = fptr->dts;
    if (ps_ts < start_psts) return false;
#ifdef DEBUG_MPEG2T_PSTS
    initial = fptr->dts;
#endif
    fptr->dts -= start_psts;
#ifdef DEBUG_MPEG2T_PSTS
    player_debug_message(" convert dts "U64" to "U64" "U64,
			 fptr->dts, start_psts, initial);
#endif
  } else {
    ps_ts = fptr->ps_ts;
    if (ps_ts < start_psts) return false;
#ifdef DEBUG_MPEG2T_PSTS
    initial = fptr->dts;
#endif
    fptr->ps_ts -= start_psts;
#ifdef DEBUG_MPEG2T_PSTS
    player_debug_message(" convert psts "U64" to "U64" "U64,
			 fptr->ps_ts, start_psts, initial);
#endif
  }
  return true;
}

/**************************************************************************
 * 
 **************************************************************************/
CMpeg2fByteStream::CMpeg2fByteStream (CMpeg2tFile *f,
				      mpeg2t_es_t *es_pid,
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
  m_file = f;
  m_es_pid = es_pid;
  m_has_video = has_video;
  m_timestamp_loaded = 0;
  m_frame = NULL;
}

CMpeg2fByteStream::~CMpeg2fByteStream()
{
  mpeg2t_set_frame_status(m_es_pid, MPEG2T_PID_NOTHING); // eliminate any callbacks
#ifdef OUTPUT_TO_FILE
  fclose(m_output_file);
#endif
  mpeg2t_free_frame(m_frame);
}

int CMpeg2fByteStream::eof(void)
{
  return m_es_pid->list == NULL && m_file->eof();
}

// clear out the cruft.
void CMpeg2fByteStream::reset (void) 
{
  mpeg2t_frame_t *p;
  do {
    p = mpeg2t_get_es_list_head(m_es_pid);
    if (p != NULL)
      mpeg2t_free_frame(p);
  } while (p != NULL);
}

// pause - just reset.  We might not want to do this if we
// need to start where we stopped.
void CMpeg2fByteStream::pause (void)
{
  reset();
}

void CMpeg2fByteStream::play (uint64_t start_time)
{
  m_play_start_time = start_time;
  m_file->seek_to(start_time);
  mpeg2t_clear_frames(m_es_pid);
}

bool CMpeg2fByteStream::start_next_frame (uint8_t **buffer, 
					  uint32_t *buflen,
					  frame_timestamp_t *ts,
					  void **ud)
{
  if (m_frame) {
    mpeg2t_free_frame(m_frame);
  }
  
  // see if there is a frame ready for this pid.  If not, 
  // request one
  m_frame = mpeg2t_get_es_list_head(m_es_pid);
  if (m_frame == NULL) {
    m_file->get_frame_for_pid(m_es_pid);
    m_frame = mpeg2t_get_es_list_head(m_es_pid);
    if (m_frame == NULL) {
      player_debug_message("%s no frame %d", m_name, m_file->eof());
      return false;
    }
  }
  // Convert the psts
  if (convert_psts(m_es_pid, m_frame, m_file->get_start_psts()) == false) {
    return false;
  }

  // convert the psts into a timestamp
  if (get_timestamp_for_frame(m_frame, ts) >= 0) {
    *buffer = m_frame->frame;
    *buflen = m_frame->frame_len;
#ifdef DEBUG_MPEG2T_FRAME
    player_debug_message("%s - len %d time "U64" ftype %d", 
			 m_name, *buflen, ret, m_frame->frame_type);
#endif
    return true;
  }
  return false;
}

void CMpeg2fByteStream::used_bytes_for_frame (uint32_t bytes_used)
{
  // nothing here yet...
}

bool CMpeg2fByteStream::skip_next_frame (frame_timestamp_t *pts, 
					int *pSync,
					uint8_t **buffer, 
					uint32_t *buflen,
					void **userdata)
{
  *pSync = 0; //m_frame_on_has_sync;
  return start_next_frame(buffer, buflen, pts, NULL);
}

double CMpeg2fByteStream::get_max_playtime (void) 
{
  return m_file->get_max_time();
};

void CMpeg2fVideoByteStream::reset(void)
{
  m_have_prev_frame_type = 0;
  m_timestamp_loaded = 0;
  CMpeg2fByteStream::reset();
}

void CMpeg2fVideoByteStream::play (uint64_t start_time)
{
  CMpeg2fByteStream::play(start_time);
  // make sure we start with the next i frame
  do {
    m_file->get_frame_for_pid(m_es_pid);
    while (m_es_pid->list && m_es_pid->list->frame_type != 1) {
      mpeg2t_free_frame(mpeg2t_get_es_list_head(m_es_pid));
    }
  } while (!m_file->eof() && 
	   (m_es_pid->list == NULL || m_es_pid->list->frame_type != 1));
}

int CMpeg2fVideoByteStream::get_timestamp_for_frame (mpeg2t_frame_t *fptr,
						     frame_timestamp_t *ts)

{
#ifdef DEBUG_MPEG2T_PSTS
  if (fptr->have_dts) {
    player_debug_message("video frame len %d have  dts %d ts "U64,
			 fptr->frame_len, fptr->have_dts, fptr->dts);
  } if (fptr->have_ps_ts) {
    player_debug_message("video frame len %d have psts %d ts "U64, 
			 fptr->frame_len, fptr->have_ps_ts, fptr->ps_ts);
  }
#endif
#if 0
  if (m_es_pid->stream_type == MPEG2T_STREAM_H264) {
    if (fptr->have_dts || fptr->have_ps_ts) {
      if (fptr->have_dts)
	outts = fptr->dts;
      else
	outts = fptr->ps_ts;
      outts *= TO_U64(1000);
      outts /= TO_U64(90000); // get msec from 90000 timescale
      return 0;
    }
    return -1;
  }
  uint64_t ts;
  //  m_es_pid->frame_rate = 24;
  double value = 90000.0 / m_es_pid->frame_rate;
  uint64_t frame_time = (uint64_t)value;
  if (fptr->have_ps_ts == 0 && fptr->have_dts == 0) {
    // We don't have a timestamp on this - just increment from
    // the previous timestamp.
    if (m_timestamp_loaded == 0) return -1;
    if (m_es_pid->info_loaded == 0) return -1;

    outts = m_prev_ts + frame_time;
    m_have_prev_frame_type = 1;
    m_prev_frame_type = fptr->frame_type;
    m_prev_ts = outts;
    outts *= TO_U64(1000);
    outts /= TO_U64(90000); // get msec from 90000 timescale
    return 0;
  }
  m_timestamp_loaded = 1;
  if (fptr->have_dts != 0) {
    outts = fptr->dts;
  } else {
    ts = fptr->ps_ts;

    if (m_have_prev_frame_type) {
      if (fptr->frame_type == 3) {
	// B frame
	outts = ts;
      } else {
	outts = m_prev_ts + frame_time;
      }
    } else {
      if (fptr->frame_type == 1) {
	uint16_t temp_ref = MP4AV_Mpeg3PictHdrTempRef(fptr->frame + fptr->pict_header_offset);
	ts -= ((temp_ref + 1) * m_es_pid->tick_per_frame);
	outts = ts;
      } else {
	player_error_message( "no psts and no prev frame");
	outts = ts;
      }
    }
  }

  m_have_prev_frame_type = 1;
  m_prev_frame_type = fptr->frame_type;
  m_prev_ts = outts;

  outts *= TO_U64(1000);
  outts /= TO_U64(90000); // get msec from 90000 timescale
  
  return 0;
#endif
  ts->timestamp_is_pts = fptr->have_dts == false;
  uint64_t outts;
  if (fptr->have_dts)
    outts = fptr->dts;
  else if (fptr->have_ps_ts) 
    outts = fptr->ps_ts;
  else 
    outts = m_prev_ts + 3003; // just a guess for now

  m_prev_ts = outts;
  outts *= TO_U64(1000);
  outts /= TO_U64(90000); // get msec from 90000 timescale
  ts->msec_timestamp = outts;
  return 0;
}

void CMpeg2fAudioByteStream::reset(void)
{
  m_timestamp_loaded = 0;
  CMpeg2fByteStream::reset();
}

int CMpeg2fAudioByteStream::get_timestamp_for_frame (mpeg2t_frame_t *fptr,
						     frame_timestamp_t *ts)
{
  uint64_t pts_in_msec;
  // all ts for audio are stored in msec, not in timescale
#ifdef DEBUG_MPEG2T_PSTS
  player_debug_message("audio frame len %d have  dts %d ts "U64, 
		       fptr->frame_len, fptr->have_dts, fptr->dts);
  player_debug_message("audio frame len %d have psts %d ts "U64" %d %d", 
		       fptr->frame_len, fptr->have_ps_ts, fptr->ps_ts,
		       m_es_pid->sample_per_frame, 
		       m_es_pid->sample_freq);
#endif
  if (fptr->have_ps_ts != 0 || fptr->have_dts != 0) {
    m_timestamp_loaded = 1;
    pts_in_msec = fptr->have_dts ? fptr->dts : fptr->ps_ts;
    m_audio_last_freq_timestamp = 
      ((pts_in_msec * m_es_pid->sample_freq) / TO_U64(90000));
    ts->audio_freq_timestamp = m_audio_last_freq_timestamp;
    ts->audio_freq = m_es_pid->sample_freq;
    pts_in_msec *= TO_U64(1000);
    pts_in_msec /= TO_U64(90000);
    m_last_timestamp = pts_in_msec;
    ts->msec_timestamp = m_last_timestamp;
    m_frames_since_last_timestamp = 0;
    return 0;
  }

  if (m_timestamp_loaded == 0) return -1;
  if (m_es_pid->info_loaded == 0) return -1;

  ts->msec_timestamp = m_last_timestamp;
  ts->audio_freq_timestamp = m_audio_last_freq_timestamp;
  ts->audio_freq = m_es_pid->sample_freq;
  return 0;
}

/* end file mpeg2t_bytestream.cpp */
