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

static int look_for_vol (xvid_codec_t *xvid, 
			 uint8_t *bufptr, 
			 uint32_t len)
{
  uint8_t *volptr;
  int vollen;
  int ret;
  volptr = MP4AV_Mpeg4FindVol(bufptr, len);
  if (volptr == NULL) {
    return -1 ;
  }
  vollen = len - (volptr - bufptr);

  uint8_t timeBits;
  uint16_t timeTicks, dur, width, height;
  uint8_t aspect_ratio, aspect_ratio_w, aspect_ratio_h;
  aspect_ratio = aspect_ratio_w = aspect_ratio_h = 0;
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

  xvid_dec_create_t create;
  create.version = XVID_VERSION;
  create.width = width;
  create.height = height;

  ret = xvid_decore(NULL, XVID_DEC_CREATE,
		    &create, NULL);
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
      ar *= (double) create.width;
      ar /= (double)aspect_ratio_h;
      ar /= (double)create.height;
    }
    xvid->m_xvid_handle = create.handle;
    xvid->m_vft->video_configure(xvid->m_ifptr, 
				 create.width,
				 create.height,
				 VIDEO_FORMAT_YUV,
				 ar);
   // we need to then run the VOL through the decoder.
  xvid_dec_frame_t dec;
  xvid_dec_stats_t stats;
  do {
    dec.version = XVID_VERSION;
    dec.bitstream = volptr;
    dec.length = vollen;
    dec.output.csp = XVID_CSP_INTERNAL;

    stats.version = XVID_VERSION;

    ret = xvid_decore(xvid->m_xvid_handle, 
		      XVID_DEC_DECODE, 
		      &dec, 
		      &stats);
    if (ret < 0) {
      xvid_message(LOG_NOTICE, "xvidif", "decoded vol ret %d", ret);
    }
      
    if (ret < 0 || ret > vollen) {
      vollen = 0;
    } else {
      vollen -= ret;
      volptr += ret;
    }
    // we could check for vol changes, etc here, if we wanted.
  } while (vollen > 4 && stats.type == 0);
  return 0;
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
      return -1;
    }
    config += strlen("config=");
    const char *end;
    end = config;
    while (isxdigit(*end)) end++;
    if (config == end) {
      return -1;
    }
    // config string will run from config to end
    len = end - config;
    // make sure we have even number of digits to convert to binary
    if ((len % 2) == 1) 
      return -1;
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
  return look_for_vol(xvid, bufptr, len);
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

  xvid_gbl_init_t gbl_init;
  gbl_init.version = XVID_VERSION;
  gbl_init.cpu_flags = 0;
  xvid_global(NULL, 0, &gbl_init, NULL);

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
  xvid_clean_up(xvid);
}



static void xvid_do_pause (codec_data_t *ifptr)
{
  // nothing here
  xvid_codec_t *xvid = (xvid_codec_t *)ifptr;
  MP4AV_clear_dts_from_pts(&xvid->pts_to_dts);
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
			uint32_t blen,
			void *ud)
{
  int ret;
  xvid_codec_t *xvid = (xvid_codec_t *)ptr;
  uint64_t ts = pts->msec_timestamp;
  int buflen = blen, used = 0;

  uint8_t *vop = MP4AV_Mpeg4FindVop(buffer, blen);
  int type = 0;
  if (vop != NULL) {
    type = MP4AV_Mpeg4GetVopType(vop, blen);
    uint64_t dts;
    if (MP4AV_calculate_dts_from_pts(&xvid->pts_to_dts,
				     ts,
				     type,
				     &dts) < 0) {
      return buflen;
    }
    ts = dts;
  }
#if 0
  xvid_message(LOG_DEBUG, "xvidif", "%u at %llu %d", 
	       blen, ts, type);
#endif

  if (xvid->m_decodeState == XVID_STATE_VO_SEARCH) {
    ret = look_for_vol(xvid, buffer, buflen);
    if (ret < 0) {
      return buflen;
    }
    xvid->m_decodeState = XVID_STATE_NORMAL;
  }
  xvid_dec_frame_t dec;
  xvid_dec_stats_t stats;
  do {
    memset(&dec, 0, sizeof(dec));
    memset(&stats, 0, sizeof(stats));
	   
    dec.version = XVID_VERSION;
    dec.bitstream = buffer;
    dec.length = buflen;
    dec.general = 0;
    dec.output.csp = XVID_CSP_INTERNAL;

    stats.version = XVID_VERSION;

    ret = xvid_decore(xvid->m_xvid_handle, 
		      XVID_DEC_DECODE, 
		      &dec, 
		      &stats);
#if 0
    xvid_message(LOG_DEBUG, "xvidif", "ret %d type %d blen %d of %u",
		 ret, stats.type, buflen, blen);
#endif
    if (ret < 0 || ret > buflen) {
      buflen = 0;
      used = blen;
    } else {
      buflen -= ret;
      buffer += ret;
      used += ret;
    }
    // we could check for vol changes, etc here, if we wanted.
  } while (buflen > 4 && stats.type <= 0);

  if (stats.type > 0) {
    xvid->m_vft->video_have_frame(xvid->m_ifptr,
				  (const uint8_t *)dec.output.plane[0],
				  (const uint8_t *)dec.output.plane[1],
				  (const uint8_t *)dec.output.plane[2],
				  dec.output.stride[0],
				  dec.output.stride[1],
				  ts);
  } 
#if 0
    xvid->m_vft->log_msg(LOG_DEBUG, "xvid", "error returned %d", ret);
#endif
    xvid->m_total_frames++;
  return (used);
}
#if 0
static int xvid_skip_frame (codec_data_t *ifptr)

{
  return 0;
}
#endif

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
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0 &&
      (strcasecmp(compressor, "mp4v") == 0 ||
       strcasecmp(compressor, "encv") == 0)) {
    if ((type == MP4_MPEG4_VIDEO_TYPE) &&
	((profile >= MPEG4_SP_L1 && profile <= MPEG4_SP_L3) ||
	 (profile == MPEG4_SP_L0) ||
	 (profile >= MPEG4_ASP_L0 && profile <= MPEG4_ASP_L5) ||
	 (profile == MPEG4_ASP_L3B))) {
      return 4;
    }
    return -1;
  }
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    // find format. If matches, call parse_fmtp_for_mpeg4, look at
    // profile level.
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "MP4V-ES") == 0) {
	media_desc_t *mptr = fptr->media;
	if (find_unparsed_a_value(mptr->unparsed_a_lines, "a=x-mpeg4-simple-profile-decoder") != NULL) {
	  // our own special code for simple profile decoder
	  return 4;
	}
#if 1
	// we don't handle b frames right, yet.
	fmtp_parse_t *fmtp;
	fmtp = parse_fmtp_for_mpeg4(fptr->fmt_param, message);
	int retval = -1;
	if (fmtp != NULL) {
	  int profile = fmtp->profile_level_id;
	  if ((profile >= MPEG4_SP_L1 && profile <= MPEG4_SP_L3) || 
	      (profile == MPEG4_SP_L0)) {
	    retval = 4;
	  } else if (fmtp->config_binary != NULL) {
	    // see if they indicate simple tools in VOL
	    uint8_t *volptr;
	    volptr = MP4AV_Mpeg4FindVol(fmtp->config_binary, 
					fmtp->config_binary_len);
	    if (volptr != NULL && 
		((volptr[4] & 0x7f) == 0) &&
		((volptr[5] & 0x80) == 0x80)) {
	      retval = 4;
	    }
	  }
	  free_fmtp_parse(fmtp);
	}
	return retval;
#endif 
	return 4;
      }
    }
    return -1;
  }

  if (compressor != NULL) {
    const char **lptr = xvid_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 4;
      }
      lptr++;
    }
  }
  return -1;
}
#if 0
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
#else
VIDEO_CODEC_PLUGIN("xvid-10", 
				 xvid_create,
				 xvid_do_pause,
				 xvid_decode,
				 NULL,
				 xvid_close,
				 xvid_codec_check,
				 xvid_frame_is_sync,
				 NULL,
				 0);
#endif
