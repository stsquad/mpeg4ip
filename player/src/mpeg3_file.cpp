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
#include "libmpeg3.h"
#include "mpeg3_file.h"
#include "mpeg3_bytestream.h"
#include "codec_plugin_private.h"

static int create_mpeg3_video (mpeg3_t *vfile, 
			       CPlayerSession *psptr,
			       char *errmsg, 
			       uint32_t errlen)
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
  vinfo->height = mpeg3_video_height(vfile, 0);
  vinfo->width = mpeg3_video_width(vfile, 0);

  ret = mptr->create_video_plugin(plugin, NULL, vinfo, NULL, 0);
  if (ret < 0) {
    mpeg3f_message(LOG_ERR, "Failed to create video plugin");
    snprintf(errmsg, errlen, "Failed to create video plugin");
    free(vinfo);
    return -1;
  }
  CMpeg3VideoByteStream *vbyte;
  vbyte = new CMpeg3VideoByteStream(vfile, 0);
  if (vbyte == NULL) {
    snprintf(errmsg, errlen, "Failed to create video bytestream");
    return -1;
  }
  ret = mptr->create_from_file(vbyte, TRUE);
  if (ret != 0) {
    snprintf(errmsg, errlen, "Couldn't create video media");
    return -1;
  }
  return 1;
}

static int create_mpeg3_audio (mpeg3_t *afile, 
			       CPlayerSession *psptr,
			       char *errmsg, 
			       uint32_t errlen)
{
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int ret;

  plugin = check_for_audio_codec("MPEG FILE",
				 NULL,
				 mpeg3_get_audio_format(afile, 0),
				 -1,
				 NULL,
				 0);
  if (plugin == NULL) {
    snprintf(errmsg, errlen, "Can't find plugin for mpeg audio format %s",
	     mpeg3_audio_format(afile, 0));
    return 0;
  } 
  mptr = new CPlayerMedia(psptr);
  if (mptr == NULL) {
    snprintf(errmsg, errlen, "Could not create video media");
    return -1;
  }
  audio_info_t *ainfo;
  ainfo = MALLOC_STRUCTURE(audio_info_t);
  ainfo->freq = mpeg3_sample_rate(afile, 0);
  ainfo->chans = mpeg3_audio_channels(afile, 0);
  ainfo->bitspersample = 16;

  ret = mptr->create_audio_plugin(plugin, NULL, ainfo, NULL, 0);
  if (ret < 0) {
    mpeg3f_message(LOG_ERR, "Failed to create audio plugin");
    snprintf(errmsg, errlen, "Failed to create audio plugin");
    free(ainfo);
    delete mptr;
    return -1;
  }
  CMpeg3AudioByteStream *abyte;
  abyte = new CMpeg3AudioByteStream(afile, 0);
  if (abyte == NULL) {
    snprintf(errmsg, errlen, "Failed to create audio bytestream");
    return -1;
  }
  ret = mptr->create_from_file(abyte, FALSE);
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
				int has_audio_driver)
{
  mpeg3_t *file, *vfile, *afile;
  int has_video, has_audio;

  if (mpeg3_check_sig(name) != 1) {
    snprintf(errmsg, errlen, "file %s is not a valid .mpg file",
	     name);
    return -1;
  }

  file = mpeg3_open(name);
  has_video = mpeg3_has_video(file);
  has_audio = mpeg3_has_audio(file);
  if (has_video != 0 && 
      (has_audio != 0 && has_audio_driver)) {
    // need to open both audio and video
    vfile = file;
    afile = mpeg3_open_copy(name, vfile);
  } else if (has_video != 0) {
    vfile = file;
    afile = NULL;
  } else if (has_audio != 0) {
    vfile = NULL;
    afile = file;
  } else {
    snprintf(errmsg, errlen, "Weird error - neither audio nor video");
    return -1;
  }
  if (vfile != NULL) {
    has_video = create_mpeg3_video(vfile, psptr, errmsg, errlen);
    if (has_video <= 0) {
      mpeg3_close(vfile);
    }
    if (has_video < 0) return -1;
  }
  if (afile != NULL) {
    has_audio = create_mpeg3_audio(afile, psptr, errmsg, errlen);
    if (has_audio <= 0) {
      mpeg3_close(afile);
    }
    if (has_audio < 0) return -1;
  }
  psptr->session_set_seekable(1);
  return 0;
}
