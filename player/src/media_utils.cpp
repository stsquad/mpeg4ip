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
#include "systems.h"
#include <sdp/sdp.h>
#include <libhttp/http.h>
#include "media_utils.h"
#include "player_session.h"
#include "player_media.h"
#include "avi_file.h"
#include "mp4_file.h"
#include "qtime_file.h"
#include "our_config_file.h"
#include "rtp_bytestream.h"
#include "codec_plugin_private.h"
#include <gnu/strcasestr.h>
#include "mpeg3_file.h"
#include "audio.h"
#ifndef _WIN32
#include "mpeg2t.h"
#endif
/*
 * This needs to be global so we can store any ports that we don't
 * care about but need to reserve
 */
/*
 * these are lists of supported audio and video codecs
 */
static struct codec_list_t {
  const char *name;
  int val;
} video_codecs[] = {
  {"mp4 ", VIDEO_MPEG4_ISO},
  {"mp4v", VIDEO_MPEG4_ISO},
  {"encv", VIDEO_MPEG4_ISO},
  {"MPG4", VIDEO_MPEG4_ISO},
  {"MP4V-ES", VIDEO_MPEG4_ISO_OR_DIVX},
  {"MPEG4-GENERIC", VIDEO_DIVX},
  {"divx", VIDEO_DIVX},
  {"dvx1", VIDEO_DIVX},
  {"div4", VIDEO_DIVX},
  {NULL, -1},
},
  audio_codecs[] = {
    {"MPA", MPEG4IP_AUDIO_MP3 },
    {"mpa-robust", MPEG4IP_AUDIO_MP3_ROBUST}, 
    {"L16", MPEG4IP_AUDIO_GENERIC },
    {"L8", MPEG4IP_AUDIO_GENERIC },
    {NULL, -1},
  };



static int lookup_codec_by_name (const char *name,
				 struct codec_list_t *codec_list)
{
  for (struct codec_list_t *cptr = codec_list; cptr->name != NULL; cptr++) {
    if (strcasecmp(name, cptr->name) == 0) {
      return (cptr->val);
    }
  }
  return (-1);
}
  
int lookup_audio_codec_by_name (const char *name)
{
  return (lookup_codec_by_name(name, audio_codecs));
}

int lookup_video_codec_by_name (const char *name)
{
  return (lookup_codec_by_name(name, video_codecs));
}
				

static int create_media_from_sdp (CPlayerSession *psptr,
				  const char *name,
				  session_desc_t *sdp,
				  char *errmsg,
				  uint32_t errlen,
				  int have_audio_driver,
				  int broadcast,
				  int only_check_first,
				  control_callback_vft_t *cc_vft)
{
  int err;
  int media_count = 0;
  int invalid_count = 0;
  int have_audio_but_no_driver = 0;
  char buffer[80];
  codec_plugin_t *codec;
  format_list_t *fmt;
  int audio_count, video_count;
  int audio_offset, video_offset;
  int ix;

  if (sdp->session_name != NULL) {
    snprintf(buffer, sizeof(buffer), "Name: %s", sdp->session_name);
    psptr->set_session_desc(0, buffer);
  }
  if (sdp->session_desc != NULL) {
    snprintf(buffer, sizeof(buffer), "Description: %s", sdp->session_desc);
    psptr->set_session_desc(1, buffer);
  }
#ifndef _WIN32
  if (sdp->media != NULL &&
      sdp->media->next == NULL &&
      strcasecmp(sdp->media->media, "video") == 0 &&
      sdp->media->fmt != NULL &&
      strcmp(sdp->media->fmt->fmt, "33") == 0) {
    // we have a mpeg2 transport stream
    return (create_mpeg2t_session(psptr, NULL, sdp, errmsg, errlen,
				  have_audio_driver, cc_vft));
				  
  }
#endif
  media_desc_t *sdp_media;
  audio_count = video_count = 0;
  for (sdp_media = psptr->get_sdp_info()->media;
       sdp_media != NULL;
       sdp_media = sdp_media->next) {
    if (strcasecmp(sdp_media->media, "audio") == 0) {
      if (have_audio_driver == 0) {
	have_audio_but_no_driver = 1;
      } else {
	audio_count++;
      }
    } else if (strcasecmp(sdp_media->media, "video") == 0) {
      video_count++;
    }
  }

  video_query_t *vq;
  audio_query_t *aq;

  if (video_count > 0) {
    vq = (video_query_t *)malloc(sizeof(video_query_t) * video_count);
  } else {
    vq = NULL;
  }
  if (audio_count > 0) {
    aq = (audio_query_t *)malloc(sizeof(audio_query_t) * audio_count);
  } else {
    aq = NULL;
  }
      
  video_offset = audio_offset = 0;
  for (sdp_media = psptr->get_sdp_info()->media;
       sdp_media != NULL;
       sdp_media = sdp_media->next) {

    if (have_audio_driver != 0 &&
	strcasecmp(sdp_media->media, "audio") == 0) {
      fmt = sdp_media->fmt;
      codec = NULL;
      while (codec == NULL && fmt != NULL) {
	codec = check_for_audio_codec(NULL, 
				      fmt,
				      -1,
				      -1,
				      NULL,
				      0);
	if (codec == NULL) {
	  if (only_check_first != 0)
	    fmt = NULL;
	  else
	    fmt = fmt->next;
	}
      }
      if (codec == NULL) {
	invalid_count++;
	continue;
      } else {
	// set up audio qualifier
	aq[audio_offset].track_id = audio_offset;
	aq[audio_offset].compressor = NULL;
	aq[audio_offset].type = -1;
	aq[audio_offset].profile = -1;
	aq[audio_offset].fptr = fmt;
	aq[audio_offset].sampling_freq = -1;
	aq[audio_offset].chans = -1;
	aq[audio_offset].enabled = 0;
	aq[audio_offset].reference = NULL;
	audio_offset++;
      }
    } else if (strcasecmp(sdp_media->media, "video") == 0) {
	fmt = sdp_media->fmt;
	codec = NULL;
	while (codec == NULL && fmt != NULL) {
	  codec = check_for_video_codec(NULL, 
					fmt,
					-1,
					-1,
					NULL,
					0);
	  if (codec == NULL) {
	    if (only_check_first != 0)
	      fmt = NULL;
	    else
	      fmt = fmt->next;
	  }
	}
	if (codec == NULL) {
	  invalid_count++;
	  continue;
	} else {
	  vq[video_offset].track_id = video_offset;
	  vq[video_offset].compressor = NULL;
	  vq[video_offset].type = -1;
	  vq[video_offset].profile = -1;
	  vq[video_offset].fptr = fmt;
	  vq[video_offset].h = -1;
	  vq[video_offset].w = -1;
	  vq[video_offset].frame_rate = -1;
	  vq[video_offset].enabled = 0;
	  vq[video_offset].reference = NULL;
	  video_offset++;
	}
      } else {
	player_error_message("Skipping media type `%s\'", sdp_media->media);
	continue;
      }
    }
  // okay - from here, write the callback call, and go ahead...
  if (cc_vft != NULL &&
      cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_offset, vq, audio_offset, aq);
  } else {
    if (video_offset > 0) {
      vq[0].enabled = 1;
    }
    if (audio_offset > 0) {
      aq[0].enabled = 1;
    }
  }
  for (ix = 0; ix < video_offset; ix++) {
    if (vq[ix].enabled != 0) {
      CPlayerMedia *mptr = new CPlayerMedia(psptr);
      err = mptr->create_streaming(vq[ix].fptr->media, 
				   errmsg, 
				   errlen,
				   broadcast, 
				   config.get_config_value(CONFIG_USE_RTP_OVER_RTSP),
				   media_count);
      if (err < 0) {
	return (-1);
      }
      if (err > 0) {
	delete mptr;
      } else 
	media_count++;
    }
  }
  for (ix = 0; ix < audio_offset; ix++) {
    if (aq[ix].enabled != 0) {
      CPlayerMedia *mptr = new CPlayerMedia(psptr);
      err = mptr->create_streaming(aq[ix].fptr->media, 
				   errmsg, 
				   errlen,
				   broadcast, 
				   config.get_config_value(CONFIG_USE_RTP_OVER_RTSP),
				   media_count);
      if (err < 0) {
	return (-1);
      }
      if (err > 0) {
	delete mptr;
      } else 
	media_count++;
    }
  }
  if (aq != NULL) free(aq);
  if (vq != NULL) free(vq);

  if (media_count == 0) {
    snprintf(errmsg, errlen, "No known codecs found in SDP");
    return (-1);
  }
  psptr->streaming_media_set_up();
  if (have_audio_but_no_driver > 0) {
    snprintf(errmsg, errlen, "Not playing audio codecs - no driver");
    return (1);
  }
  if (invalid_count > 0) {
    snprintf(errmsg, errlen, 
	     "There were unknowns codecs during decode - playing valid ones");
    return (1);
  }
  return (0);
}
static int create_media_for_streaming_broadcast (CPlayerSession *psptr,
						 const char *name,
						 session_desc_t *sdp,
						 char *errmsg,
						 uint32_t errlen,
						 int have_audio_driver,
						 control_callback_vft_t *cc_vft)
{
  int err;
  // need to set range in player session...
  err = psptr->create_streaming_broadcast(sdp, errmsg, errlen);
  if (err != 0) {
    return (-1);
  }
  return (create_media_from_sdp(psptr, 
				name, 
				sdp, 
				errmsg, 
				errlen,
				have_audio_driver, 
				0,
				1,
				cc_vft));
}
/*
 * create_media_for_streaming_ondemand - create streaming media session
 * for an on demand program.
 */
static int create_media_for_streaming_ondemand (CPlayerSession *psptr, 
						const char *name,
						char *errmsg,
						uint32_t errlen,
						control_callback_vft_t *cc_vft)
{
  int err;
  session_desc_t *sdp;
  /*
   * This will open the rtsp session
   */
  err = psptr->create_streaming_ondemand(name, 
					 errmsg,
					 errlen,
					 config.get_config_value(CONFIG_USE_RTP_OVER_RTSP));
  if (err != 0) {
    return (-1);
  }
  sdp = psptr->get_sdp_info();
  int have_audio_driver = do_we_have_audio();
  return (create_media_from_sdp(psptr,
				name, 
				sdp, 
				errmsg,
				errlen,
				have_audio_driver, 
				1,
				0,
				cc_vft));
}

/*
 * create_media_from_sdp_file - create a streaming session based on an
 * sdp file.  It could be either an on-demand program (we look for the 
 * url in the control string), or a broadcast.
 */

static int create_from_sdp (CPlayerSession *psptr,
			    const char *name,
			    char *errmsg,
			    uint32_t errlen,
			    sdp_decode_info_t *sdp_info,
			    int have_audio_driver,
			    control_callback_vft_t *cc_vft) 
{
  session_desc_t *sdp;
  int translated;

  if (sdp_info == NULL) {
    strcpy(errmsg, "SDP error");
    return (-1);
  }

  if (sdp_decode(sdp_info, &sdp, &translated) != 0) {
    snprintf(errmsg, errlen, "Invalid SDP file");
    sdp_decode_info_free(sdp_info);
    return (-1);
  }

  if (translated != 1) {
    snprintf(errmsg, errlen, "More than 1 program described in SDP");
    sdp_decode_info_free(sdp_info);
    return (-1);
  }

  int err;
  if (sdp->control_string != NULL) {
    // An on demand file... Just use the URL...
    err = create_media_for_streaming_ondemand(psptr,
					      sdp->control_string,
					      errmsg,
					      errlen,
					      cc_vft);
    sdp_free_session_desc(sdp);
    sdp_decode_info_free(sdp_info);
    return (err);
  }
  sdp_decode_info_free(sdp_info);
  return (create_media_for_streaming_broadcast(psptr,
					       name, 
					       sdp,
					       errmsg,
					       errlen,
					       have_audio_driver,
					       cc_vft));
}

static int create_media_from_sdp_file(CPlayerSession *psptr, 
				      const char *name, 
				      char *errmsg,
				      uint32_t errlen,
				      int have_audio_driver,
				      control_callback_vft_t *cc_vft)
{
  sdp_decode_info_t *sdp_info;
  sdp_info = set_sdp_decode_from_filename(name);

  return (create_from_sdp(psptr, 
			  name, 
			  errmsg, 
			  errlen, 
			  sdp_info, 
			  have_audio_driver, 
			  cc_vft));
}

static int create_media_for_http (CPlayerSession *psptr,
				  const char *name,
				  char *errmsg,
				  uint32_t errlen,
				  control_callback_vft_t *cc_vft)
{
  http_client_t *http_client;

  int ret;
  http_resp_t *http_resp;
  http_resp = NULL;
  http_client = http_init_connection(name);

  if (http_client == NULL) {
    snprintf(errmsg, errlen, "Can't create http client");
    return -1;
  } 
  ret = http_get(http_client, NULL, &http_resp);
  if (ret > 0) {
    sdp_decode_info_t *sdp_info;

    int have_audio_driver = do_we_have_audio();
    sdp_info = set_sdp_decode_from_memory(http_resp->body);
    ret = create_from_sdp(psptr, 
			  name, 
			  errmsg, 
			  errlen, 
			  sdp_info, 
			  have_audio_driver,
			  cc_vft);
  } else {
    ret = -1;
  }
  http_resp_free(http_resp);
  http_free_connection(http_client);

  return (ret);
}
#ifndef _WIN32
static session_desc_t *find_sdp_for_program (const char *cm, 
					     const char *loc,
					     char *errmsg, 
					     uint32_t errlen,
					     uint64_t prog)
{
  char buffer[1024];
  http_client_t *http_client;
  http_resp_t *http_resp;
  sdp_decode_info_t *sdp_info;
  session_desc_t *sdp, *ptr, *sdp_ret;
  int translated;
  int ret, ix;

  snprintf(buffer, sizeof(buffer), "http://%s/%s", cm, loc);
    http_resp = NULL;
  http_client = http_init_connection(buffer);

  if (http_client == NULL) {
    snprintf(errmsg, errlen, "Cannot create http client with %s\n", 
	     cm);
    return NULL;
  }
  ret = http_get(http_client, NULL, &http_resp);
  
  sdp_ret = NULL;
  if (ret > 0) {
    sdp_info = set_sdp_decode_from_memory(http_resp->body);
    if ((sdp_decode(sdp_info, &sdp, &translated) == 0) &&
	(translated > 0)) {
      for (ix = 0; ix < translated && sdp_ret == NULL; ix++) {
	if (sdp->session_id == prog) {
	  sdp_ret = sdp;
	  sdp = sdp->next;
	  sdp_ret->next = NULL;
	} else {
	  ptr = sdp->next;
	  sdp->next = NULL;
	  sdp_free_session_desc(sdp);
	  sdp = ptr;
	}
      }
      sdp_decode_info_free(sdp_info);
      if (sdp != NULL) sdp_free_session_desc(sdp);
      
    } 
  }
  http_resp_free(http_resp);
  http_free_connection(http_client);
  return sdp_ret;
}

static int create_media_for_iptv (CPlayerSession *psptr,
				  const char *name,
				  char *errmsg,
				  uint32_t errlen,
				  int have_audio_driver,
				  control_callback_vft_t *cc_vft)
{
  char *slash, *cm;
  uint64_t prog;
  session_desc_t *sdp;

  name += strlen("iptv://");
  slash = strchr(name, '/');
  if (slash == NULL || slash == name) {
    snprintf(errmsg, errlen, "Invalid iptv content manager");
    return -1;
  }
  cm = (char *)malloc(slash - name + 1);
  memcpy(cm, name, slash - name);
  cm[slash - name] = '\0';
  slash++;
  if (sscanf(slash, "%llu", &prog) != 1) {
    snprintf(errmsg, errlen, "Invalid iptv program");
    return -1;
  }
  
  // check on-demand first
  sdp = find_sdp_for_program(cm, "iptvfiles/guide.sdf",
			     errmsg, errlen, prog);
  if (sdp == NULL) {
    sdp = find_sdp_for_program(cm, "servlet/OdPublish", 
			       errmsg, errlen, prog);
  }
  free(cm);

  if (sdp == NULL) {
    return -1;
  }
  int err;
  if (sdp->control_string != NULL) {
    // An on demand file... Just use the URL...
    err = create_media_for_streaming_ondemand(psptr,
					      sdp->control_string,
					      errmsg,
					      errlen,
					      cc_vft);
    sdp_free_session_desc(sdp);
    return (err);
  }
  return (create_media_for_streaming_broadcast(psptr,
					       name, 
					       sdp,
					       errmsg,
					       errlen,
					       have_audio_driver,
					       cc_vft));
}  
#endif

/*
 * parse_name_for_session - look at the name, determine what routine to 
 * call to set up the session.  This should be redone with plugins at
 * some point.
 */

int parse_name_for_session (CPlayerSession *psptr,
			    const char *name,
			    char *errmsg,
			    uint32_t errlen,
			    control_callback_vft_t *cc_vft)
{
  int err;
  ADV_SPACE(name);
  if (strncmp(name, "rtsp://", strlen("rtsp://")) == 0) {    
    err = create_media_for_streaming_ondemand(psptr, 
					      name, 
					      errmsg, 
					      errlen,
					      cc_vft);
    return (err);
  }
  if (strncmp(name, "http://", strlen("http://")) == 0) {
    err = create_media_for_http(psptr, name, errmsg, errlen, cc_vft);
    return (err);
  }    

  int have_audio_driver;

  have_audio_driver = do_we_have_audio();
#ifndef _WIN32
  if (strncmp(name, "iptv://", strlen("iptv://")) == 0) {
    err = create_media_for_iptv(psptr, 
				name, 
				errmsg, 
				errlen, 
				have_audio_driver, 
				cc_vft);
    return err;
  }
#endif
#ifndef _WIN32
  if (strncmp(name, "mpeg2t://", strlen("mpeg2t://")) == 0) {
    err = create_mpeg2t_session(psptr, name, NULL, errmsg, errlen, 
				have_audio_driver, cc_vft);
    return (err);
  }

  struct stat statbuf;
  if (stat(name, &statbuf) != 0) {
    snprintf(errmsg, errlen, "File \'%s\' not found", name);
    return (-1);
  }
  if (!S_ISREG(statbuf.st_mode)) {
    snprintf(errmsg, errlen, "File \'%s\' is not a file", name);
    return (-1);
  }
#else
  OFSTRUCT ReOpenBuff;
  if (OpenFile(name, &ReOpenBuff,OF_READ) == HFILE_ERROR) {
    snprintf(errmsg, errlen, "File %s not found", name);
    return (-1);
  }

#endif
  err = -1;

  const char *suffix = strrchr(name, '.');
  if (suffix == NULL) {
    snprintf(errmsg, errlen, "No useable suffix on file %s", name);
    return err;
  }


  if (strcasecmp(suffix, ".sdp") == 0) {
    err = create_media_from_sdp_file(psptr, 
				     name, 
				     errmsg, 
				     errlen, 
				     have_audio_driver,
				     cc_vft);
  } else if ((strcasecmp(suffix, ".mov") == 0) ||
	     ((config.get_config_value(CONFIG_USE_OLD_MP4_LIB) != 0) &&
	      (strcasecmp(suffix, ".mp4") == 0))){
    err = create_media_for_qtime_file(psptr, 
				      name, 
				      errmsg, 
				      errlen,
				      have_audio_driver);
  } else if (strcasecmp(suffix, ".mp4") == 0) {
    err = create_media_for_mp4_file(psptr, 
				    name, 
				    errmsg, 
				    errlen,
				    have_audio_driver,
				    cc_vft);
  } else if (strcasecmp(suffix, ".avi") == 0) {
    err = create_media_for_avi_file(psptr, 
				    name, 
				    errmsg, 
				    errlen, 
				    have_audio_driver,
				    cc_vft);
  } else if (strcasecmp(suffix, ".mpeg") == 0 ||
	     strcasecmp(suffix, ".mpg") == 0) {
    err = create_media_for_mpeg_file(psptr, name, errmsg, 
				     errlen, have_audio_driver, cc_vft);
  } else {
    // raw files
    if (have_audio_driver) {
      err = audio_codec_check_for_raw_file(psptr, name);
    }
    if (err < 0) {
      err = video_codec_check_for_raw_file(psptr, name);
    }
  }
  
  if (err >= 0) {
    const char *temp;
    temp = psptr->get_session_desc(0);
    if (temp == NULL) {
      psptr->set_session_desc(0, name);
    }
  }
  return (err);
}

int check_name_for_network (const char *name, 
			    int &isOnDemand, 
			    int &isRtpOverRtsp)
{
  sdp_decode_info_t *sdp_info;
  session_desc_t *sdp;
  int translated;
  http_resp_t *http_resp;
  int do_sdp = 0;

  http_resp = NULL;
  isOnDemand = 0;
  isRtpOverRtsp = 0;
  sdp_info = NULL;
  if (strncmp(name, "mpeg2t://", strlen("mpeg2t://")) == 0) {
    return 1;
  }
  if (strncmp(name, "iptv://", strlen("iptv://")) == 0) {
    // more later to handle the on demand/streaming case
    return 1;
  }
  if (strncmp(name, "rtsp://", strlen("rtsp://")) == 0) {
    isOnDemand = 1;
    isRtpOverRtsp = config.get_config_value(CONFIG_USE_RTP_OVER_RTSP);
    return 1;
  }
  // handle http, .sdp case
  if (strncmp(name, "http://", strlen("http://")) == 0) {
    http_client_t *http_client;
    int ret;
    http_client = http_init_connection(name);

    if (http_client == NULL) {
      return -1;
    } 
    ret = http_get(http_client, NULL, &http_resp);
    if (ret > 0) {
      sdp_decode_info_t *sdp_info;
      sdp_info = set_sdp_decode_from_memory(http_resp->body);
      do_sdp = 1;
      http_free_connection(http_client);
    } else return -1;
    do_sdp = 1;
  } else {
    const char *suffix = strrchr(name, '.');
    if (suffix == NULL) {
      return -1;
    }
    if (strcasecmp(suffix, ".sdp") == 0) {
      sdp_info = set_sdp_decode_from_filename(name);
      do_sdp = 1;
    } else 
      return 0;
  }
  if (do_sdp != 0) {
    if ((sdp_decode(sdp_info, &sdp, &translated) != 0) ||
	translated != 1){
      sdp_decode_info_free(sdp_info);
      return (-1);
    }
    if (sdp->control_string != NULL) {
      isOnDemand = 1;
      isRtpOverRtsp = config.get_config_value(CONFIG_USE_RTP_OVER_RTSP);
    }
    sdp_free_session_desc(sdp);
    if (http_resp != NULL)
      http_resp_free(http_resp);
    sdp_decode_info_free(sdp_info);
    return 1;
  }
  return 0;
}

/* end file media_utils.cpp */
