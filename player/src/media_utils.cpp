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
 * media_utils.cpp - various utilities, globals (ick).
 */
#include <stdlib.h>
#include <sdp/sdp.h>
#include "player_session.h"
#include "codec/codec.h"
#include "codec/aa/aa.h"
#include "codec/aa/aa_file.h"
#include "codec/mpeg4/mpeg4.h"
#include "codec/mpeg4/mpeg4_file.h"
#include "codec/divx/divx.h"
#include "codec/divx/divx_file.h"
#if 1
#include "codec/mp3/mp3.h"
#include "codec/mp3/mp3_file.h"
#endif
#include "codec/wav/ourwav.h"
#include "codec/wav/wav_file.h"
#include "avi_file.h"
#include "qtime_file.h"
#include "our_config_file.h"
/*
 * This needs to be global so we can store any ports that we don't
 * care about but need to reserve
 */
CIpPort *global_invalid_ports = NULL;

enum {
  VIDEO_MPEG4_ISO,
  VIDEO_DIVX,
  VIDEO_MPEG4_ISO_OR_DIVX,
};

enum {
  AUDIO_AAC,
  AUDIO_MP3,
  AUDIO_WAV,
};
/*
 * these are lists of supported audio and video codecs
 */
static struct codec_list_t {
  const char *name;
  int val;
} video_codecs[] = {
  {"mp4 ", VIDEO_MPEG4_ISO},
  {"mp4v", VIDEO_MPEG4_ISO},
  {"MPG4", VIDEO_MPEG4_ISO},
  {"MP4V-ES", VIDEO_MPEG4_ISO_OR_DIVX},
  {"divx", VIDEO_DIVX},
  {"dvx1", VIDEO_DIVX},
  {NULL, -1},
},
  audio_codecs[] = {
    {"MP-AAC", AUDIO_AAC},
    {"MPA-AAC", AUDIO_AAC},
    {"mp4a", AUDIO_AAC },
    {"aac ", AUDIO_AAC },
#if 1
    {"mp3 ", AUDIO_MP3 },
    {"MPA", AUDIO_MP3 },
#endif
    {"wav ", AUDIO_WAV },
    {NULL, -1},
  };

static int lookup_codec_by_name (const char *name,
				 struct codec_list_t *codec_list, 
				 int *val)
{
  for (struct codec_list_t *cptr = codec_list; cptr->name != NULL; cptr++) {
    if (strcmp(name, cptr->name) == 0) {
      if (val != NULL) *val = cptr->val;
      return (0);
    }
  }
  return (-1);
}
  
int lookup_audio_codec_by_name (const char *name)
{
  return (lookup_codec_by_name(name, audio_codecs, NULL));
}

int lookup_video_codec_by_name (const char *name)
{
  return (lookup_codec_by_name(name, video_codecs, NULL));
}
				
static int sdp_lookup_codec (media_desc_t *media, 
			     struct codec_list_t *codec_list)
{
  for (format_list_t *fptr = media->fmt; fptr != NULL; fptr = fptr->next) {
    if (fptr->rtpmap == NULL || fptr->rtpmap->encode_name == NULL)
      continue;
    int ret = lookup_codec_by_name(fptr->rtpmap->encode_name,
				   codec_list, 
				   NULL);
    if (ret == 0) {
      return (0);
    }
  }
  return (-1);
}

static int sdp_lookup_audio_defaults (media_desc_t *media)
{
  for (format_list_t *fptr = media->fmt; fptr != NULL; fptr = fptr->next) {
    if (strcmp(fptr->fmt, "14") == 0) {
      return (AUDIO_MP3);
    }
  }
  return (-1);
}
/*
 * sdp_is_valid_codec - look and see if a media session has valid codecs
 */
static int sdp_is_valid_codec (media_desc_t *media)
{
  if (strcmp(media->media, "video") == 0) {
    return (sdp_lookup_codec(media, video_codecs));
  }
  if (strcmp(media->media, "audio") == 0) {
    int ret;

    ret = sdp_lookup_audio_defaults(media);
    if (ret > 0) {
      return (ret);
    }
    return (sdp_lookup_codec(media, audio_codecs));
  }
  return (-1);
}

static int create_media_for_streaming_broadcast (CPlayerSession *psptr,
						 const char *name,
						 session_desc_t *sdp,
						 const char **errmsg)
{
  int valid_count = 0;
  int invalid_count = 0;
  int err;
  char buffer[80];
  // need to set range in player session...
  err = psptr->create_streaming_broadcast(sdp, errmsg);
  if (err != 0) {
    return (-1);
  }

  if (sdp->session_name != NULL) {
    snprintf(buffer, sizeof(buffer), "Name: %s", sdp->session_name);
    psptr->set_session_desc(0, buffer);
  }
  if (sdp->session_desc != NULL) {
    snprintf(buffer, sizeof(buffer), "Description: %s", sdp->session_desc);
    psptr->set_session_desc(1, buffer);
  }
  media_desc_t *sdp_media;
  for (sdp_media = psptr->get_sdp_info()->media;
       sdp_media != NULL;
       sdp_media = sdp_media->next) {
    if (sdp_is_valid_codec(sdp_media) >= 0) {
      CPlayerMedia *mptr = new CPlayerMedia;
      err = mptr->create_streaming(psptr, sdp_media, errmsg, 0);
      if (err < 0) {
	return (-1);
      }
      if (err > 0) {
	delete mptr;
      } else 
	valid_count++;
    } else {
      invalid_count++;
    }
  }
  if (valid_count == 0) {
    *errmsg = "No valid codecs";
    return (-1);
  }
  if (invalid_count > 0) {
    *errmsg = "Invalid codec - playing valid ones";
    return (1);
  }
  return (0);
}
/*
 * create_media_for_streaming_ondemand - create streaming media session
 * for an on demand program.
 */
static int create_media_for_streaming_ondemand (CPlayerSession *psptr, 
						const char *name,
						const char **errmsg)
{
  int err;
  session_desc_t *sdp;
  media_desc_t *sdp_media;
  int media_count = 0;
  int invalid_count = 0;
  char buffer[80];
  /*
   * This will open the rtsp session
   */
  err = psptr->create_streaming_ondemand(name, errmsg);
  if (err != 0) {
    return (-1);
  }
  sdp = psptr->get_sdp_info();
  if (sdp->session_name != NULL) {
    snprintf(buffer, sizeof(buffer), "Name: %s", sdp->session_name);
    psptr->set_session_desc(0, buffer);
  }
  if (sdp->session_desc != NULL) {
    snprintf(buffer, sizeof(buffer), "Description: %s", sdp->session_desc);
    psptr->set_session_desc(1, buffer);
  }
  for (sdp_media = psptr->get_sdp_info()->media;
       sdp_media != NULL;
       sdp_media = sdp_media->next) {
    if (sdp_is_valid_codec(sdp_media) >= 0) {
      CPlayerMedia *mptr = new CPlayerMedia;
      err = mptr->create_streaming(psptr, sdp_media, errmsg, 1);
      if (err < 0) {
	return (-1);
      }
      if (err > 0) {
	delete mptr;
      } else 
	media_count++;
    } else 
	invalid_count++;
  }
  if (media_count == 0) {
    *errmsg = "No valid codecs";
    return (-1);
  }
  if (invalid_count > 0) {
    *errmsg = "Invalid codec - playing valid ones";
    return (1);
  }
  return (0);
}

/*
 * create_media_from_sdp_file - create a streaming session based on an
 * sdp file.  It could be either an on-demand program (we look for the 
 * url in the control string), or a broadcast.
 */
static int create_media_from_sdp_file(CPlayerSession *psptr, 
				      const char *name, 
				      const char **errmsg)
{
  sdp_decode_info_t *sdp_info;
  session_desc_t *sdp;
  sdp_info = set_sdp_decode_from_filename(name);
  int translated;


  if (sdp_info == NULL) {
    *errmsg = "SDP error";
    return (-1);
  }

  if (sdp_decode(sdp_info, &sdp, &translated) != 0) {
    *errmsg = "Invalid SDP file";
    return (-1);
  }

  if (translated != 1) {
    *errmsg = "More than 1 program described in SDP";
    return (-1);
  }

  int err;
  if (sdp->control_string != NULL) {
    // An on demand file... Just use the URL...
    err = create_media_for_streaming_ondemand(psptr,
						  sdp->control_string,
						  errmsg);
    free_session_desc(sdp);
    free(sdp_info);
    return (err);
  }
  return (create_media_for_streaming_broadcast(psptr,
					       name, 
					       sdp,
					       errmsg));
}

/*
 * parse_name_for_session - look at the name, determine what routine to 
 * call to set up the session.  This should be redone with plugins at
 * some point.
 */
int parse_name_for_session (CPlayerSession *psptr,
			    const char *name,
			    const char **errmsg)
{
  int err;
  if (strncmp(name, "rtsp://", strlen("rtsp://")) == 0) {    
    err = create_media_for_streaming_ondemand(psptr, name, errmsg);
    return (err);
  }
#ifndef _WINDOWS
  struct stat statbuf;
  if (stat(name, &statbuf) != 0) {
    *errmsg = "File not found";
    return (-1);
  }
  if (!S_ISREG(statbuf.st_mode)) {
    *errmsg = "Not a file";
    return (-1);
  }
#else
  OFSTRUCT ReOpenBuff;
  if (OpenFile(name, &ReOpenBuff,OF_READ) == HFILE_ERROR) {
	  *errmsg = "File not found";
		return (-1);
  }

#endif

  if (strstr(name, ".sdp") != NULL) {
    err = create_media_from_sdp_file(psptr, name, errmsg);
    return (err);
  } else if (strstr(name, ".aac") != NULL) {
    err = create_media_for_aac_file(psptr, name, errmsg);
    return (err);
  } else if ((strstr(name, ".mp4v") != NULL) ||
	     (strstr(name, ".cmp") != NULL)) {
    err = create_media_for_mpeg4_file(psptr, name, errmsg);
    return (err);
  } else if ((strstr(name, ".mov") != NULL) ||
	     (strstr(name, ".mp4") != NULL)) {
    err = create_media_for_qtime_file(psptr, name, errmsg);
    return (err);
  } else if (strstr(name, ".divx") != NULL) {
    err = create_media_for_divx_file(psptr, name, errmsg);
    return (err);
  } else if ((strstr(name, ".wav") != NULL) ||
	     (strstr(name, ".WAV") != NULL)) {
    err = create_media_for_wav_file(psptr, name, errmsg);
    return (err);
  } else if (strstr(name, ".mp3") != NULL) {
    player_debug_message("starting %s", name);
    err = create_media_for_mp3_file(psptr, name, errmsg);
    return (err);
#if 1
  } else if (strstr(name, ".avi") != NULL) {
    err = create_media_for_avi_file(psptr, name, errmsg);
    return (err);
#endif
  }
  *errmsg = "Illegal or unknown file type";
  player_error_message("Illegal or unknown file type - %s", name);
  return (-1);
}

/*
 * start_audio_codec - start up the codec based on the codec_name and
 * the rest of the parameters.
 */
CCodecBase *start_audio_codec (const char *codec_name,
			       CAudioSync *audio_sync,
			       CInByteStreamBase *pbytestream,
			       format_list_t *media_fmt,
			       audio_info_t *aud,
			       const unsigned char *userdata,
			       size_t userdata_size)
{
  if (codec_name == NULL) {
    return (NULL);
  }
  int val;

  player_debug_message("audio codec %s", codec_name);
  if (lookup_codec_by_name(codec_name, audio_codecs, &val) == 0) {
    if (val == AUDIO_AAC) 
      return (new CAACodec(audio_sync,
			   pbytestream,
			   media_fmt,
			   aud,
			   userdata,
			   userdata_size));
    if (val == AUDIO_WAV) 
      return (new CWavCodec(audio_sync,
			    pbytestream,
			    media_fmt,
			    aud,
			    userdata,
			    userdata_size));
#if 1
    if (val == AUDIO_MP3)
      return (new CMP3Codec(audio_sync,
			    pbytestream,
			    media_fmt,
			    aud, 
			    userdata, 
			    userdata_size));
#endif
  }
  return (NULL);
}

static const char *profile_tag="profile-level-id=";

int which_mpeg4_codec (format_list_t *fptr,
		       const unsigned char *userdata,
		       size_t userdata_size)
{
  
  if (fptr && fptr->fmt_param) {
    char *config = strstr(fptr->fmt_param, profile_tag);
    if (config != NULL) {
      config += strlen(profile_tag);
      int profile_value = -1;
      sscanf(config, "%d", &profile_value);
      // check for profile tag value - if it's simple, use DIVX
      if (profile_value > 0 && profile_value <= 3) 
	return (VIDEO_DIVX);
    }
  }
  return (VIDEO_MPEG4_ISO);
}
/*
 * start_video_codec - start up the codec based on the codec_name and
 * the rest of the parameters.
 */
CCodecBase *start_video_codec (const char *codec_name,
			       CVideoSync *video_sync,
			       CInByteStreamBase *pbytestream,
			       format_list_t *media_fmt,
			       video_info_t *vid,
			       const unsigned char *userdata,
			       size_t userdata_size)
{
  if (codec_name == NULL) {
    return (NULL);
  }
  int val;

  if (lookup_codec_by_name(codec_name, video_codecs, &val) == 0) {
    if (val == VIDEO_MPEG4_ISO_OR_DIVX) {
      if (config.get_config_value(CONFIG_USE_MPEG4_ISO_ONLY))
	val = VIDEO_MPEG4_ISO;
      else
	val = which_mpeg4_codec(media_fmt, userdata, userdata_size);
    }
    if (val == VIDEO_MPEG4_ISO) {
      player_debug_message("Starting MPEG4 iso reference codec");
      return (new CMpeg4Codec(video_sync, 
			      pbytestream, 
			      media_fmt, 
			      vid, 
			      userdata, 
			      userdata_size));
    }
    if (val == VIDEO_DIVX) {
      player_debug_message("Starting MPEG4 divx codec");
      return (new CDivxCodec(video_sync,
			     pbytestream,
			     media_fmt, 
			     vid,
			     userdata, 
			     userdata_size));
    }
  }
  return (NULL);
}
  
