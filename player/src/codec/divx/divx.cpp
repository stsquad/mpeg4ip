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
 * divx.cpp - implementation with ISO reference codec
 */

#include "divx.h"
#include "divxif.h"
#include "codec_plugin.h"
#include <mp4util/mpeg4_sdp.h>
#include <gnu/strcasestr.h>
#include <mp4v2/mp4.h>

#define divx_message (divx->m_vft->log_msg)

// Convert a hex character to it's decimal value.
static char tohex (char a)
{ 
  if (isdigit(a))
    return (a - '0');
  return (tolower(a) - 'a' + 10);
}

// Parse the format config passed in.  This is the vo vod header
// that we need to get width/height/frame rate
static int parse_vovod (divx_codec_t *divx,
			const char *vovod,
			int ascii,
			uint32_t len)
{
  unsigned char buffer[255];
  const char *bufptr;
  int ret;

  if (ascii == 1) {
    const char *config = strcasestr(vovod, "config=");
    if (config == NULL) {
      return 0;
    }
    config += strlen("config=");
    const char *end;
    end = config;
    while (isxdigit(*end)) end++;
    if (config == end) {
      return 0;
    }
    // config string will run from config to end
    len = end - config;
    // make sure we have even number of digits to convert to binary
    if ((len % 2) == 1) 
      return 0;
    unsigned char *write;
    write = buffer;
    // Convert the config= from ascii to binary
    for (uint32_t ix = 0; ix < len; ix++) {
      *write = 0;
      *write = (tohex(*config)) << 4;
      config++;
      *write += tohex(*config);
      config++;
      write++;
    }
    len /= 2;
    bufptr = (const char *)buffer;
  } else {
    bufptr = vovod;
  }
  
  // Get the VO/VOL header.  If we fail, set the bytestream back
  ret = 0;
  try {
    ret = newdec_read_volvop((unsigned char *)bufptr, len);
    if (ret != 0) {
      divx_message(LOG_DEBUG, "divxif", "Found VOL in header");
      divx->m_vft->video_configure(divx->m_ifptr,
				   mp4_hdr.width,
				   mp4_hdr.height,
				   VIDEO_FORMAT_YUV);
      post_volprocessing();
    }
    
  } catch (int err) {

  }

  if (ret == 0) {
    return (0);
  }

  // We've found the VO VOL header - that's good.
  // Reset the byte stream back to what it was, delete our temp stream
  //player_debug_message("Decoded vovod header correctly");
  return ret;
}

static codec_data_t *divx_create (format_list_t *media_fmt,
				  video_info_t *vinfo,
				  const unsigned char *userdata,
				  uint32_t ud_size,
				  video_vft_t *vft,
				  void *ifptr)
{
  divx_codec_t *divx;

  divx = MALLOC_STRUCTURE(divx_codec_t);
  memset(divx, 0, sizeof(*divx));

  divx->m_vft = vft;
  divx->m_ifptr = ifptr;
  divx->m_last_time = 0;

  newdec_init();
  divx->m_decodeState = DIVX_STATE_VO_SEARCH;
  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    // See if we can decode a passed in vovod header
    if (parse_vovod(divx, media_fmt->fmt_param, 1, 0) == 1) {
      divx->m_decodeState = DIVX_STATE_WAIT_I;
    }
  } else if (userdata != NULL) {
    if (parse_vovod(divx, (const char *)userdata, 0, ud_size) == 1) {
      divx->m_decodeState = DIVX_STATE_WAIT_I;
    }
  } else if (vinfo != NULL) {
    mp4_hdr.width = vinfo->width;
    mp4_hdr.height = vinfo->height;
    
    // defaults from mp4_decoder.c routine dec_init...
    mp4_hdr.quant_precision = 5;
    mp4_hdr.bits_per_pixel = 8;

    mp4_hdr.quant_type = 0;

    mp4_hdr.time_increment_resolution = 15;
    mp4_hdr.time_increment_bits = 0;
    while (mp4_hdr.time_increment_resolution > 
	   (1 << mp4_hdr.time_increment_bits)) {
      mp4_hdr.time_increment_bits++;
    }
    mp4_hdr.fps = 30;

    mp4_hdr.complexity_estimation_disable = 1;

    divx->m_vft->video_configure(divx->m_ifptr, 
				 vinfo->width,
				 vinfo->height,
				 VIDEO_FORMAT_YUV);
    post_volprocessing();
    divx->m_decodeState = DIVX_STATE_NORMAL;
  } 

  divx->m_dropped_b_frames = 0;
  divx->m_num_wait_i = 0;
  divx->m_num_wait_i_frames = 0;
  divx->m_total_frames = 0;
  return ((codec_data_t *)divx);
}

void divx_clean_up (divx_codec_t *divx)
{
  closedecoder();
  if (divx->m_ifile != NULL) {
    fclose(divx->m_ifile);
    divx->m_ifile = NULL;
  }
  if (divx->m_buffer != NULL) {
    free(divx->m_buffer);
    divx->m_buffer = NULL;
  }
  if (divx->m_fpos != NULL) {
    delete divx->m_fpos;
    divx->m_fpos = NULL;
  }

  free(divx);
}

static void divx_close (codec_data_t *ifptr)
{
  divx_codec_t *divx;

  divx = (divx_codec_t *)ifptr;

  divx_message(LOG_NOTICE, "divxif", "Divx codec results:");
  divx_message(LOG_NOTICE, "divxif", "total frames    : %u", divx->m_total_frames);
  divx_message(LOG_NOTICE, "divxif", "dropped b frames: %u", divx->m_dropped_b_frames);
  divx_message(LOG_NOTICE, "divxif", "wait for I times: %u", divx->m_num_wait_i);
  divx_message(LOG_NOTICE, "divxif", "wait I frames   : %u", divx->m_num_wait_i_frames);
  divx_clean_up(divx);
}



static void divx_do_pause (codec_data_t *ifptr)
{
  divx_codec_t *divx = (divx_codec_t *)ifptr;
  if (divx->m_decodeState != DIVX_STATE_VO_SEARCH)
    divx->m_decodeState = DIVX_STATE_WAIT_I;
}

static int divx_decode (codec_data_t *ptr,
			uint64_t ts, 
			int from_rtp,
			int *sync_frame,
			unsigned char *buffer, 
			uint32_t buflen)
{
  int ret;
  int do_wait_i = 0;
  divx_codec_t *divx = (divx_codec_t *)ptr;

  //divx->m_vft->log_msg(LOG_DEBUG, "divx", "frame "LLU, ts);
  divx->m_total_frames++;
  
  switch (divx->m_decodeState) {
  case DIVX_STATE_VO_SEARCH:
    try {
      ret = newdec_read_volvop(buffer, buflen);
      if (ret != 0) {
	divx->m_vft->video_configure(divx->m_ifptr, 
				     mp4_hdr.width,
				     mp4_hdr.height, 
				     VIDEO_FORMAT_YUV);
	post_volprocessing();
	divx->m_decodeState = DIVX_STATE_WAIT_I;
      } else {
	return (-1);
      }
    } catch (int err) {
      divx_message(LOG_INFO, "divxif", "Caught exception in VO search %d", 
		   err);
      return (-1);
    }
    //      return(0);
  case DIVX_STATE_WAIT_I:
    do_wait_i = 1;
    break;
  case DIVX_STATE_NORMAL:
    break;
  }
  
  unsigned char *y, *u, *v;
  ret = divx->m_vft->video_get_buffer(divx->m_ifptr, &y, &u, &v);
  if (ret == 0) {
    return (-1);
  }

  try {
    ret = newdec_frame(y, v, u, do_wait_i, buffer, buflen);

    if (ret < 0) {
      //player_debug_message("ret from newdec_frame %d "LLU, ret, ts);
      if (divx->m_last_time != ts)
	divx->m_decodeState = DIVX_STATE_WAIT_I;
      return (-ret);
    }
    //player_debug_message("frame type %d", mp4_hdr.prediction_type);
    divx->m_decodeState = DIVX_STATE_NORMAL;
    divx->m_last_time = ts;
  } catch (int err) {
    divx_message(LOG_DEBUG, "divxif", "divx caught %d", 
		 err);
    divx->m_decodeState = DIVX_STATE_WAIT_I;
    return (-1);
  } catch (...) {
    divx_message(LOG_DEBUG, "divxif", "divx caught exception");
    divx->m_decodeState = DIVX_STATE_WAIT_I;
    return (-1);
  }

  divx->m_vft->video_filled_buffer(divx->m_ifptr, ts);
      
  return (ret);
}

static int divx_skip_frame (codec_data_t *ifptr)

{
  return 0;
}

static const char *divx_compressors[] = {
  "divx", 
  "dvx1", 
  "div4", 
  NULL,
};

static int divx_codec_check (lib_message_func_t message,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const unsigned char *userdata,
			     uint32_t userdata_size)
{
  if (compressor != NULL && 
      (strcasecmp(compressor, "MP4 FILE") == 0)) {
    if ((type == MP4_MPEG4_VIDEO_TYPE) &&
	(profile >= 1 && profile <= 3)) {
      return 2;
    }
    return -1;
  }
  if (fptr != NULL) {
    // find format. If matches, call parse_fmtp_for_mpeg4, look at
    // profile level.
    if (fptr->rtpmap != NULL && fptr->rtpmap->encode_name != NULL) {
      if (strcasecmp(fptr->rtpmap->encode_name, "MP4V-ES") == 0) {
	fmtp_parse_t *fmtp;
	fmtp = parse_fmtp_for_mpeg4(fptr->fmt_param, message);
	if (fmtp != NULL) {
	  int profile = fmtp->profile_level_id;
	  free_fmtp_parse(fmtp);
	  if (profile >= 1 && profile <= 3) return 2;
	}
	return -1;
      }
    }
    return -1;
  }

  if (compressor != NULL) {
    const char **lptr = divx_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 2;
      }
      lptr++;
    }
  }
  return -1;
}

VIDEO_CODEC_WITH_RAW_FILE_PLUGIN("divx", 
				 divx_create,
				 divx_do_pause,
				 divx_decode,
				 divx_close,
				 divx_codec_check,
				 divx_file_check,
				 divx_file_next_frame,
				 divx_file_used_for_frame,
				 divx_file_seek_to,
				 divx_skip_frame,
				 divx_file_eof);
