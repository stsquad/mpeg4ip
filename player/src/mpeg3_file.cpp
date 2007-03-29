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
#include "mpeg3_file.h"
#include "mpeg3_bytestream.h"
#include "codec_plugin_private.h"
#include "our_config_file.h"

static void close_mpeg3_file (void *data)
{
  mpeg2ps_t *ps = (mpeg2ps_t *)data;
  mpeg2ps_close(ps);
}

static int create_mpeg3_video (video_query_t *vq,
			       mpeg2ps_t *vfile, 
			       CPlayerSession *psptr,
			       int &sdesc)
{
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int ret;

  plugin = check_for_video_codec(STREAM_TYPE_MPEG_FILE,
				 "mp2v",
				 NULL,
				 vq->type,
				 -1,
				 NULL,
				 0,
				 &config);
  if (plugin == NULL) {
    psptr->set_message("Can't find plugin for mpeg video");
    return 0;
  } 
  mptr = new CPlayerMedia(psptr, VIDEO_SYNC);
  if (mptr == NULL) {
    psptr->set_message("Could not create video media");
    return -1;
  }
  video_info_t *vinfo;
  vinfo = MALLOC_STRUCTURE(video_info_t);
  vinfo->height = vq->h;
  vinfo->width = vq->w;

  char buffer[80];
  int bitrate;
  char *name = mpeg2ps_get_video_stream_name(vfile, vq->track_id);
  ret = snprintf(buffer, 80, "%s Video, %d x %d",
		 name,
		 vinfo->width, vinfo->height);
  free(name);
  if (vq->frame_rate != 0.0) {
    ret += snprintf(buffer + ret, 80 - ret, ", %g", vq->frame_rate);
  }
  bitrate = 
    (int)(mpeg2ps_get_video_stream_bitrate(vfile, vq->track_id) / 1000.0);
  if (bitrate > 0) {
    snprintf(buffer + ret, 80 - ret, ", %d kbps", bitrate);
  }
  psptr->set_session_desc(sdesc, buffer);
  sdesc++;
  mpeg3f_message(LOG_DEBUG, "video stream h %d w %d fr %g bitr %d", 
		 vinfo->height, vinfo->width, vq->frame_rate,
		 bitrate);
  ret = mptr->create_video_plugin(plugin, STREAM_TYPE_MPEG_FILE,
				  vq->compressor,
				  vq->type, vq->profile, 
				  NULL, vinfo, NULL, 0);
  if (ret < 0) {
    mpeg3f_message(LOG_ERR, "Failed to create video plugin");
    psptr->set_message("Failed to create video plugin");
    free(vinfo);
    return -1;
  }
  CMpeg3VideoByteStream *vbyte;
  vbyte = new CMpeg3VideoByteStream(vfile, vq->track_id);
  if (vbyte == NULL) {
    psptr->set_message("Failed to create video bytestream");
    return -1;
  }
  ret = mptr->create_media("video", vbyte);
  if (ret != 0) {
    psptr->set_message("Couldn't create video media");
    return -1;
  }
  return 1;
}

static int create_mpeg3_audio (audio_query_t * aq,
			       mpeg2ps_t *afile, 
			       CPlayerSession *psptr,
			       int &sdesc)
{
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int ret;

  plugin = check_for_audio_codec(STREAM_TYPE_MPEG_FILE,
				 NULL,
				 NULL,
				 aq->type,
				 -1,
				 NULL,
				 0,
				 &config);
  if (plugin == NULL) {
    psptr->set_message("Can't find plugin for mpeg audio format %s",
	     mpeg2ps_get_audio_stream_name(afile, aq->track_id));
    return 0;
  } 
  mptr = new CPlayerMedia(psptr, AUDIO_SYNC);
  if (mptr == NULL) {
    psptr->set_message("Could not create video media");
    return -1;
  }
  audio_info_t *ainfo;
  ainfo = MALLOC_STRUCTURE(audio_info_t);
  ainfo->freq = aq->sampling_freq;
  ainfo->chans = aq->chans;
  ainfo->bitspersample = 16;

  char buffer[80];
  snprintf(buffer, 80, "%s Audio, %d, %d channels", 
	   mpeg2ps_get_audio_stream_name(afile, aq->track_id),
	   ainfo->freq,
	   ainfo->chans);
  psptr->set_session_desc(sdesc, buffer);
  sdesc++;

  ret = mptr->create_audio_plugin(plugin, aq->stream_type,
				  aq->compressor,
				  aq->type, aq->profile,
				  NULL, ainfo, NULL, 0);
  if (ret < 0) {
    mpeg3f_message(LOG_ERR, "Failed to create audio plugin");
    psptr->set_message("Failed to create audio plugin");
    free(ainfo);
    delete mptr;
    return -1;
  }
  CMpeg3AudioByteStream *abyte;
  abyte = new CMpeg3AudioByteStream(afile, aq->track_id);
  if (abyte == NULL) {
    psptr->set_message("Failed to create audio bytestream");
    return -1;
  }
  ret = mptr->create_media("audio", abyte);
  if (ret != 0) {
    psptr->set_message("Couldn't create audio media");
    return -1;
  }
  return 1;
}

int create_media_for_mpeg_file (CPlayerSession *psptr,
				const char *name,
				int have_audio_driver,
				control_callback_vft_t *cc_vft)
{
  mpeg2ps_t *file;
  int video_streams, audio_streams;
  int video_cnt, audio_cnt;
  int ix;
  codec_plugin_t *plugin;
  int video_offset, audio_offset;
  int ret;
  int sdesc;

  file = mpeg2ps_init(name);
  if (file == NULL) {
    psptr->set_message("file %s is not a valid .mpg file",
	     name);
    return -1;
  }

  psptr->set_media_close_callback(close_mpeg3_file, (void *)file);
  video_streams = mpeg2ps_get_video_stream_count(file);
  audio_streams = mpeg2ps_get_audio_stream_count(file);

  video_cnt = 0;
  if (video_streams > 0) {
    plugin = check_for_video_codec(STREAM_TYPE_MPEG_FILE,
				   "mp2v",
				   NULL,
				   mpeg2ps_get_video_stream_type(file, 0),
				   -1,
				   NULL,
				   0,
				   &config);
    if (plugin != NULL) video_cnt = video_streams;
  }

  for (ix = 0, audio_cnt = 0; ix < audio_streams; ix++) {
    plugin = check_for_audio_codec(STREAM_TYPE_MPEG_FILE,
				   NULL,
				   NULL,
				   mpeg2ps_get_audio_stream_type(file, ix),
				   -1,
				   NULL,
				   0,
				   &config);
    if (plugin != NULL) audio_cnt++;
  }
    
  video_query_t *vq;
  audio_query_t *aq;

  if (video_cnt > 0) {
    vq = (video_query_t *)malloc(sizeof(video_query_t) * video_cnt);
  } else {
    vq = NULL;
  }
  if (have_audio_driver && audio_cnt > 0) {
    aq = (audio_query_t *)malloc(sizeof(audio_query_t) * audio_cnt);
  } else {
    aq = NULL;
  }
  video_offset = 0;
  for (ix = 0; ix < video_cnt; ix++) {
    vq[video_offset].track_id = ix;
    vq[video_offset].stream_type = STREAM_TYPE_MPEG_FILE;
    vq[video_offset].compressor = "mp2v";
    vq[video_offset].type = mpeg2ps_get_video_stream_type(file, ix);
    vq[video_offset].profile = -1;
    vq[video_offset].fptr = NULL;
    vq[video_offset].h = mpeg2ps_get_video_stream_height(file, ix);
    vq[video_offset].w = mpeg2ps_get_video_stream_width(file, ix);
    vq[video_offset].frame_rate = mpeg2ps_get_video_stream_framerate(file, ix);
    vq[video_offset].config = NULL;
    vq[video_offset].config_len = 0;
    vq[video_offset].enabled = 0;
    vq[video_offset].reference = NULL;
    video_offset++;
  }
  audio_offset = 0;
  if (have_audio_driver) {
    for (ix = 0; ix < audio_streams; ix++) {
      plugin = check_for_audio_codec(STREAM_TYPE_MPEG_FILE,
				     NULL,
				     NULL,
				     mpeg2ps_get_audio_stream_type(file, ix),
				     -1,
				     NULL,
				     0,
				     &config);
      if (plugin != NULL) {
	aq[audio_offset].track_id = ix;
	aq[audio_offset].stream_type = STREAM_TYPE_MPEG_FILE;
	aq[audio_offset].compressor = NULL;
	aq[audio_offset].type = mpeg2ps_get_audio_stream_type(file, ix);
	aq[audio_offset].profile = -1;
	aq[audio_offset].fptr = NULL;
	aq[audio_offset].config = NULL;
	aq[audio_offset].config_len = 0;
	aq[audio_offset].sampling_freq = 
	  mpeg2ps_get_audio_stream_sample_freq(file, ix);
	aq[audio_offset].chans = mpeg2ps_get_audio_stream_channels(file, ix);
	aq[audio_offset].enabled = 0;
	aq[audio_offset].reference = NULL;
	audio_offset++;
      } else {
	mpeg3f_message(LOG_ERR, "Unsupported audio type %s in track %d", 
		       mpeg2ps_get_audio_stream_name(file, ix), ix);
      }
    }
  }

  if (audio_offset == 0 && video_offset == 0) {
    psptr->set_message("No playable streams in file");
    CHECK_AND_FREE(aq);
    CHECK_AND_FREE(vq);
    return -1;
  }
  if (cc_vft && cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_offset, vq, audio_offset, aq,
			       0, NULL);
  } else {
    if (video_offset > 0) vq[0].enabled = 1;
    if (audio_offset > 0) aq[0].enabled = 1;
  }

  ret = 0;
  sdesc = 1;
  for (ix = 0; ret >= 0 && ix < video_offset; ix++) {
    if (vq[ix].enabled) {
      ret = create_mpeg3_video(&vq[ix], file, psptr, sdesc);
      if (ret <= 0) {
      } 
    }
  }
  if (ret >= 0) {
    for (ix = 0; ix < audio_offset && ret >= 0; ix++) {
      if (aq[ix].enabled) {
	ret = create_mpeg3_audio(&aq[ix], file, psptr, sdesc);
	if (ret <= 0) {
	} 
      }
    }
  }

  free(vq);
  free(aq);
  if (ret < 0) {
    mpeg2ps_close(file);
    return ret;
  }
  psptr->session_set_seekable(1);
  return 0;
}
