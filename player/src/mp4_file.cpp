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
#include <mp4util/mpeg4_audio_config.h>
#include "our_config_file.h"
#include "codec_plugin_private.h"

CMp4File *Mp4File1 = NULL;
/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
static void close_mp4_file (void)
{
  if (Mp4File1 != NULL) {
    delete Mp4File1;
    Mp4File1 = NULL;
  }
}

int create_media_for_mp4_file (CPlayerSession *psptr, 
			       const char *name,
			       char *errmsg,
			       uint32_t errlen,
			       int have_audio_driver)
{
  MP4FileHandle fh;

  psptr->set_media_close_callback(close_mp4_file);

  fh = MP4Read(name, MP4_DETAILS_ERROR);
  if (!MP4_IS_VALID_FILE_HANDLE(fh)) {
    snprintf(errmsg, errlen, "`%s\' is not an mp4 file", name);
    return -1;
  }

  close_mp4_file();

  Mp4File1 = new CMp4File(fh);
  // quicktime is searchable...
  psptr->session_set_seekable(1);

  int video;
  video = Mp4File1->create_video(psptr, errmsg, errlen);
  if (video < 0) {
    return (-1);
  }
  mp4f_message(LOG_DEBUG, "create video returned %d", video);
  int audio = 0;
  if (have_audio_driver > 0) {
    audio = Mp4File1->create_audio(psptr, errmsg, errlen);
    if (audio < 0) {
      return (-1);
    }
    mp4f_message(LOG_DEBUG, "create audio returned %d", audio);
  }
  if (audio == 0 && video == 0) {
    snprintf(errmsg, errlen, "No known audio or video codecs found in file");
    return (-1);
  }
  if (audio == 0 && Mp4File1->get_audio_tracks() > 0) {
    if (have_audio_driver > 0) 
      snprintf(errmsg, errlen,  "Unknown Audio Codec");
    else 
      snprintf(errmsg, errlen, "No Audio driver - can't play audio");
    return (1);
  }
  if ((Mp4File1->get_video_tracks() != 0) && video == 0) {
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

int CMp4File::create_video (CPlayerSession *psptr,
			    char *errmsg, 
			    uint32_t errlen)
{
  CPlayerMedia *mptr;
  MP4TrackId trackId;

  trackId = MP4FindTrackId(m_mp4file, 0, 
	MP4_VIDEO_TRACK_TYPE, MP4_MPEG4_VIDEO_TYPE);

  if (!MP4_IS_VALID_TRACK_ID(trackId)) {
    return 0;
  }
  uint8_t video_type = MP4GetTrackVideoType(m_mp4file, trackId);
  uint8_t profileID = MP4GetVideoProfileLevel(m_mp4file);
  mp4f_message(LOG_DEBUG, "MP4 - got track %x profile ID %d", 
	       trackId, profileID);
  unsigned char *foo;
  u_int32_t bufsize;
  MP4GetTrackESConfiguration(m_mp4file, trackId, &foo, &bufsize);

  codec_plugin_t *plugin;
  plugin = check_for_video_codec("MP4 FILE",
				 NULL,
				 video_type,
				 profileID,
				 foo, 
				 bufsize);

  if (plugin == NULL) {
    snprintf(errmsg, errlen, "Can't find plugin for video type %d, profile %d",
	     video_type, profileID);
    return 0;
  }

  mptr = new CPlayerMedia(psptr);
  if (mptr == NULL) {
    return (-1);
  }
  video_info_t *vinfo;
  vinfo = (video_info_t *)malloc(sizeof(video_info_t));
  vinfo->height = MP4GetTrackVideoHeight(m_mp4file, trackId);
  vinfo->width = MP4GetTrackVideoWidth(m_mp4file, trackId);

  int ret = mptr->create_video_plugin(plugin, 
				      NULL, // sdp info
				      vinfo, // video info
				      foo,
				      bufsize);

  if (ret < 0) {
    mp4f_message(LOG_ERR, "Failed to create plugin data");
    snprintf(errmsg, errlen, "Failed to start plugin");
    delete mptr;
    return -1;
  }

  CMp4VideoByteStream *vbyte;
  vbyte = new CMp4VideoByteStream(this, trackId);
  if (vbyte == NULL) {
    return (-1);
  }

  ret = mptr->create_from_file(vbyte, TRUE);
  if (ret != 0) {
    return (-1);
  }
  
  mp4f_message(LOG_DEBUG, "ES config has %d bytes", bufsize);
  m_video_tracks = 1;
  return (1);
}

int CMp4File::create_audio (CPlayerSession *psptr,
			    char *errmsg,
			    uint32_t errlen)
{
  CPlayerMedia *mptr;
  u_int32_t trackcnt = 0;
  MP4TrackId trackId;
  unsigned char *userdata = NULL;
  u_int32_t userdata_size;
  const char *compressor = NULL;
  uint8_t audio_type;
  uint8_t profile;
  codec_plugin_t *plugin = NULL;

  while (1) {
    trackId = MP4FindTrackId(m_mp4file, trackcnt, MP4_AUDIO_TRACK_TYPE);

    // no more audio tracks, none are acceptable
    if (!MP4_IS_VALID_TRACK_ID(trackId)) {
      mp4f_message(LOG_ERR, "No supported MP4 audio types");
      return 0;
    }

    audio_type = MP4GetTrackAudioType(m_mp4file, trackId);
    profile = MP4GetAudioProfileLevel(m_mp4file);
    compressor = "MP4 FILE";
    switch (audio_type) {
    case MP4_MPEG1_AUDIO_TYPE:
    case MP4_MPEG2_AUDIO_TYPE:
      break;
    case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
    case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
    case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
    case MP4_MPEG4_AUDIO_TYPE:
      MP4GetTrackESConfiguration(m_mp4file, 
				 trackId, 
				 &userdata, 
				 &userdata_size);
      break;
    default:
      break;
    }

    // track looks good
    if (MP4_IS_VALID_TRACK_ID(trackId)) {
      plugin = check_for_audio_codec(compressor,
				     NULL,
				     audio_type,
				     profile,
				     userdata,
				     userdata_size);
      if (plugin != NULL)
	break;
      snprintf(errmsg, errlen, "Couldn't find plugin for audio type %d profile %d", 
	       audio_type, profile);
    }

    // keep looking
    trackcnt++;
  }

  // can't find any acceptable audio tracks
  if (!MP4_IS_VALID_TRACK_ID(trackId)) {
    return 0;
  }

  if (plugin == NULL) {
    return 0;
  }

  // Say we have at least 1 track...
  m_audio_tracks = 1;

  CMp4AudioByteStream *abyte;
  mptr = new CPlayerMedia(psptr);
  if (mptr == NULL) {
    return (-1);
  }
  abyte = new CMp4AudioByteStream(this, trackId);

  audio_info_t *ainfo;
  ainfo = (audio_info_t *)malloc(sizeof(audio_info_t));
  memset(ainfo, 0, sizeof(*ainfo));

  ainfo->freq = MP4GetTrackTimeScale(m_mp4file, trackId);

  int ret;
  ret = mptr->create_audio_plugin(plugin,
				  NULL, // sdp info
				  ainfo, // audio info
				  userdata,
				  userdata_size);
  if (ret < 0) {
    mp4f_message(LOG_ERR, "Couldn't create audio from plugin %s", plugin->c_name);
    snprintf(errmsg, errlen, "Couldn't start audio plugin %s", 
	     plugin->c_name);
    delete mptr;
    delete abyte;
    return -1;
  }

  ret = mptr->create_from_file(abyte, FALSE);
  if (ret != 0) {
    return (-1);
  }
  return (1);
}

/* end file mp4_file.cpp */
