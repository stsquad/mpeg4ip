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
#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "media_utils.h"
#include "avi_bytestream.h"
#include "avi_file.h"
#include "codec_plugin_private.h"
#include "our_config_file.h"

static void close_avi_file (void *data)
{
  CAviFile *Avifile1 = (CAviFile *)data;
  if (Avifile1 != NULL) {
    delete Avifile1;
    Avifile1 = NULL;
  }
}

/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
int create_media_for_avi_file (CPlayerSession *psptr, 
			       const char *name,
			       int have_audio_driver,
			       control_callback_vft_t *cc_vft)
{
  CAviFile *Avifile1 = NULL;
  avi_t *avi;
  CPlayerMedia *mptr;
  avi = AVI_open_input_file(name, 1);
  if (avi == NULL) {
    psptr->set_message("%s", AVI_strerror());
    player_error_message("%s", AVI_strerror());
    return (-1);
  }

  int video_count = 1;
  codec_plugin_t *plugin;
  video_query_t vq;

  const char *codec_name = AVI_video_compressor(avi);
  player_debug_message("Trying avi video codec %s", codec_name);
  plugin = check_for_video_codec(STREAM_TYPE_AVI_FILE,
				 codec_name, 
				 NULL,
				 -1,
				 -1,
				 NULL,
				 0, 
				 &config);
  if (plugin == NULL) {
    video_count = 0;
  } else {
    vq.track_id = 1;
    vq.stream_type = STREAM_TYPE_AVI_FILE;
    vq.compressor = codec_name;
    vq.type = -1;
    vq.profile = -1;
    vq.fptr = NULL;
    vq.h = AVI_video_height(avi);
    vq.w = AVI_video_width(avi);
    vq.frame_rate = AVI_video_frame_rate(avi);
    vq.config = NULL;
    vq.config_len = 0;
    vq.enabled = 0;
    vq.reference = NULL;
  }

  int have_audio = 0;
  int audio_count = 0;
  audio_query_t aq;

  if (AVI_audio_bytes(avi) != 0) {
    have_audio = 1;
    plugin = check_for_audio_codec(STREAM_TYPE_AVI_FILE,
				   NULL,
				   NULL,
				   AVI_audio_format(avi), 
				   -1, 
				   NULL, 
				   0,
				   &config);
    if (plugin != NULL) {
      audio_count = 1;
      aq.track_id = 1;
      aq.stream_type = STREAM_TYPE_AVI_FILE;
      aq.compressor = NULL;
      aq.type = AVI_audio_format(avi);
      aq.profile = -1;
      aq.fptr = NULL;
      aq.sampling_freq = AVI_audio_rate(avi);
      aq.chans = AVI_audio_channels(avi);
      aq.config = NULL;
      aq.config_len = 0;
      aq.enabled = 0;
      aq.reference = NULL;
    }
  }

  if (cc_vft != NULL && cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_count, &vq, audio_count, &aq,
			       0, NULL);
  } else {
    if (video_count != 0) vq.enabled = 1;
    if (audio_count != 0) aq.enabled = 1;
  }


  if ((video_count == 0 || vq.enabled == 0) && 
      (audio_count == 0 || aq.enabled == 0)) {
    psptr->set_message("No audio or video tracks enabled or playable");
    AVI_close(avi);
    return -1;
  }
  
  Avifile1 = new CAviFile(name, avi, vq.enabled, audio_count);
  psptr->set_media_close_callback(close_avi_file, Avifile1);

  if (video_count != 0 && vq.enabled) {
    mptr = new CPlayerMedia(psptr, VIDEO_SYNC);
    if (mptr == NULL) {
      return (-1);
    }
  
    video_info_t *vinfo = MALLOC_STRUCTURE(video_info_t);
    if (vinfo == NULL) 
      return (-1);
    vinfo->height = vq.h;
    vinfo->width = vq.w;
    player_debug_message("avi file h %d w %d frame rate %g", 
			 vinfo->height,
			 vinfo->width,
			 vq.frame_rate);

    plugin = check_for_video_codec(STREAM_TYPE_AVI_FILE,
				   codec_name, 
				   NULL,
				   -1,
				   -1,
				   NULL,
				   0,
				   &config);
    int ret;
    ret = mptr->create_video_plugin(plugin,
				    STREAM_TYPE_AVI_FILE,
				    codec_name,
				    -1,
				    -1,
				    NULL,
				    vinfo,
				    NULL,
				    0);
    if (ret < 0) {
      psptr->set_message("Failed to create video plugin %s", 
	       codec_name);
      player_error_message("Failed to create plugin data");
      delete mptr;
      return -1;
    }
    CAviVideoByteStream *vbyte = new CAviVideoByteStream(Avifile1);
    if (vbyte == NULL) {
      delete mptr;
      return (-1);
    }
    vbyte->config(AVI_video_frames(avi), vq.frame_rate);
    ret = mptr->create_media("video", vbyte);
    if (ret != 0) {
      return (-1);
    }
  }
    
  int seekable = 1;
  if (have_audio_driver > 0 && audio_count > 0 && aq.enabled != 0) {
    plugin = check_for_audio_codec(STREAM_TYPE_AVI_FILE,
				   NULL,
				   NULL,
				   aq.type,
				   -1, 
				   NULL, 
				   0,
				   &config);
    CAviAudioByteStream *abyte;
    mptr = new CPlayerMedia(psptr, AUDIO_SYNC);
    if (mptr == NULL) {
      return (-1);
    }
    audio_info_t *ainfo;
    ainfo = MALLOC_STRUCTURE(audio_info_t);
    ainfo->freq = aq.sampling_freq;
    ainfo->chans = aq.chans;
    ainfo->bitspersample = AVI_audio_bits(avi); 

  
    int ret;
    ret = mptr->create_audio_plugin(plugin, 
				    aq.stream_type,
				    aq.compressor,
				    aq.type,
				    aq.profile,
				    NULL, 
				    ainfo,
				    NULL, 
				    0);
    if (ret < 0) {
      delete mptr;
      player_error_message("Couldn't create audio from plugin %s", 
			   plugin->c_name);
      return -1;
    }
    abyte = new CAviAudioByteStream(Avifile1);

    ret = mptr->create_media("audio", abyte);
    if (ret != 0) {
      return (-1);
    }
    seekable = 0;
  } 
  psptr->session_set_seekable(seekable);

  if (audio_count == 0 && have_audio != 0) {
    psptr->set_message("Unknown Audio Codec in avi file ");
    return (1);
  }
  if (video_count != 1) {
    psptr->set_message("Unknown Video Codec %s in avi file",
	     codec_name);
    return (1);
  }
  return (0);
}

CAviFile::CAviFile (const char *name, avi_t *avi,
		    int at, int vt)
{
  m_name = strdup(name);
  m_file = avi;
  m_file_mutex = SDL_CreateMutex();
  m_video_tracks = vt;
  m_audio_tracks = at;
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

/* end file avi_file.cpp */
