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
#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "media_utils.h"
#include <mp4.h>
#include "mp4_bytestream.h"
#include "mp4_file.h"
#include "mpeg4_audio_config.h"
#include "our_config_file.h"

CMp4File *Mp4File1 = NULL;
/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
int create_media_for_mp4_file (CPlayerSession *psptr, 
			       const char *name,
			       const char **errmsg,
			       int have_audio_driver)
{
  MP4FileHandle fh;

  fh = MP4Read(name, 0);
  if (!MP4_IS_VALID_FILE_HANDLE(fh)) {
    *errmsg = "Cannot open mp4 file";
    return -1;
  }
  if (Mp4File1 != NULL) 
    delete Mp4File1;
  Mp4File1 = new CMp4File(fh);
  // quicktime is searchable...
  psptr->session_set_seekable(1);

  int video;
  video = Mp4File1->create_video(psptr);
  if (video < 0) {
    *errmsg = "Internal MP4 error";
    return (-1);
  }
  player_debug_message("create video returned %d", video);
  int audio = 0;
  if (have_audio_driver > 0) {
    audio = Mp4File1->create_audio(psptr);
    if (audio < 0) {
      *errmsg = "Internal mp4 file error";
      return (-1);
    }
    player_debug_message("create audio returned %d", audio);
  }
  if (audio == 0 && video == 0) {
    *errmsg = "No valid codecs";
    return (-1);
  }
  if (audio == 0 && Mp4File1->get_audio_tracks() > 0) {
    if (have_audio_driver > 0) 
      *errmsg = "Invalid Audio Codec";
    else 
      *errmsg = "No Audio driver - no sound";
    return (1);
  }
  if ((Mp4File1->get_video_tracks() != 0) && video == 0) {
    *errmsg = "Invalid Video Codec";
    return (1);
  }
  return (0);
}

CMp4File::CMp4File (MP4FileHandle filehandle)
{
  m_mp4file = filehandle;
  m_file_mutex = SDL_CreateMutex();
  m_audio_tracks = 0;
  m_video_tracks = 0;
}

CMp4File::~CMp4File (void)
{
  MP4Close(m_mp4file);
  m_mp4file = NULL;
  if (m_file_mutex) {
    SDL_DestroyMutex(m_file_mutex);
    m_file_mutex = NULL;
  }
}

int CMp4File::create_video (CPlayerSession *psptr)
{
  CPlayerMedia *mptr;
  int trackcnt = 0;
  MP4TrackId trackId;

  do {
    trackId = MP4FindTrackId(m_mp4file, trackcnt, "video");
    trackcnt++;
  } while (!MP4_IS_VALID_TRACK_ID(trackId) && trackcnt < 10);

  if (!MP4_IS_VALID_TRACK_ID(trackId)) {
    return 0;
  }
  trackcnt--;

  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    return (-1);
  }
  CMp4VideoByteStream *vbyte;
  vbyte = new CMp4VideoByteStream(this, trackId);
  if (vbyte == NULL) {
    return (-1);
  }

  /*  
      vbyte->config(quicktime_video_length(m_qtfile, ix),
      quicktime_video_frame_rate(m_qtfile, ix),
      quicktime_video_time_scale(m_qtfile, ix));
      player_debug_message("Video Max time is %g", vbyte->get_max_playtime());
  */

  int ret = mptr->create_from_file(psptr, vbyte, TRUE);
  if (ret != 0) {
    return (-1);
  }
  
  uint8_t profileID = MP4GetVideoProfileLevel(m_mp4file);
  player_debug_message("MP4 - got profile ID %d", profileID);
  if (profileID >= 1 && profileID <= 3) {
    if (config.get_config_value(CONFIG_USE_MPEG4_ISO_ONLY) == 1) {
      mptr->set_codec_type("mp4v");
    } else
      mptr->set_codec_type("divx");
  } else {
    mptr->set_codec_type("mp4v");
  }
  
  unsigned char *foo;
  u_int32_t bufsize;
  MP4GetTrackESConfiguration(m_mp4file, trackId, &foo, &bufsize);
#if 0
  if (ret >= 0) {
#endif
    mptr->set_user_data(foo, bufsize);
    player_debug_message("ES config has %d bytes", bufsize);
#if 0
  } else {
    player_error_message("Can't read video ES config");
    return (-1);
  }
#endif
  m_video_tracks = 1;
  return (1);
}

int CMp4File::create_audio (CPlayerSession *psptr)
{
  CPlayerMedia *mptr;
  MP4TrackId trackId;
  unsigned char *foo = NULL;
  u_int32_t bufsize;
  uint8_t audio_type;
  int audio_is_mp3 = 0;

  trackId = MP4FindTrackId(m_mp4file, 0, "audio");

  if (!MP4_IS_VALID_TRACK_ID(trackId)) {
    return 0;
  }

  // Say we have at least 1 track...
  m_audio_tracks = 1;

  audio_type = MP4GetTrackAudioType(m_mp4file, trackId);
  switch (audio_type) {
  case MP4_MPEG1_AUDIO_TYPE:
  case MP4_MPEG2_AUDIO_TYPE:
    audio_is_mp3 = 1;
    break;
  case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
  case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
  case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
  case MP4_MPEG4_AUDIO_TYPE:
    mpeg4_audio_config_t audio_config;
    audio_is_mp3 = 0;
    MP4GetTrackESConfiguration(m_mp4file, trackId, &foo, &bufsize);
    if (foo != NULL) {
      decode_mpeg4_audio_config(foo, bufsize, &audio_config);

      if (audio_object_type_is_aac(&audio_config) == 0) {
	// should be unsupported
	player_error_message("MP4 audio object type %d not supported", audio_config.audio_object_type);
	return (0);
      }
    }
    break;
  case MP4_PRIVATE_AUDIO_TYPE:
  case MP4_INVALID_AUDIO_TYPE:
  default:
    player_error_message("Unsupported MP4 audio type %x", audio_type);
    return (0);
  }

  CMp4AudioByteStream *abyte;
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    return (-1);
  }
  abyte = new CMp4AudioByteStream(this, trackId);
  
  if (audio_is_mp3 == 0) {
    mptr->set_codec_type("aac ");
    mptr->set_user_data(foo, bufsize);
  } else {
    mptr->set_codec_type("mp3 ");
  }

  int ret;
  ret = mptr->create_from_file(psptr, abyte, FALSE);
  if (ret != 0) {
    return (-1);
  }
  return (1);
}

/* end file mp4_file.cpp */
