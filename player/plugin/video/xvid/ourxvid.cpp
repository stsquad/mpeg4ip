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
#define DECLARE_CONFIG_VARIABLES
#include "ourxvid.h"
#include "codec_plugin.h"
#include <mp4util/mpeg4_sdp.h>
#include <mp4v2/mp4.h>
#include <xvid.h>
#include <mp4av/mp4av.h>

#define xvid_message (xvid->m_vft->log_msg)

// Convert a hex character to it's decimal value.
static uint8_t tohex (char a)
{ 
  if (isdigit(a))
    return (a - '0');
  return (tolower(a) - 'a' + 10);
}
static int look_and_parse_vol (xvid_codec_t *xvid,
			       uint8_t *bufptr,
			       uint32_t len)
{
  uint8_t *volptr;
  uint32_t vollen;
  int ret;
  volptr = MP4AV_Mpeg4FindVol(bufptr, len);
  if (volptr == NULL) {
    return -1;
  }
  vollen = len - (volptr - bufptr);

  uint8_t timeBits;
  uint16_t timeTicks, dur, width, height;
  uint8_t aspect_ratio, aspect_ratio_w, aspect_ratio_h;
  if (MP4AV_Mpeg4ParseVol(volptr, 
			  vollen, 
			  &timeBits, 
			  &timeTicks,
			  &dur,
			  &width,
			  &height,
			  &aspect_ratio,
			  &aspect_ratio_w,
			  &aspect_ratio_h) == false) {
    return -1;
  }

  xvid_message(LOG_DEBUG, "xvid", "aspect ratio %x %d %d", 
	       aspect_ratio, aspect_ratio_w, aspect_ratio_h);
  // Get the VO/VOL header.  If we fail, set the bytestream back
  ret = 0;
  XVID_DEC_PARAM param;
  param.width = width;
  param.height = height;

  ret = xvid_decore(NULL, XVID_DEC_CREATE,
		    &param, NULL);
  if (ret != XVID_ERR_OK) {
    return -1;
  }
  double ar = 0.0;
  switch (aspect_ratio) {
  default:
    aspect_ratio_h = 0;
    break; 
  case 2:
    aspect_ratio_w = 12;
    aspect_ratio_h = 11;
    break;
  case 3:
    aspect_ratio_w = 10;
    aspect_ratio_h = 11;
    break;
  case 4:
    aspect_ratio_w = 16;
    aspect_ratio_h = 11;
    break;
  case 5:
    aspect_ratio_w = 40;
    aspect_ratio_h = 33;
    break;
  case 0xf:
    break;
  }
  if (aspect_ratio_h != 0) {
    ar = (double)aspect_ratio_w;
    ar *= (double) param.width;
    ar /= (double)aspect_ratio_h;
    ar /= (double)param.height;
  }
  xvid->m_xvid_handle = param.handle;
  xvid->m_vft->video_configure(xvid->m_ifptr, 
			       param.width,
			       param.height,
			       VIDEO_FORMAT_YUV,
			       ar);
  // we need to then run the VOL through the decoder.
  XVID_DEC_FRAME frame;
  XVID_DEC_PICTURE decpict;

  frame.bitstream = (void *)volptr;
  frame.length = vollen;
  frame.colorspace = XVID_CSP_USER;
  frame.image = &decpict;
  ret = xvid_decore(xvid->m_xvid_handle, XVID_DEC_DECODE, &frame, NULL);
  xvid_message(LOG_NOTICE, "xvidif", "decoded vol ret %d", ret);
  
  return ret == XVID_ERR_OK ? 0 : -1;
}

// Parse the format config passed in.  This is the vo vod header
// that we need to get width/height/frame rate
static int parse_vovod (xvid_codec_t *xvid,
			const char *vovod,
			int ascii,
			uint32_t len)
{
  uint8_t buffer[255];
  uint8_t *bufptr;

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
    uint8_t *write;
    write = buffer;
    // Convert the config= from ascii to binary
    for (uint32_t ix = 0; ix < len; ix += 2) {
      *write = 0;
      *write = (tohex(*config)) << 4;
      config++;
      *write += tohex(*config);
      config++;
      write++;
    }
    len /= 2;
    bufptr = buffer;
  } else {
    bufptr = (uint8_t *)vovod;
  }
  return look_and_parse_vol(xvid, bufptr, len);
}

static codec_data_t *xvid_create (const char *stream_type,
				  const char *compressor, 
				  int type, 
				  int profile,
				  format_list_t *media_fmt,
				  video_info_t *vinfo,
				  const uint8_t *userdata,
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

  xvid->m_decodeState = XVID_STATE_VO_SEARCH;
  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    // See if we can decode a passed in vovod header
    if (parse_vovod(xvid, media_fmt->fmt_param, 1, 0) == 0) {
      xvid->m_decodeState = XVID_STATE_WAIT_I;
    }
  } else if (userdata != NULL) {
    if (parse_vovod(xvid, (char *)userdata, 0, ud_size) == 0) {
      xvid->m_decodeState = XVID_STATE_WAIT_I;
    }
  } 
  xvid->m_vinfo = vinfo;

  xvid->m_num_wait_i = 0;
  xvid->m_num_wait_i_frames = 0;
  xvid->m_total_frames = 0;
  xvid_message(LOG_DEBUG, "xvid", "created xvid");
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

static int xvid_frame_is_sync (codec_data_t *ptr,
			       uint8_t *buffer, 
			       uint32_t buflen,
			       void *userdata)
{
  int vop_type;

  while (buflen > 3 && 
	 !(buffer[0] == 0 && buffer[1] == 0 && 
	   buffer[2] == 1 && buffer[3] == MP4AV_MPEG4_VOP_START)) {
    buffer++;
    buflen--;
  }

  vop_type = MP4AV_Mpeg4GetVopType(buffer, buflen);

  if (vop_type == VOP_TYPE_I) return 1;
  return 0;
}

static int xvid_decode (codec_data_t *ptr,
			frame_timestamp_t *pts, 
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer, 
			uint32_t buflen,
			void *ud)
{
  int ret;
  int do_wait_i = 0;
  xvid_codec_t *xvid = (xvid_codec_t *)ptr;
  XVID_DEC_FRAME frame;
  uint64_t ts = pts->msec_timestamp;

#if 0
  xvid->m_vft->log_msg(LOG_DEBUG, "xvid", " %d frame "U64" len %u %d", 
		       xvid->m_total_frames, ts, buflen,
		       xvid->m_decodeState);
#endif
  xvid->m_total_frames++;

  
  switch (xvid->m_decodeState) {
  case XVID_STATE_VO_SEARCH:
    ret = look_and_parse_vol(xvid, buffer, buflen);
    if (ret < 0) {
      return buflen;
    }
    xvid->m_decodeState = XVID_STATE_NORMAL;
    break;
  case XVID_STATE_WAIT_I:
    do_wait_i = 1;
    break;
  case XVID_STATE_NORMAL:
    break;
  }
  
  frame.bitstream = buffer;
  frame.length = buflen;
  frame.colorspace = XVID_CSP_USER;
  XVID_DEC_PICTURE decpict;
  frame.image = &decpict;

  ret = xvid_decore(xvid->m_xvid_handle, XVID_DEC_DECODE, &frame, NULL);
  if (xvid->m_decodeState == XVID_STATE_VO_SEARCH) {
    xvid->m_vft->video_configure(xvid->m_ifptr, 
				 xvid->m_width,
				 xvid->m_height,
				 VIDEO_FORMAT_YUV,
				 0.0);
    xvid->m_decodeState = XVID_STATE_NORMAL;
  }

  if (ret == XVID_ERR_OK) {
    xvid->m_vft->video_have_frame(xvid->m_ifptr,
				  (const uint8_t *)decpict.y,
				  (const uint8_t *)decpict.u,
				  (const uint8_t *)decpict.v,
				  decpict.stride_y,
				  decpict.stride_u,
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
			     const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			     CConfigSet *pConfig)
{
  int retval;
  retval = 3;
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0 &&
      (strcasecmp(compressor, "mp4v") == 0 ||
       strcasecmp(compressor, "encv") == 0)) {
    if ((type == MP4_MPEG4_VIDEO_TYPE) &&
	((profile >= MPEG4_SP_L1 && profile <= MPEG4_SP_L3) ||
	 (profile == MPEG4_SP_L0))) {
      return retval;
    }
    return -1;
  }
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    // find format. If matches, call parse_fmtp_for_mpeg4, look at
    // profile level.
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "MP4V-ES") == 0) {
	fmtp_parse_t *fmtp;
	media_desc_t *mptr = fptr->media;
	if (find_unparsed_a_value(mptr->unparsed_a_lines, "a=x-mpeg4-simple-profile-decoder") != NULL) {
	  // our own special code for simple profile decoder
	  return retval;
	}
	fmtp = parse_fmtp_for_mpeg4(fptr->fmt_param, message);
	int retvl = -1;
	if (fmtp != NULL) {
	  int profile = fmtp->profile_level_id;
	  if ((profile >= MPEG4_SP_L1 && profile <= MPEG4_SP_L3) || 
	      (profile == MPEG4_SP_L0)) {
	    retvl = retval;
	  } else if (fmtp->config_binary != NULL) {
	    // see if they indicate simple tools in VOL
#if 0
	    uint8_t *volptr;
	    volptr = MP4AV_Mpeg4FindVol(fmtp->config_binary, 
					fmtp->config_binary_len);
	    if (volptr != NULL && 
		((volptr[4] & 0x7f) == 0) &&
		((volptr[5] & 0x80) == 0x80)) {
message(LOG_ERR, "xvid", "indicated simple tools in xvid");
	      retvl = retval;
	    }
#endif
	  }
	  free_fmtp_parse(fmtp);
	}
	return retvl;
      }
    }
    return -1;
  }

  if (compressor != NULL) {
    const char **lptr = xvid_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return retval;
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
				 NULL,
				 xvid_close,
				 xvid_codec_check,
				 xvid_frame_is_sync,
				 xvid_file_check,
				 xvid_file_next_frame,
				 xvid_file_used_for_frame,
				 xvid_file_seek_to,
				 xvid_skip_frame,
				 xvid_file_eof,
				 NULL,
				 0);
