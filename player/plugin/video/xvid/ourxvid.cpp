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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * ourxvid.cpp - plugin interface with xvid decoder.
 */

#include "ourxvid.h"
#include "codec_plugin.h"
#include <mp4util/mpeg4_sdp.h>
#include <gnu/strcasestr.h>
#include <mp4v2/mp4.h>
#include <xvid.h>
#include <divx4.h>

#define xvid_message (xvid->m_vft->log_msg)

// Convert a hex character to it's decimal value.
static char tohex (char a)
{ 
  if (isdigit(a))
    return (a - '0');
  return (tolower(a) - 'a' + 10);
}

// Parse the format config passed in.  This is the vo vod header
// that we need to get width/height/frame rate
static int parse_vovod (xvid_codec_t *xvid,
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
  XVID_DEC_FRAME frame;
  XVID_DEC_PARAM param;
  frame.bitstream = (void *)bufptr;
  frame.length = len;
  ret = xvid_decore(xvid->m_xvid_handle, XVID_DEC_FIND_VOL,
		    &frame, &param);

  if (ret == XVID_ERR_OK) {
    xvid_message(LOG_DEBUG, "xvid", "found vol %d %d", 
		 param.width, param.height);
    xvid->m_vft->video_configure(xvid->m_ifptr, 
				 param.width,
				 param.height,
				 VIDEO_FORMAT_YUV);
  }
  return ret;
}

static codec_data_t *xvid_create (format_list_t *media_fmt,
				  video_info_t *vinfo,
				  const unsigned char *userdata,
				  uint32_t ud_size,
				  video_vft_t *vft,
				  void *ifptr)
{
  xvid_codec_t *xvid;

  xvid = MALLOC_STRUCTURE(xvid_codec_t);
  memset(xvid, 0, sizeof(*xvid));

  xvid->m_vft = vft;
  xvid->m_ifptr = ifptr;

  XVID_INIT_PARAM xvid_param;
  xvid_param.cpu_flags = 0;
  xvid_init(NULL, 0, &xvid_param, NULL);

  XVID_DEC_PARAM dec_param;

  xvid_decore(NULL, XVID_DEC_ALLOC, &dec_param, NULL);
  xvid->m_xvid_handle = dec_param.handle;

  xvid->m_decodeState = XVID_STATE_VO_SEARCH;
  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    // See if we can decode a passed in vovod header
    if (parse_vovod(xvid, media_fmt->fmt_param, 1, 0) == XVID_ERR_OK) {
      xvid->m_decodeState = XVID_STATE_WAIT_I;
    }
  } else if (userdata != NULL) {
    if (parse_vovod(xvid, (const char *)userdata, 0, ud_size) == XVID_ERR_OK) {
      xvid->m_decodeState = XVID_STATE_WAIT_I;
    }
  } 
  xvid->m_vinfo = vinfo;

  xvid->m_num_wait_i = 0;
  xvid->m_num_wait_i_frames = 0;
  xvid->m_total_frames = 0;
  return ((codec_data_t *)xvid);
}

void xvid_clean_up (xvid_codec_t *xvid)
{
  if (xvid->m_xvid_handle != NULL) {
    xvid_decore(xvid->m_xvid_handle, XVID_DEC_DESTROY, NULL, NULL);
    xvid->m_xvid_handle = NULL;
  }

  if (xvid->m_ifile != NULL) {
    fclose(xvid->m_ifile);
    xvid->m_ifile = NULL;
  }
  if (xvid->m_buffer != NULL) {
    free(xvid->m_buffer);
    xvid->m_buffer = NULL;
  }
  if (xvid->m_fpos != NULL) {
    delete xvid->m_fpos;
    xvid->m_fpos = NULL;
  }

  free(xvid);
}

static void xvid_close (codec_data_t *ifptr)
{
  xvid_codec_t *xvid;

  xvid = (xvid_codec_t *)ifptr;

  xvid_message(LOG_NOTICE, "xvidif", "Xvid codec results:");
  xvid_message(LOG_NOTICE, "xvidif", "total frames    : %u", xvid->m_total_frames);
  xvid_message(LOG_NOTICE, "xvidif", "wait for I times: %u", xvid->m_num_wait_i);
  xvid_message(LOG_NOTICE, "xvidif", "wait I frames   : %u", xvid->m_num_wait_i_frames);
  xvid_clean_up(xvid);
}



static void xvid_do_pause (codec_data_t *ifptr)
{
  xvid_codec_t *xvid = (xvid_codec_t *)ifptr;
  if (xvid->m_decodeState != XVID_STATE_VO_SEARCH)
    xvid->m_decodeState = XVID_STATE_WAIT_I;
}

static int xvid_decode (codec_data_t *ptr,
			uint64_t ts, 
			int from_rtp,
			int *sync_frame,
			unsigned char *buffer, 
			uint32_t buflen,
			void *ud)
{
  int ret;
  int do_wait_i = 0;
  xvid_codec_t *xvid = (xvid_codec_t *)ptr;
  XVID_DEC_FRAME frame;

#if 0
  xvid->m_vft->log_msg(LOG_DEBUG, "xvid", " %d frame %llu len %u %d", 
		       xvid->m_total_frames, ts, buflen,
		       xvid->m_decodeState);
#endif
  xvid->m_total_frames++;

  frame.bitstream = buffer;
  frame.length = buflen;
  
  switch (xvid->m_decodeState) {
  case XVID_STATE_VO_SEARCH:
    XVID_DEC_PARAM param;
    ret = xvid_decore(xvid->m_xvid_handle, 
		      XVID_DEC_FIND_VOL,
		      &frame,
		      &param);

    if (ret > 0) {
	xvid->m_decodeState = XVID_STATE_WAIT_I;
	xvid->m_vft->video_configure(xvid->m_ifptr, 
				     param.width,
				     param.height,
				     VIDEO_FORMAT_YUV);
    } else {
      xvid_decore(xvid->m_xvid_handle, XVID_DEC_DESTROY, NULL, NULL);

      xvid->m_xvid_handle = NULL;
      XVID_DEC_PARAM dec_param;
      dec_param.width = xvid->m_vinfo->width;
      dec_param.height = xvid->m_vinfo->height;
      xvid_decore(NULL, XVID_DEC_CREATE, &dec_param, NULL);

      xvid->m_xvid_handle = dec_param.handle;

      xvid->m_vft->video_configure(xvid->m_ifptr, 
				   xvid->m_vinfo->width,
				   xvid->m_vinfo->height,
				   VIDEO_FORMAT_YUV);
      xvid->m_decodeState = XVID_STATE_NORMAL;
    } 
    break;
  case XVID_STATE_WAIT_I:
    do_wait_i = 1;
    break;
  case XVID_STATE_NORMAL:
    break;
  }
  
  frame.colorspace = XVID_CSP_USER;
  DEC_PICTURE decpict;
  frame.image = &decpict;

  ret = xvid_decore(xvid->m_xvid_handle, XVID_DEC_DECODE, &frame, NULL);

  if (ret == XVID_ERR_OK) {
    xvid->m_vft->video_have_frame(xvid->m_ifptr,
				  (const unsigned char *)decpict.y,
				  (const unsigned char *)decpict.u,
				  (const unsigned char *)decpict.v,
				  decpict.stride_y,
				  decpict.stride_uv,
				  ts);
    ret = frame.length;
  } else {
#if 0
    xvid->m_vft->log_msg(LOG_DEBUG, "xvid", "error returned %d", ret);
#endif
  }
      
  return (ret);
}

static int xvid_skip_frame (codec_data_t *ifptr)

{
  return 0;
}

static const char *xvid_compressors[] = {
  "xvid", 
  "divx",
  "dvx1", 
  "div4", 
  NULL,
};

static int xvid_codec_check (lib_message_func_t message,
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
      return 3;
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
	  if (profile >= 1 && profile <= 3) return 3;
	}
	return -1;
      }
    }
    return -1;
  }

  if (compressor != NULL) {
    const char **lptr = xvid_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 3;
      }
      lptr++;
    }
  }
  return -1;
}

VIDEO_CODEC_WITH_RAW_FILE_PLUGIN("xvid", 
				 xvid_create,
				 xvid_do_pause,
				 xvid_decode,
				 xvid_close,
				 xvid_codec_check,
				 xvid_file_check,
				 xvid_file_next_frame,
				 xvid_file_used_for_frame,
				 xvid_file_seek_to,
				 xvid_skip_frame,
				 xvid_file_eof);
