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
#include "libmpeg3.h"
#include "mpeg3_file.h"
#include "mpeg3_bytestream.h"
#include "codec_plugin_private.h"

static int create_mpeg3_video (video_query_t *vq,
			       mpeg3_t *vfile, 
			       CPlayerSession *psptr,
			       char *errmsg, 
			       uint32_t errlen, 
			       int &sdesc)
{
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int ret;
#ifdef ARCH_X86
  //  mpeg3_set_mmx(vfile, 1);
#endif
  plugin = check_for_video_codec("MPEG FILE",
				 NULL,
				 -1,
				 -1,
				 NULL,
				 0);
  if (plugin == NULL) {
    snprintf(errmsg, errlen, "Can't find plugin for mpeg video");
    return 0;
  } 
  mptr = new CPlayerMedia(psptr);
  if (mptr == NULL) {
    snprintf(errmsg, errlen, "Could not create video media");
    return -1;
  }
  video_info_t *vinfo;
  vinfo = MALLOC_STRUCTURE(video_info_t);
  vinfo->height = vq->h;
  vinfo->width = vq->w;

  char buffer[80];
  int bitrate;
  ret = snprintf(buffer, 80, "MPEG-%d Video, %d x %d, %g",
		 mpeg3_video_layer(vfile, vq->track_id),
		 vinfo->width, vinfo->height, vq->frame_rate);
  bitrate = (int)(mpeg3_video_bitrate(vfile, vq->track_id) / 1000.0);
  if (bitrate > 0) {
    snprintf(buffer + ret, 80 - ret, ", %d kbps", bitrate);
  }
  psptr->set_session_desc(sdesc, buffer);
  sdesc++;
  mpeg3f_message(LOG_DEBUG, "video stream h %d w %d fr %g bitr %d", 
		 vinfo->height, vinfo->width, vq->frame_rate,
		 bitrate);
  ret = mptr->create_video_plugin(plugin, "MPEG FILE", 
				  vq->type, vq->profile, 
				  NULL, vinfo, NULL, 0);
  if (ret < 0) {
    mpeg3f_message(LOG_ERR, "Failed to create video plugin");
    snprintf(errmsg, errlen, "Failed to create video plugin");
    free(vinfo);
    return -1;
  }
  CMpeg3VideoByteStream *vbyte;
  vbyte = new CMpeg3VideoByteStream(vfile, vq->track_id);
  if (vbyte == NULL) {
    snprintf(errmsg, errlen, "Failed to create video bytestream");
    return -1;
  }
  ret = mptr->create(vbyte, TRUE, errmsg, errlen);
  if (ret != 0) {
    snprintf(errmsg, errlen, "Couldn't create video media");
    return -1;
  }
  return 1;
}

static int create_mpeg3_audio (audio_query_t * aq,
			       mpeg3_t *afile, 
			       CPlayerSession *psptr,
			       char *errmsg, 
			       uint32_t errlen,
			       int &sdesc)
{
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int ret;

  plugin = check_for_audio_codec("MPEG FILE",
				 NULL,
				 aq->type,
				 -1,
				 NULL,
				 0);
  if (plugin == NULL) {
    snprintf(errmsg, errlen, "Can't find plugin for mpeg audio format %s",
	     mpeg3_audio_format(afile, aq->track_id));
    return 0;
  } 
  mptr = new CPlayerMedia(psptr);
  if (mptr == NULL) {
    snprintf(errmsg, errlen, "Could not create video media");
    return -1;
  }
  audio_info_t *ainfo;
  ainfo = MALLOC_STRUCTURE(audio_info_t);
  ainfo->freq = aq->sampling_freq;
  ainfo->chans = aq->chans;
  ainfo->bitspersample = 16;

  char buffer[80];
  snprintf(buffer, 80, "%s Audio, %d %s", 
	   mpeg3_audio_format(afile, aq->track_id),
	   ainfo->freq,
	   ainfo->chans == 1 ? "mono" : "stereo");
  psptr->set_session_desc(sdesc, buffer);
  sdesc++;

  ret = mptr->create_audio_plugin(plugin, "MPEG FILE", 
				  aq->type, aq->profile,
				  NULL, ainfo, NULL, 0);
  if (ret < 0) {
    mpeg3f_message(LOG_ERR, "Failed to create audio plugin");
    snprintf(errmsg, errlen, "Failed to create audio plugin");
    free(ainfo);
    delete mptr;
    return -1;
  }
  CMpeg3AudioByteStream *abyte;
  abyte = new CMpeg3AudioByteStream(afile, aq->track_id);
  if (abyte == NULL) {
    snprintf(errmsg, errlen, "Failed to create audio bytestream");
    return -1;
  }
  ret = mptr->create(abyte, FALSE);
  if (ret != 0) {
    snprintf(errmsg, errlen, "Couldn't create audio media");
    return -1;
  }
  return 1;
}

int create_media_for_mpeg_file (CPlayerSession *psptr,
				const char *name,
				char *errmsg, 
				uint32_t errlen,
				int have_audio_driver,
				control_callback_vft_t *cc_vft)
{
  mpeg3_t *file,*newfile;
  int video_streams, audio_streams;
  int video_cnt, audio_cnt;
  int ix;
  codec_plugin_t *plugin;
  int audio_offset;
  int ret;
  int sdesc;

  if (mpeg3_check_sig(name) != 1) {
    snprintf(errmsg, errlen, "file %s is not a valid .mpg file",
	     name);
    return -1;
  }

  file = mpeg3_open(name);
  video_streams = mpeg3_total_vstreams(file);
  audio_streams = mpeg3_total_astreams(file);

  video_cnt = 0;
  if (video_streams > 0) {
    plugin = check_for_video_codec("MPEG FILE",
				   NULL,
				   -1,
				   -1,
				   NULL,
				   0);
    if (plugin != NULL) video_cnt = video_streams;
  }

  for (ix = 0, audio_cnt = 0; ix < audio_streams; ix++) {
    plugin = check_for_audio_codec("MPEG FILE",
				   NULL,
				   mpeg3_get_audio_format(file, ix),
				   -1,
				   NULL,
				   0);
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
    
  for (ix = 0; ix < video_cnt; ix++) {
    vq[ix].track_id = ix;
    vq[ix].compressor = "MPEG FILE";
    vq[ix].type = mpeg3_video_layer(file, ix);
    vq[ix].profile = -1;
    vq[ix].fptr = NULL;
    vq[ix].h = mpeg3_video_height(file, ix);
    vq[ix].w = mpeg3_video_width(file, ix);
    vq[ix].frame_rate = mpeg3_frame_rate(file, ix);
    vq[ix].config = NULL;
    vq[ix].config_len = 0;
    vq[ix].enabled = 0;
    vq[ix].reference = NULL;
  }
  audio_offset = 0;
  if (have_audio_driver) {
    for (ix = 0; ix < audio_streams; ix++) {
      plugin = check_for_audio_codec("MPEG FILE",
				     NULL,
				     mpeg3_get_audio_format(file, ix),
				     -1,
				     NULL,
				     0);
      if (plugin != NULL) {
	aq[audio_offset].track_id = ix;
	aq[audio_offset].compressor = "MPEG FILE";
	aq[audio_offset].type = mpeg3_get_audio_format(file, ix);
	aq[audio_offset].profile = -1;
	aq[audio_offset].fptr = NULL;
	aq[audio_offset].config = NULL;
	aq[audio_offset].config_len = 0;
	aq[audio_offset].sampling_freq = mpeg3_sample_rate(file, ix);
	aq[audio_offset].chans = mpeg3_audio_channels(file, ix);
	aq[audio_offset].enabled = 0;
	aq[audio_offset].reference = NULL;
	audio_offset++;
      } else {
	mpeg3f_message(LOG_ERR, "Unsupported audio type %s in track %d", 
		       mpeg3_audio_format(file, ix), ix);
      }
    }
  }

  if (cc_vft && cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_cnt, vq, audio_offset, aq);
  } else {
    if (video_cnt > 0) vq[0].enabled = 1;
    if (audio_offset > 0) aq[0].enabled = 1;
  }

  newfile = file;

  ret = 0;
  sdesc = 1;
  for (ix = 0; ret >= 0 && ix < video_cnt; ix++) {
    if (vq[ix].enabled) {
      if (newfile == NULL) {
	newfile = mpeg3_open_copy(name, file);
      }
      ret = create_mpeg3_video(&vq[ix], newfile, psptr, errmsg, errlen, sdesc);
      if (ret <= 0) {
	if (newfile != file) {
	  mpeg3_close(file);
	  newfile = NULL;
	}
      } else {
	newfile = NULL;
      }
    }
  }
  if (ret >= 0) {
    for (ix = 0; ix < audio_offset && ret >= 0; ix++) {
      if (aq[ix].enabled) {
	if (newfile == NULL) {
	  newfile = mpeg3_open_copy(name, file);
	}
	ret = create_mpeg3_audio(&aq[ix], newfile, psptr, errmsg, errlen,sdesc);
	if (ret <= 0) {
	  if (newfile != file) {
	    mpeg3_close(file);
	    newfile = NULL;
	  }
	} else {
	  newfile = NULL;
	}
      }
    }
  }

  free(vq);
  free(aq);
  if (ret < 0) return ret;
  psptr->session_set_seekable(1);
  return 0;
}
