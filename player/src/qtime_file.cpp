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
 * qtime_file.cpp - provides generic class for quicktime file access control.
 * file access is then used by quicktime audio and video bytestreams.
 */
#include <unistd.h>
#include <stdint.h>
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include <quicktime.h>
#include "qtime_bytestream.h"
#include "qtime_file.h"

CQtimeFile *QTfile1 = NULL;
/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
int create_media_for_qtime_file (CPlayerSession *psptr, 
				 const char *name,
				 const char **errmsg)
{
  if (quicktime_check_sig(name) == 0) {
    *errmsg = "File is not quicktime";
    player_error_message("File %s is not quicktime", name);
    return (1);
  }
  if (QTfile1 != NULL) 
    delete QTfile1;
  QTfile1 = new CQtimeFile(name);
  // quicktime is searchable...
  psptr->session_set_seekable(1);

  if (QTfile1->create_video(psptr) == 1) {
    *errmsg = "Internal quicktime error";
    return (1);
  }
#if 0
  if (QTfile1 != NULL) 
    delete QTfile1;
  QTfile1 = new CQtimeFile(name);
  // quicktime is searchable...
  psptr->session_set_seekable(1);
#endif

  if (QTfile1->create_audio(psptr) == 1) {
    *errmsg = "Internal quicktime error";
    return (1);
  }
  return (0);
}

CQtimeFile::CQtimeFile (const char *name)
{
  m_name = strdup(name);
  m_qtfile = quicktime_open(m_name, 1, 0, 0);
  m_file_mutex = SDL_CreateMutex();
}

CQtimeFile::~CQtimeFile (void)
{
  free(m_name);
  m_name = NULL;
  quicktime_close(m_qtfile);
  if (m_file_mutex) {
    SDL_DestroyMutex(m_file_mutex);
    m_file_mutex = NULL;
  }
}

int CQtimeFile::create_video (CPlayerSession *psptr)
{
  CPlayerMedia *mptr;

  m_video_tracks = quicktime_video_tracks(m_qtfile);
  for (int ix = 0; ix < m_video_tracks; ix++) {
    video_info_t *vinfo;
    mptr = new CPlayerMedia;
    if (mptr == NULL) {
      return 1;
    }
    CQTVideoByteStream *vbyte;
    vbyte = new CQTVideoByteStream(this, mptr, ix);
    if (vbyte == NULL) {
      return (1);
    }
    vbyte->config(quicktime_video_length(m_qtfile, ix),
		  quicktime_video_frame_rate(m_qtfile, ix));
    int ret = mptr->create_from_file(psptr, vbyte, TRUE);
    if (ret != 0) {
      return (1);
    }
    // This needs much work.  We need to figure a way to get extensions
    // down to the audio and video codecs.
    vinfo = (video_info_t *)malloc(sizeof(video_info_t));
    if (vinfo == NULL) 
      return (1);
    vinfo->height = quicktime_video_height(m_qtfile, ix);
    vinfo->width = quicktime_video_width(m_qtfile, ix);
    vinfo->frame_rate = (int)quicktime_video_frame_rate(m_qtfile, ix);
    mptr->set_video_info(vinfo);
    player_debug_message("qtime file h %d w %d frame rate %d", 
			 vinfo->height,
			 vinfo->width,
			 vinfo->frame_rate);
  }

  return (0);
}
int CQtimeFile::create_audio (CPlayerSession *psptr)
{
  CPlayerMedia *mptr;

  m_audio_tracks = quicktime_audio_tracks(m_qtfile);
  if (m_audio_tracks > 0) {
    CQTAudioByteStream *abyte;
    mptr = new CPlayerMedia;
    if (mptr == NULL) {
      return 1;
    }
    abyte = new CQTAudioByteStream(this, mptr, 0, 1);
    long sample_rate = quicktime_audio_sample_rate(m_qtfile, 0);
    float sr = (float)sample_rate;
    long len = quicktime_audio_length(m_qtfile, 0);
    int duration = quicktime_audio_sample_duration(m_qtfile, 0);
    player_debug_message("audio - rate %g len %ld", sr, len);
    audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
    audio->freq = (int)sr;
    audio->stream_has_length = 1;
    mptr->set_audio_info(audio);
    abyte->config(len, sr, duration);
    int ret = mptr->create_from_file(psptr, abyte, FALSE);
    if (ret != 0) {
      return (1);
    }
  }
  // add audio processing here...
  return (0);
}

/* end file qtime_file.cpp */
