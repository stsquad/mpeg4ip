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
 * avi_file.cpp - provides generic class for avi file access control.
 * file access is then used by avi audio and video bytestreams.
 */
#include "systems.h"
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "media_utils.h"
#include "avi_bytestream.h"
#include "avi_file.h"
#include "codec_plugin_private.h"

CAviFile *Avifile1 = NULL;
/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
int create_media_for_avi_file (CPlayerSession *psptr, 
			       const char *name,
			       char *errmsg,
			       uint32_t errlen,
			       int have_audio_driver)
{
  avi_t *avi;

  avi = AVI_open_input_file(name, 1);
  if (avi == NULL) {
    snprintf(errmsg, errlen, AVI_strerror());
    player_error_message(errmsg);
    return (-1);
  }

  if (Avifile1 != NULL) 
    delete Avifile1;
  Avifile1 = new CAviFile(name, avi);
  // quicktime is searchable...
  psptr->session_set_seekable(1);

   int video;
   video = Avifile1->create_video(psptr, errmsg, errlen);
   if (video < 0) {
     return (-1);
   }
   int audio;
   if (have_audio_driver > 0) {
     audio = Avifile1->create_audio(psptr, errmsg, errlen);
     if (audio < 0) {
       return (-1);
     }
   } else
     audio = 0;
   if (audio == 0 && video == 0) {
     snprintf(errmsg, errlen, "No valid audio or video codecs in avi file");
     return (-1);
   }
   if (audio == 0 && Avifile1->get_audio_tracks() > 0) {
     snprintf(errmsg, errlen, "Unknown Audio Codec in avi file ");
     return (1);
   }
   if (video != 1) {
     snprintf(errmsg, errlen, "Unknown Video Codec %s in avi file",
	     AVI_video_compressor(Avifile1->get_file()));
    return (1);
  }
  return (0);
}

CAviFile::CAviFile (const char *name, avi_t *avi)
{
  m_name = strdup(name);
  m_file = avi;
  m_file_mutex = SDL_CreateMutex();
}

CAviFile::~CAviFile (void)
{
  free(m_name);
  m_name = NULL;
  AVI_close(m_file);
  if (m_file_mutex) {
    SDL_DestroyMutex(m_file_mutex);
    m_file_mutex = NULL;
  }
}

int CAviFile::create_video (CPlayerSession *psptr,
			    char *errmsg, 
			    uint32_t errlen)
{
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;

  const char *codec_name = AVI_video_compressor(m_file);
  plugin = check_for_video_codec(codec_name, 
				 NULL,
				 -1,
				 -1,
				 NULL,
				 0);
  if (plugin == NULL) {
    snprintf(errmsg,errlen, "Couldn't find video codec `%s\'", 
	     codec_name);
    player_debug_message(errmsg);
    return -1;
  }

  mptr = new CPlayerMedia(psptr);
  if (mptr == NULL) {
    return (-1);
  }
  
  video_info_t *vinfo = (video_info_t *)malloc(sizeof(video_info_t));
  if (vinfo == NULL) 
    return (-1);
  vinfo->height = AVI_video_height(m_file);
  vinfo->width = AVI_video_width(m_file);
  player_debug_message("avi file h %d w %d frame rate %g", 
		       vinfo->height,
		       vinfo->width,
		       AVI_video_frame_rate(m_file));

  int ret;
  ret = mptr->create_video_plugin(plugin,
				  NULL,
				  vinfo,
				  NULL,
				  0);
  if (ret < 0) {
    snprintf(errmsg, errlen, "Failed to create video plugin %s", 
	     codec_name);
    player_error_message("Failed to create plugin data");
    delete mptr;
    return -1;
  }
  CAviVideoByteStream *vbyte = new CAviVideoByteStream(this);
  if (vbyte == NULL) {
    delete mptr;
    return (-1);
  }
  vbyte->config(AVI_video_frames(m_file),
		AVI_video_frame_rate(m_file));
  ret = mptr->create_from_file(vbyte, TRUE);
  if (ret != 0) {
    return (-1);
  }
  player_debug_message("Video Max time is %g", vbyte->get_max_playtime());
  return (1);
}

int CAviFile::create_audio (CPlayerSession *psptr, char *errmsg, uint32_t errlen)
{
  m_audio_tracks = 0;
  //  CPlayerMedia *mptr;

  if (AVI_audio_bytes(m_file) != 0) {
    player_debug_message("Avi file audio - channels %d bits %d format %x rate %ld bytes %ld", 
			 AVI_audio_channels(m_file),
			 AVI_audio_bits(m_file),
			 AVI_audio_format(m_file),
			 AVI_audio_rate(m_file),
			 AVI_audio_bytes(m_file));
  }
#if 0

    const char *codec = "mp3 ";
    if (lookup_audio_codec_by_name(codec) < 0) {
      player_debug_message("Couldn't find audio codec %s", codec);
      return (0);
    }
    CAviAudioByteStream *abyte;
    mptr = new CPlayerMedia(psptr);
    if (mptr == NULL) {
      return (-1);
    }
    abyte = new CQTAudioByteStream(this, mptr, 0);
    long sample_rate = AVI_audio_rate(m_file);
    float sr = (float)sample_rate;
    long len = AVI_audio_bytes(m_file);
    int duration = len / 2;
    player_debug_message("audio - rate %g len %ld samples %d", sr, len, duration);
    audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
    audio->freq = (int)sr;
    mptr->set_codec_type(codec);
    mptr->set_audio_info(audio);
    abyte->config(len, sr, duration);
    player_debug_message("audio Max time is %g", abyte->get_max_playtime());
    int ret = mptr->create_from_file(abyte, FALSE);
    if (ret != 0) {
      return (-1);
    }
  }
  return (1);
#else
  return (0);
#endif
}

/* end file qtime_file.cpp */
