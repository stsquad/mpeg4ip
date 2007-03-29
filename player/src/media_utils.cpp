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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * media_utils.cpp - various utilities, globals (ick).
 */
#include "mpeg4ip.h"
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
#include "audio.h"
#ifndef _WIN32
#include "mpeg3_file.h"
#include "mpeg2t.h"
#include "mpeg2t_file.h"
#endif
#include "our_bytestream_file.h"
#include <rtp/net_udp.h>
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
				  session_desc_t *sdp,
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
  uint audio_count, video_count, text_count;
  uint audio_offset, video_offset, text_offset;
  uint ix;

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
      sdp->media->fmt_list != NULL &&
      strcmp(sdp->media->fmt_list->fmt, "33") == 0) {
    // we have a mpeg2 transport stream
    return (create_mpeg2t_session(psptr, NULL, sdp, 
				  have_audio_driver, cc_vft));
				  
  }
#endif
  media_desc_t *sdp_media;
  audio_count = video_count = text_count = 0;
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
    } else if (strcasecmp(sdp_media->media, "control") == 0 ||
	       strcasecmp(sdp_media->media, "application") == 0) {
      text_count++;
    }
  }

  video_query_t *vq;
  audio_query_t *aq;
  text_query_t *tq;

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
  if (text_count > 0) {
    tq = (text_query_t *)malloc(sizeof(text_query_t) * text_count);
  } else {
    tq = NULL;
  }
      
  video_offset = audio_offset = text_offset = 0;
  for (sdp_media = psptr->get_sdp_info()->media;
       sdp_media != NULL;
       sdp_media = sdp_media->next) {

    if (have_audio_driver != 0 &&
	strcasecmp(sdp_media->media, "audio") == 0) {
      fmt = sdp_media->fmt_list;
      codec = NULL;
      while (codec == NULL && fmt != NULL) {
	codec = check_for_audio_codec(STREAM_TYPE_RTP,
				      NULL, 
				      fmt,
				      -1,
				      -1,
				      NULL,
				      0,
				      &config);
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
	aq[audio_offset].stream_type = STREAM_TYPE_RTP;
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
	fmt = sdp_media->fmt_list;
	codec = NULL;
	while (codec == NULL && fmt != NULL) {
	  codec = check_for_video_codec(STREAM_TYPE_RTP,
					NULL, 
					fmt,
					-1,
					-1,
					NULL,
					0,
					&config);
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
	  vq[video_offset].stream_type = STREAM_TYPE_RTP;
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
      } else if (strcasecmp(sdp_media->media, "application") == 0 ||
		 strcasecmp(sdp_media->media, "control") == 0) {
	fmt = sdp_media->fmt_list;
	codec = NULL;
	while (codec == NULL && fmt != NULL) {
	  codec = check_for_text_codec(STREAM_TYPE_RTP, 
				       NULL,
				       fmt, 
				       NULL,
				       0, 
				       &config);
	  if (codec == NULL) {
	    if (only_check_first != 0) {
	      fmt = NULL;
	    } else {
	      fmt = fmt->next;
	    }
	  } else {
	    if (fmt != sdp_media->fmt_list || fmt->next != NULL) {
	      player_error_message("Disallow sdp stream %s - more than 1 possible format", sdp_media->media);
	      codec = NULL;
	      fmt = fmt->next;
	    }
	  }
	}
	if (codec == NULL) {
	  invalid_count++;
	  continue;
	} else {
	  tq[text_offset].track_id = text_offset;
	  tq[text_offset].stream_type = STREAM_TYPE_RTP;
	  tq[text_offset].compressor = NULL;
	  tq[text_offset].fptr = fmt;
	  tq[text_offset].config = NULL;
	  tq[text_offset].config_len = 0;
	  tq[text_offset].enabled = 0;
	  tq[text_offset].reference = NULL;
	  text_offset++;
	}
      } else {
	player_error_message("Skipping media type `%s\'", sdp_media->media);
	continue;
      }
    }
  // okay - from here, write the callback call, and go ahead...
  if (cc_vft != NULL &&
      cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, 
			       video_offset, vq, 
			       audio_offset, aq,
			       text_offset, tq);
  } else {
    if (video_offset > 0) {
      vq[0].enabled = 1;
    }
    if (audio_offset > 0) {
      aq[0].enabled = 1;
    }
    if (text_offset > 0) {
      tq[0].enabled = 1;
    }
  }
  for (ix = 0; ix < video_offset; ix++) {
    if (vq[ix].enabled != 0) {
      CPlayerMedia *mptr = new CPlayerMedia(psptr, VIDEO_SYNC);
      err = mptr->create_streaming(vq[ix].fptr->media, 
				   broadcast, 
				   config.get_config_value(CONFIG_USE_RTP_OVER_RTSP),
				   media_count, 
				   ix == 0 ? psptr->get_video_rtp_session() : NULL);
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
      CPlayerMedia *mptr = new CPlayerMedia(psptr, AUDIO_SYNC);
      err = mptr->create_streaming(aq[ix].fptr->media, 
				   broadcast, 
				   config.get_config_value(CONFIG_USE_RTP_OVER_RTSP),
				   media_count,
				   ix == 0 ? psptr->get_audio_rtp_session() : NULL);
      if (err < 0) {
	return (-1);
      }
      if (err > 0) {
	delete mptr;
      } else 
	media_count++;
    }
  }
  for (ix = 0; ix < text_offset; ix++) {
    if (tq[ix].enabled != 0) {
      CPlayerMedia *mptr = new CPlayerMedia(psptr, TIMED_TEXT_SYNC);
      err = mptr->create_streaming(tq[ix].fptr->media, 
				   broadcast, 
				   config.get_config_value(CONFIG_USE_RTP_OVER_RTSP),
				   media_count,
				   NULL);
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
  if (tq != NULL) free(tq);

  if (media_count == 0) {
    psptr->set_message("No known codecs found in SDP");
    return (-1);
  }
  psptr->streaming_media_set_up();
  if (have_audio_but_no_driver > 0) {
    psptr->set_message("Not playing audio codecs - no driver");
    return (1);
  }
  if (invalid_count > 0) {
    psptr->set_message("There were unknowns codecs during decode - playing valid ones");
    return (1);
  }
  return (0);
}
/*
 * This is where we get the sdp from a source, iptv or http
 */
static int create_media_for_streaming_broadcast (CPlayerSession *psptr,
						 session_desc_t *sdp,
						 int have_audio_driver,
						 control_callback_vft_t *cc_vft)
{
  int err;
  // need to set range in player session...
  err = psptr->create_streaming_broadcast(sdp);
  if (err != 0) {
    return (-1);
  }
  return (create_media_from_sdp(psptr, 
				sdp, 
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
						control_callback_vft_t *cc_vft)
{
  int err;
  session_desc_t *sdp;
  /*
   * This will open the rtsp session
   */
  err = psptr->create_streaming_ondemand(name, 
					 config.get_config_value(CONFIG_USE_RTP_OVER_RTSP));
  if (err != 0) {
    return (-1);
  }
  sdp = psptr->get_sdp_info();
  int have_audio_driver = do_we_have_audio();
  return (create_media_from_sdp(psptr,
				sdp, 
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
			    sdp_decode_info_t *sdp_info,
			    int have_audio_driver,
			    control_callback_vft_t *cc_vft) 
{
  session_desc_t *sdp;
  int translated;

  if (sdp_info == NULL) {
    psptr->set_message("SDP error");
    return (-1);
  }

  if (sdp_decode(sdp_info, &sdp, &translated) != 0) {
    psptr->set_message("Invalid SDP file");
    sdp_decode_info_free(sdp_info);
    return (-1);
  }

  if (translated != 1) {
    psptr->set_message("More than 1 program described in SDP");
    sdp_decode_info_free(sdp_info);
    return (-1);
  }

  int err;
  if (sdp->control_string != NULL) {
    // An on demand file... Just use the URL...
    err = create_media_for_streaming_ondemand(psptr,
					      sdp->control_string,
					      cc_vft);
    sdp_free_session_desc(sdp);
    sdp_decode_info_free(sdp_info);
    return (err);
  }
  sdp_decode_info_free(sdp_info);
  return (create_media_for_streaming_broadcast(psptr,
					       sdp,
					       have_audio_driver,
					       cc_vft));
}

static int create_media_from_sdp_file(CPlayerSession *psptr, 
				      const char *name, 
				      int have_audio_driver,
				      control_callback_vft_t *cc_vft)
{
  sdp_decode_info_t *sdp_info;
  sdp_info = set_sdp_decode_from_filename(name);

  return (create_from_sdp(psptr, 
			  name, 
			  sdp_info, 
			  have_audio_driver, 
			  cc_vft));
}


static int create_media_from_sdp(CPlayerSession *psptr, 
				 const char *name, 
				 int have_audio_driver,
				 control_callback_vft_t *cc_vft)
{
  sdp_decode_info_t *sdp_info;

  sdp_info = set_sdp_decode_from_memory(name);
  return create_from_sdp(psptr, 
			 name, 
			 sdp_info, 
			 have_audio_driver,
			 cc_vft);
}


static int create_media_for_http (CPlayerSession *psptr,
				  const char *name,
				  control_callback_vft_t *cc_vft)
{
  http_client_t *http_client;

  int ret;
  http_resp_t *http_resp;
  http_resp = NULL;
  http_client = http_init_connection(name);

  if (http_client == NULL) {
    psptr->set_message("Can't create http client");
    return -1;
  } 
  ret = http_get(http_client, NULL, &http_resp);
  if (ret > 0) {
    sdp_decode_info_t *sdp_info;

    int have_audio_driver = do_we_have_audio();
    sdp_info = set_sdp_decode_from_memory(http_resp->body);
    ret = create_from_sdp(psptr, 
			  name, 
			  sdp_info, 
			  have_audio_driver,
			  cc_vft);
  } else {
    ret = -1;
    psptr->set_message("HTTP error %d, %s", http_resp->ret_code, 
	     http_resp->resp_phrase);
  }
  http_resp_free(http_resp);
  http_free_connection(http_client);

  return (ret);
}
#ifndef _WIN32
static session_desc_t *find_sdp_for_program (CPlayerSession *psptr, 
					     const char *cm, 
					     const char *loc,
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
    psptr->set_message("Cannot create http client with %s\n", 
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
				  int have_audio_driver,
				  control_callback_vft_t *cc_vft)
{
  char *slash, *cm;
  uint64_t prog;
  session_desc_t *sdp;

  name += strlen("iptv://");
  slash = strchr(name, '/');
  if (slash == NULL || slash == name) {
    psptr->set_message("Invalid iptv content manager");
    return -1;
  }
  cm = (char *)malloc(slash - name + 1);
  memcpy(cm, name, slash - name);
  cm[slash - name] = '\0';
  slash++;
  if (sscanf(slash, U64, &prog) != 1) {
    psptr->set_message("Invalid iptv program");
    return -1;
  }
  
  // check on-demand first
  sdp = find_sdp_for_program(psptr, cm, "iptvfiles/guide.sdf",
			     prog);
  if (sdp == NULL) {
    sdp = find_sdp_for_program(psptr, cm, "servlet/OdPublish", 
			       prog);
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
					      cc_vft);
    sdp_free_session_desc(sdp);
    return (err);
  }
  return (create_media_for_streaming_broadcast(psptr,
					       sdp,
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
			    control_callback_vft_t *cc_vft)
{
  int err;
#ifdef HAVE_IGMP_V3
  //  gross, but here for multiple files
  const char *mcast_src = config.get_config_string(CONFIG_MULTICAST_SRC);
  if (mcast_src != NULL) {
    udp_set_multicast_src(mcast_src);
  } else {
    udp_set_multicast_src("0.0.0.0");
  }
#endif

  ADV_SPACE(name);
  if (strncmp(name, "rtsp://", strlen("rtsp://")) == 0) {    
    err = create_media_for_streaming_ondemand(psptr, 
					      name, 
					      cc_vft);
    return (err);
  }
  if (strncmp(name, "http://", strlen("http://")) == 0) {
    err = create_media_for_http(psptr, name, cc_vft);
    return (err);
  }    

  int have_audio_driver;

  have_audio_driver = do_we_have_audio();
#ifndef _WIN32
  if (strncmp(name, "iptv://", strlen("iptv://")) == 0) {
    err = create_media_for_iptv(psptr, 
				name, 
				have_audio_driver, 
				cc_vft);
    return err;
  }
#endif

  if (strncmp(name, "v=0", strlen("v=0")) == 0) {
    // sdp
    err = create_media_from_sdp(psptr, 
				name, 
				have_audio_driver,
				cc_vft);
    return err;
  }

#ifndef _WIN32
  if (strncmp(name, "mpeg2t://", strlen("mpeg2t://")) == 0) {
    err = create_mpeg2t_session(psptr, name, NULL, 
				have_audio_driver, cc_vft);
    return (err);
  }

  struct stat statbuf;
  if (stat(name, &statbuf) != 0) {
    psptr->set_message("File \'%s\' not found", name);
    return (-1);
  }
  if (!S_ISREG(statbuf.st_mode)) {
    psptr->set_message("File \'%s\' is not a file", name);
    return (-1);
  }
#else
  HANDLE hFile;
  hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		     FILE_ATTRIBUTE_NORMAL, NULL);
  
  if (hFile == INVALID_HANDLE_VALUE) { 
    psptr->set_message("File %s not found", name);
    return (-1);
  }
#endif

  err = -1;

  const char *suffix = strrchr(name, '.');
  if (suffix == NULL) {
    psptr->set_message("No useable suffix on file %s", name);
    return err;
  }


  if (strcasecmp(suffix, ".m4v") == 0) {
    err = create_media_for_mp4_file(psptr, 
				    name, 
				    have_audio_driver, 
				    cc_vft);
  }
  if (err < 0) {
      
  if (strcasecmp(suffix, ".sdp") == 0) {
    err = create_media_from_sdp_file(psptr, 
				     name, 
				     have_audio_driver,
				     cc_vft);
  } else if ((strcasecmp(suffix, ".mov") == 0) ||
	     (strcasecmp(suffix, ".mp4") == 0) ||
	     (strcasecmp(suffix, ".3gp") == 0) ||
	     (strcasecmp(suffix, ".3g2") == 0) ||
	     (strcasecmp(suffix, ".m4a") == 0)) {
    if (config.get_config_value(CONFIG_USE_OLD_MP4_LIB) == 0) {
      err = create_media_for_mp4_file(psptr, 
				      name, 
				      have_audio_driver,
				      cc_vft);
    } else err = -1;
    if (err < 0) {
      err = create_media_for_qtime_file(psptr, 
					name, 
					have_audio_driver);
    }
  } else if (strcasecmp(suffix, ".avi") == 0) {
    err = create_media_for_avi_file(psptr, 
				    name, 
				    have_audio_driver,
				    cc_vft);
  } else if (strcasecmp(suffix, ".mpeg") == 0 ||
	     strcasecmp(suffix, ".mpg") == 0 ||
	     strcasecmp(suffix, ".ts") == 0 ||
	     strcasecmp(suffix, ".vob") == 0) {
#ifdef _WIN32
	  err = -1;
	  psptr->set_message("Do not handle mpeg files in windows");
#else
    err = create_media_for_mpeg2t_file(psptr, name, 
				       have_audio_driver, cc_vft);

    if (err < 0) {
    err = create_media_for_mpeg_file(psptr, name, have_audio_driver, cc_vft);
    }
#endif
  } else {
    // raw files
    codec_data_t *cdata = NULL;
    double maxtime;
    char *desc[4];
    bool is_audio = true;
    codec_plugin_t *codec;
    desc[0] = NULL;
    desc[1] = NULL;
    desc[2] = NULL;
    desc[3] = NULL;

    if (have_audio_driver) {
      cdata = audio_codec_check_for_raw_file(name,
					     &codec,
					     &maxtime, 
					     desc,
					     &config);
    }
    if (cdata == NULL) {
      cdata = video_codec_check_for_raw_file(name, 
					     &codec,
					     &maxtime,
					     desc,
					     &config);
      is_audio = false;
    }

    if (cdata == NULL) {
      err = -1;
	  psptr->set_message("file \"%s\" is not understood", name);
    } else {
      CPlayerMedia *mptr;
      /*
       * Create the player media, and the bytestream
       */
      mptr = new CPlayerMedia(psptr, is_audio ? AUDIO_SYNC : VIDEO_SYNC);
      
      COurInByteStreamFile *fbyte;
      fbyte = new COurInByteStreamFile(codec,
				       cdata,
				       maxtime);
      mptr->create_media(is_audio ? "audio" : "video", fbyte);
      mptr->set_plugin_data(codec, cdata, 
			    is_audio ? NULL : get_video_vft(),
			    is_audio ? get_audio_vft() : NULL);

      for (int ix = 0; ix < 4; ix++) 
	if (desc[ix] != NULL) 
	  psptr->set_session_desc(ix, desc[ix]);
      
      if (maxtime != 0.0) {
	psptr->session_set_seekable(1);
      }
      err = 0;
    }
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

CPlayerSession *start_session (CMsgQueue *master_queue,
			       SDL_sem *master_sem,
			       void *persist,
			       const char *name,
			       control_callback_vft_t *cc_vft,
			       int audio_volume,
			       int screen_loc_x,
			       int screen_loc_y,
			       int screen_size,
			       double start_time,
			       struct rtp *video_rtp,
			       struct rtp *audio_rtp
			       )
{
  CPlayerSession *psptr;

  CMsg *newmsg;
  while ((newmsg = master_queue->get_message()) != NULL) {
    delete newmsg;
  }
  psptr = new CPlayerSession(master_queue, 
			     master_sem, 
			     name, 
			     cc_vft,
			     persist,
			     start_time);
  if (psptr == NULL) return NULL;

  psptr->set_audio_rtp_session(audio_rtp);
  psptr->set_video_rtp_session(video_rtp);
  psptr->set_audio_volume(audio_volume);
  psptr->set_screen_location(screen_loc_x, screen_loc_y);
  bool fullscreen = config.GetBoolValue(CONFIG_FULL_SCREEN);
  int pix_w = 0, pix_h = 0;
  switch (config.get_config_value(CONFIG_ASPECT_RATIO)) {
  case 1: pix_w = 4; pix_h = 3; break;
  case 2: pix_w = 16; pix_h = 9; break;
  case 3: pix_w = 185; pix_h = 100; break;
  case 4: pix_w = 235; pix_h = 100; break;
  case 5: pix_w = 1; pix_h = 1; break;
  }

  psptr->set_screen_size(screen_size, fullscreen, pix_w, pix_h);
  // we've set up all the stuff - start
  psptr->start();
  return psptr;
}

  



  

/* end file media_utils.cpp */
