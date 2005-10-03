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
#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "media_utils.h"
#include "mp4/quicktime.h"
#include "qtime_bytestream.h"
#include "qtime_file.h"
#include <mp4util/mpeg4_audio_config.h>
#include "our_config_file.h"
#include "codec_plugin.h"
#include "codec_plugin_private.h"
#include <mp4v2/mp4.h>

static void close_qt_file (void *data)
{
  CQtimeFile *QTfile1 = (CQtimeFile *)data;
  if (QTfile1 != NULL) {
    delete QTfile1;
    QTfile1 = NULL;
  }
}
/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
int create_media_for_qtime_file (CPlayerSession *psptr, 
				 const char *name,
				 int have_audio_driver)
{
  CQtimeFile *QTfile1;
  if (quicktime_check_sig(name) == 0) {
    psptr->set_message("File %s is not a quicktime file", name);
    player_error_message("%s", psptr->get_message());
    return (-1);
  }
 

  QTfile1 = new CQtimeFile(name);
  // quicktime is searchable...
  psptr->set_media_close_callback(close_qt_file, QTfile1);
  psptr->session_set_seekable(1);

  int video;
  video = QTfile1->create_video(psptr);
  if (video < 0) {
    psptr->set_message("Internal quicktime video error");
    return (-1);
  }
  int audio = 0;
  if (have_audio_driver > 0) {
    audio = QTfile1->create_audio(psptr);
    if (audio < 0) {
      psptr->set_message("Internal quicktime audio error");
      return (-1);
    }
  }
  if (audio == 0 && video == 0) {
    psptr->set_message("No valid codecs in file %s", name);
    return (-1);
  }
  if (audio == 0 && QTfile1->get_audio_tracks() > 0) {
    if (have_audio_driver > 0) 
      psptr->set_message("Invalid Audio Codec");
    else 
      psptr->set_message("No Audio driver - no sound");
    return (1);
  }
  if ((QTfile1->get_video_tracks() != 0) && video == 0) {
    psptr->set_message("Invalid Video Codec");
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
  codec_plugin_t *plugin;
  int ret;

  int vid_cnt = 0;
  m_video_tracks = quicktime_video_tracks(m_qtfile);
  player_debug_message("qtime video tracks %d", m_video_tracks);
  for (int ix = 0; ix < m_video_tracks && vid_cnt == 0; ix++) { 
    video_info_t *vinfo;
    const char *codec_name = quicktime_video_compressor(m_qtfile, ix);
    if (codec_name == NULL) 
      continue;
    int profileID = -1;
    int type = -1;

    if (strcasestr(m_name, ".mp4") != NULL && 
	(strcasecmp(codec_name, "mp4v") == 0 
	 || strcasecmp(codec_name, "encv") == 0)) {
      profileID = quicktime_get_iod_video_profile_level(m_qtfile);
      codec_name = "mpeg4";
      type = MP4_MPEG4_VIDEO_TYPE;
    }

    plugin = check_for_video_codec(STREAM_TYPE_QT_FILE,
				   codec_name,
				   NULL, 
				   type,
				   profileID,
				   NULL,
				   0,
				   &config);
    if (plugin == NULL) {
      player_debug_message("Couldn't find video codec %s", codec_name);
      continue;
    }
    mptr = new CPlayerMedia(psptr, VIDEO_SYNC);
    if (mptr == NULL) {
      return (-1);
    }
    
    /*
     * Set up vinfo structure
     */
    vinfo = (video_info_t *)malloc(sizeof(video_info_t));
    if (vinfo == NULL) 
      return (-1);
    vinfo->height = quicktime_video_height(m_qtfile, ix);
    vinfo->width = quicktime_video_width(m_qtfile, ix);
    player_debug_message("qtime file h %d w %d frame rate %g", 
			 vinfo->height,
			 vinfo->width,
			 quicktime_video_frame_rate(m_qtfile, ix));

    /*
     * See if there is userdata
     */
    unsigned char *foo;
    int bufsize;
#undef SORENSON
#ifdef SORENSON
    bufsize = 0;
    ret = quicktime_video_sequence_header(m_qtfile, ix, NULL, &length);
    if (ret < 0) {
      bufsize = -ret;
      uint8_t *foo = (uint8_t *)malloc(bufsize);
      int ret2 = quicktime_video_sequence_header(m_qtfile, ix, foo, &bufsize);
      if (ret2 != 1) {
	player_debug_message("Weird error here %d %d", ret, ret2);
	return (-1);
      }
    }
#else
    ret = quicktime_get_mp4_video_decoder_config(m_qtfile, ix, &foo, &bufsize);
#endif
    /*
     * Create plugin
     */
    ret = mptr->create_video_plugin(plugin, 
				    STREAM_TYPE_QT_FILE,
				    codec_name, 
				    -1,
				    -1,
				    NULL,
				    vinfo,
				    (uint8_t *)foo,
				    bufsize);
    /*
     * Create bytestream
     */
    CQTVideoByteStream *vbyte;
    vbyte = new CQTVideoByteStream(this, ix);
    if (vbyte == NULL) {
      return (-1);
    }
    player_debug_message("qt file length %ld", quicktime_video_length(m_qtfile, ix));
    player_debug_message("qt video time scale %d", quicktime_video_time_scale(m_qtfile, ix));
    vbyte->config(quicktime_video_length(m_qtfile, ix),
		  quicktime_video_frame_rate(m_qtfile, ix),
		  quicktime_video_time_scale(m_qtfile, ix));
    player_debug_message("Video Max time is %g", vbyte->get_max_playtime());

    ret = mptr->create_media("video", vbyte);
    if (ret != 0) {
      return (-1);
    }
    vid_cnt++;

  }

  return (vid_cnt);
}

int CQtimeFile::create_audio (CPlayerSession *psptr)
{
  CPlayerMedia *mptr;
  uint8_t *ud;
  int udsize, ret;
  long sample_rate;
  int samples_per_frame;
  codec_plugin_t *plugin = NULL;

  m_audio_tracks = quicktime_audio_tracks(m_qtfile);
  if (m_audio_tracks > 0) {
    player_debug_message("qtime audio tracks %d", m_audio_tracks);
    const char *codec = quicktime_audio_compressor(m_qtfile, 0);
    if (codec == NULL)
      return (0);

    ud = NULL;
    udsize = 0;
    ret = quicktime_get_mp4_audio_decoder_config(m_qtfile, 0, (unsigned char **)&ud, &udsize);

    plugin = check_for_audio_codec(STREAM_TYPE_QT_FILE, codec, NULL, -1, 
				   -1, ud, udsize, &config);
    if (plugin == NULL) {
      player_debug_message("Couldn't find audio codec %s", codec);
      return (0);
    }
    if (ret >= 0 && ud != NULL) {
      mpeg4_audio_config_t audio_config;

      decode_mpeg4_audio_config(ud, udsize, &audio_config, false);

      sample_rate = audio_config.frequency;
      if (audio_object_type_is_aac(&audio_config) != 0) {
	samples_per_frame = audio_config.codec.aac.frame_len_1024 == 0 ? 
	  960 : 1024;
      } else {
	player_debug_message("Unknown audio codec type %d", audio_config.audio_object_type);
	return (0);
      }
      player_debug_message("From audio rate %ld samples %d", sample_rate, samples_per_frame);
    } else {
      sample_rate = quicktime_audio_sample_rate(m_qtfile, 0);
      samples_per_frame = quicktime_audio_sample_duration(m_qtfile, 0);
      player_debug_message("audio - rate %ld samples %d", 
			   sample_rate, samples_per_frame);
    }
    CQTAudioByteStream *abyte;
    mptr = new CPlayerMedia(psptr, AUDIO_SYNC);
    if (mptr == NULL) {
      return (-1);
    }

    audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
    memset(audio, 0, sizeof(*audio));
    audio->freq = sample_rate;

    int ret = mptr->create_audio_plugin(plugin,
				        STREAM_TYPE_QT_FILE,
					codec, 
					-1, 
					-1,
					NULL, // SDP info
					audio,
					ud,
					udsize);
    if (ret < 0) {
      delete mptr;
      return 0;
    }
    abyte = new CQTAudioByteStream(this, 0);
    long len = quicktime_audio_length(m_qtfile, 0);
    abyte->config(len, sample_rate, samples_per_frame);
    player_debug_message("audio Max time is %g len %ld", 
			 abyte->get_max_playtime(), len);
    ret = mptr->create_media("audio", abyte);
    if (ret != 0) {
      return (-1);
    }
    return (1);
  }
  return (0);
}

/* end file qtime_file.cpp */
