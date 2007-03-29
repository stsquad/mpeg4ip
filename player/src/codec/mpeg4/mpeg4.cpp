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
 * mpeg4.cpp - implementation with ISO reference codec
 */


#define DECLARE_CONFIG_VARIABLES
#include "codec_plugin.h"
#if 0
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif // __PC_COMPILER_
#endif

#include <typeapi.h>
#include <mode.hpp>
#include <vopses.hpp>
#include <bitstrm.hpp>
#include <tps_enhcbuf.hpp>
#include <enhcbufdec.hpp>
#include <vopsedec.hpp>
#include "mpeg4.h"

#include <mp4v2/mp4.h>
#include <mp4av/mp4av.h>

#define iso_message (iso->m_vft->log_msg)
static const char *mp4iso = "mp4iso";
static SConfigVariable MyConfigVariables[] = {
  CONFIG_BOOL(CONFIG_USE_MPEG4_ISO_ONLY, "Mpeg4IsoOnly", false),
};

// Convert a hex character to it's decimal value.
static uint8_t tohex (char a)
{ 
  if (isdigit(a))
    return (a - '0');
  return (tolower(a) - 'a' + 10);
}

static double calculate_aspect_ratio (iso_decode_t *iso)
{
  double ret_val;
  int aspect_ratio_h;
  int aspect_ratio_w;

  ret_val = 0.0;
  aspect_ratio_h = 0;
  aspect_ratio_w = 0;
  switch (iso->m_pvodec->getvolAspectRatio()) {
  default:
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
    aspect_ratio_w = iso->m_pvodec->getvolAspectWidth();
    aspect_ratio_h = iso->m_pvodec->getvolAspectHeight();
    break;
  }

  if (aspect_ratio_h != 0) {
    ret_val = (double)iso->m_pvodec->getvolWidth();
    ret_val *= (double)aspect_ratio_w;
    ret_val /= (double)iso->m_pvodec->getvolHeight();
    ret_val /= (double)aspect_ratio_h;
  }
  return ret_val;
}

// Parse the format config passed in.  This is the vo vod header
// that we need to get width/height/frame rate
static int parse_vovod (iso_decode_t *iso,
			const char *vovod,
			int ascii,
			uint32_t len)
{
  uint8_t buffer[255];
  unsigned char *bufptr;

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
    bufptr = (unsigned char *)&buffer[0];
  } else {
    bufptr = (unsigned char *)vovod;
  }

#if 0
  for (uint32_t jx = 0; jx < len; jx++) {
    printf("%02x ", bufptr[jx]);
  }
  printf("\n");
#endif
  
  // Create a byte stream to take from our buffer.
  // Temporary set of our bytestream
  // Get the VOL header.  If we fail, set the bytestream back
  int havevol = 0;
  do {
    try {
      iso->m_pvodec->SetUpBitstreamBuffer(bufptr, len);
      iso->m_pvodec->decodeVOLHead();
      iso->m_pvodec->postVO_VOLHeadInit(iso->m_pvodec->getWidth(),
				   iso->m_pvodec->getHeight(),
				   &iso->m_bSpatialScalability);
      iso_message(LOG_DEBUG, mp4iso, "Found VOL in header");
      if (iso->m_pvodec->fSptUsage() == 2) {
	iso_message(LOG_INFO, mp4iso, "Warning: GMC detected - this reference code does not decode GMC properly - artifacts may occur");
      }
      iso->m_vft->video_configure(iso->m_ifptr, 
				  iso->m_pvodec->getWidth(),
				  iso->m_pvodec->getHeight(),
				  VIDEO_FORMAT_YUV,
				  calculate_aspect_ratio(iso));
      havevol = 1;
    } catch (int err) {
      iso_message(LOG_DEBUG, mp4iso, 
		  "Caught exception in VOL mem header search");
      if (err == 1519) {
	iso_message(LOG_DEBUG, mp4iso, 
		    "Error decoding VOL - video may not play correctly");
      }
    }
    uint32_t used;
    used = iso->m_pvodec->get_used_bytes();
    if (used == 0) used += 4;
    bufptr += used;
    if (len > used) len -= used;
    else len = 0;
  } while (havevol == 0 && len > 0);


  // We've found the VO VOL header - that's good.
  // Reset the byte stream back to what it was, delete our temp stream
  //player_debug_message("Decoded vovod header correctly");
  return havevol;
}

static codec_data_t *iso_create (const char *stream_type,
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
  iso_decode_t *iso;

  iso = MALLOC_STRUCTURE(iso_decode_t);
  if (iso == NULL) return NULL;
  memset(iso, 0, sizeof(*iso));
  iso->m_vft = vft;
  iso->m_ifptr = ifptr;

  iso->m_main_short_video_header = FALSE;
  iso->m_pvodec = new CVideoObjectDecoder();
  iso->m_decodeState = DECODE_STATE_VOL_SEARCH;
  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    // See if we can decode a passed in vovod header
    if (parse_vovod(iso, media_fmt->fmt_param, 1, 0) == 1) {
      iso->m_decodeState = DECODE_STATE_WAIT_I;
    }
  } else if (userdata != NULL) {
    if (parse_vovod(iso, (const char *)userdata, 0, ud_size) == 1) {
      iso->m_decodeState = DECODE_STATE_WAIT_I;
    }
  }
  iso->m_vinfo = vinfo;

  iso->m_num_wait_i = 0;
  iso->m_num_wait_i_frames = 0;
  iso->m_total_frames = 0;
  return (codec_data_t *)iso;
}

void iso_clean_up (iso_decode_t *iso)
{
  if (iso->m_ifile != NULL) {
    fclose(iso->m_ifile);
    iso->m_ifile = NULL;
  }
  if (iso->m_buffer != NULL) {
    free(iso->m_buffer);
    iso->m_buffer = NULL;
  }
  if (iso->m_fpos != NULL) {
    delete iso->m_fpos;
    iso->m_fpos = NULL;
  }
  
  if (iso->m_pvodec) {
    delete iso->m_pvodec;
    iso->m_pvodec = NULL;
  }

  free(iso);
}

static void iso_close (codec_data_t *ptr)
{
  iso_decode_t *iso = (iso_decode_t *)ptr;

  iso_message(LOG_INFO, mp4iso, "MPEG4 codec results:");
  iso_message(LOG_INFO, mp4iso,
	      "total frames    : %u", iso->m_total_frames);
  iso_message(LOG_INFO, mp4iso,
	      "wait for I times: %u", iso->m_num_wait_i);
  iso_message(LOG_INFO, mp4iso,
	      "wait I frames   : %u", iso->m_num_wait_i_frames);

  iso_clean_up(iso);
}


static void iso_do_pause (codec_data_t *ptr)
{
  iso_decode_t *iso = (iso_decode_t *)ptr;
  if (iso->m_decodeState != DECODE_STATE_VOL_SEARCH)
    iso->m_decodeState = DECODE_STATE_WAIT_I;
}
static int iso_frame_is_sync (codec_data_t *ptr,
			      uint8_t *buffer, 
			      uint32_t buflen,
			      void *userdata)
{
  u_char vop_type;

  while (buflen > 3 && 
	 !(buffer[0] == 0 && buffer[1] == 0 && 
	   buffer[2] == 1 && buffer[3] == MP4AV_MPEG4_VOP_START)) {
    buffer++;
    buflen--;
  }

  vop_type = MP4AV_Mpeg4GetVopType(buffer, buflen);
#if 0
  {
  iso_decode_t *iso = (iso_decode_t *)ptr;
  iso_message(LOG_DEBUG, "iso", "return from get vop is %c %d", vop_type, vop_type);
  }
#endif
  if (vop_type == VOP_TYPE_I) return 1;
  return 0;
}

static int iso_decode (codec_data_t *ptr, 
		       frame_timestamp_t *ts, 
		       int from_rtp, 
		       int *sync_frame,
		       uint8_t *buffer,
		       uint32_t buflen,
		       void *userdata)
{
  Int iEof = 1;
  iso_decode_t *iso = (iso_decode_t *)ptr;
  uint32_t used = 0;

  if (buflen <= 4) return -1;

  //  iso_message(LOG_DEBUG, "iso", "frame %d", iso->m_total_frames);
  iso->m_total_frames++;
  buffer[buflen] = 0;
  buffer[buflen + 1] = 0;
  buffer[buflen + 2] = 1;

  switch (iso->m_decodeState) {
  case DECODE_STATE_VOL_SEARCH: {
    if (buffer[0] == 0 &&
	buffer[1] == 0 &&
	(buffer[2] & 0xfc) == 0x80 &&
	(buffer[3] & 0x03) == 0x02) {
      // we have the short header
      iso->m_short_header = 1;
      iso->m_pvodec->SetUpBitstreamBuffer((unsigned char *)buffer, buflen);
      iso->m_pvodec->video_plane_with_short_header();
      iso->m_pvodec->postVO_VOLHeadInit(iso->m_pvodec->getWidth(),
					iso->m_pvodec->getHeight(),
					&iso->m_bSpatialScalability);
      iso_message(LOG_INFO, mp4iso, "Decoding using short headers");
      iso->m_vft->video_configure(iso->m_ifptr, 
				  iso->m_pvodec->getWidth(),
				  iso->m_pvodec->getHeight(),
				  VIDEO_FORMAT_YUV,
				  calculate_aspect_ratio(iso));
      iso->m_decodeState = DECODE_STATE_NORMAL;
      try {
	iEof = iso->m_pvodec->h263_decode(FALSE);
      } catch (...) {
	iso_message(LOG_ERR, mp4iso, "Couldn't decode h263 in vol search");
      }
      break; 
    } else {
      uint8_t *volhdr = MP4AV_Mpeg4FindVol(buffer, buflen);
      if (volhdr != NULL) {
	used = volhdr - buffer;
	try {
	  iso->m_pvodec->SetUpBitstreamBuffer((unsigned char *)volhdr, buflen - used);
	  iso->m_pvodec->decodeVOLHead();
	  iso->m_pvodec->postVO_VOLHeadInit(iso->m_pvodec->getWidth(),
					    iso->m_pvodec->getHeight(),
					  &iso->m_bSpatialScalability);
	  iso_message(LOG_INFO, mp4iso, "Found VOL");
	
	  iso->m_vft->video_configure(iso->m_ifptr, 
				      iso->m_pvodec->getWidth(),
				      iso->m_pvodec->getHeight(),
				      VIDEO_FORMAT_YUV,
				      calculate_aspect_ratio(iso));
	
	  iso->m_decodeState = DECODE_STATE_WAIT_I;
	  used += iso->m_pvodec->get_used_bytes();
	} catch (int err) {
	  iso_message(LOG_DEBUG, mp4iso, "Caught exception in VOL search %d", err);
	  if (err == 1) used = buflen;
	  else used += iso->m_pvodec->get_used_bytes();
	}
      }
    }
    if (iso->m_decodeState != DECODE_STATE_WAIT_I) {
      if (iso->m_vinfo != NULL) {
	iso->m_pvodec->FakeOutVOVOLHead(iso->m_vinfo->height,
					iso->m_vinfo->width,
					30,
					&iso->m_bSpatialScalability);
	iso->m_vft->video_configure(iso->m_ifptr, 
				    iso->m_vinfo->width,
				    iso->m_vinfo->height,
				    VIDEO_FORMAT_YUV,
				    calculate_aspect_ratio(iso));

	iso->m_decodeState = DECODE_STATE_NORMAL;
      } 

      return used;
    }
    // else fall through
  }
  case DECODE_STATE_WAIT_I: {
    uint8_t *vophdr = MP4AV_Mpeg4FindVop(buffer, buflen);
    if (vophdr != NULL) {
      used = vophdr - buffer;
    }
    iso->m_pvodec->SetUpBitstreamBuffer((unsigned char *)buffer + used, buflen + 3 - used);
    try {
      iEof = iso->m_pvodec->decode(NULL, TRUE);
      if (iEof == -1) {
	iso->m_num_wait_i_frames++;
	return(iso->m_pvodec->get_used_bytes());
      }
      iso_message(LOG_DEBUG, mp4iso, "Back to normal decode");
      iso->m_decodeState = DECODE_STATE_NORMAL;
      iso->m_bCachedRefFrame = FALSE;
      iso->m_bCachedRefFrameCoded = FALSE;
      iso->m_cached_valid = FALSE;
      iso->m_cached_time = 0;
    } catch (int err) {
      if (err != 1)
	iso_message(LOG_DEBUG, mp4iso, 
		    "ts "U64",Caught exception in wait_i %d", 
		    ts->msec_timestamp, err);
      return (iso->m_pvodec->get_used_bytes());
      //return (-1);
    }
    break;
  }
  case DECODE_STATE_NORMAL:
    try {
      if (iso->m_short_header != 0) {
	iso->m_pvodec->SetUpBitstreamBuffer((unsigned char *)buffer, buflen + 3);
	iEof = iso->m_pvodec->h263_decode(TRUE);
      } else {
	uint8_t *vophdr = MP4AV_Mpeg4FindVop(buffer, buflen);
	if (vophdr != NULL && vophdr != buffer) {
	  iso_message(LOG_DEBUG, mp4iso, "Illegal code before VOP header");
	  used = vophdr - buffer;
	  buflen -= used;
	  buffer = vophdr;
	}
	iso->m_pvodec->SetUpBitstreamBuffer((unsigned char *)buffer, buflen + 3);
	iEof = iso->m_pvodec->decode(NULL, FALSE, FALSE);
      }
    } catch (int err) {
      // This is because sometimes, the encoder doesn't read all the bytes
      // it should out of the rtp packet.  The rtp bytestream does a read
      // and determines that we're trying to read across bytestreams.
      // If we get this, we don't want to change anything - just fall up
      // to the decoder thread so it gives us a new timestamp.
      if (err == 1) {
	// throw from running past end of frame
	return -1;
      }
      iso_message(LOG_DEBUG, mp4iso, 
		  "Mpeg4 ncaught %d -> waiting for I", err);
      iso->m_decodeState = DECODE_STATE_WAIT_I;
      return (iso->m_pvodec->get_used_bytes());
    } catch (...) {
      iso_message(LOG_DEBUG, mp4iso, 
		  "Mpeg4 ncaught -> waiting for I");
      iso->m_decodeState = DECODE_STATE_WAIT_I;
      //return (-1);
      return (iso->m_pvodec->get_used_bytes());
    }
    break;
  }

  /*
   * We've got a good frame.  See if we need to display it
   */
  const CVOPU8YUVBA *pvopcQuant = NULL;
  if (iso->m_pvodec->fSptUsage() == 1) {
    //player_debug_message("Sprite");
  }
  uint64_t displaytime = 0;
  int cached_ts = 0;
  if (iEof == EOF) {
    if (iso->m_bCachedRefFrame) {
      iso->m_bCachedRefFrame = FALSE;
      if (iso->m_bCachedRefFrameCoded) {
	pvopcQuant = iso->m_pvodec->pvopcRefQLater();
	displaytime = ts->msec_timestamp;
      }
    }
  } else {
#if 0
    iso_message(LOG_DEBUG, mp4iso, "frame "U64" type %d", 
		ts->msec_timestamp, iso->m_pvodec->vopmd().vopPredType);
#endif
    if (iso->m_pvodec->vopmd().vopPredType == BVOP) {
      if (iEof != FALSE) {
	pvopcQuant = iso->m_pvodec->pvopcReconCurr();
	displaytime = ts->msec_timestamp;
      } 
    } else {
      if (iso->m_bCachedRefFrame) {
	iso->m_bCachedRefFrame = FALSE;
	if (iso->m_bCachedRefFrameCoded) {
	  pvopcQuant = iso->m_pvodec->pvopcRefQPrev();
	  if (ts->timestamp_is_pts) {
	    int old_was_valid = iso->m_cached_valid;
	    displaytime = iso->m_cached_time;
	    cached_ts = 1;
	    // old time stamp wasn't valid - instead of calculating it
	    // ourselves, just punt on it.
	    if (old_was_valid == 0) {
	      return (iEof == EOF ? -1 : 0);
	    }
	  } else {
	    displaytime = ts->msec_timestamp;
	  }
	}
      }

      iso->m_cached_time = ts->msec_timestamp;
      iso->m_cached_valid = TRUE;
      iso->m_bCachedRefFrame = TRUE;
      iso->m_bCachedRefFrameCoded = (iEof != FALSE);
    }
  }

  if (pvopcQuant != NULL) {
#if 0
    player_debug_message("frame rtp_ts "U64" disp "U64" cached %d", 
			 ts->msec_timestamp, displaytime, cached_ts);
#endif
    /*
     * Get the information to the video sync structure
     */
    const uint8_t *y, *u, *v;
    int pixelw_y, pixelw_uv;
    pixelw_y =  pvopcQuant->getPlane(Y_PLANE)->where().width;
    pixelw_uv = pvopcQuant->getPlane(U_PLANE)->where().width;

    y = (const uint8_t *)pvopcQuant->getPlane(Y_PLANE)->pixels(0,0);
    u = (const uint8_t *)pvopcQuant->getPlane(U_PLANE)->pixels(0,0);
    v = (const uint8_t *)pvopcQuant->getPlane(V_PLANE)->pixels(0,0);
    iso->m_last_time = displaytime;
#if 0
    player_debug_message("Adding video at "U64" %d", displaytime,
			 iso->m_pvodec->vopmd().vopPredType);
#endif

    iso->m_vft->video_have_frame(iso->m_ifptr, 
				y, 
				u, 
				v, 
				pixelw_y, 
				pixelw_uv, 
				displaytime);
  } else {
    iso_message(LOG_DEBUG, mp4iso, "decode but no frame "U64, ts->msec_timestamp);
  }
  return (iso->m_pvodec->get_used_bytes() + used);
}


static int iso_skip_frame (codec_data_t *iso)
{
#if 0
  return (iso_decode(iso, ts, 0, NULL, buffer, buflen));
#else
  return 0;
#endif
}
static const char *iso_compressors[] = {
  "mp4 ", 
  "mp4v",
  "encv",
  "divx", 
  "dvx1", 
  "div4", 
  "mpeg4", 
  "xvid",
  NULL,
};

static int iso_codec_check (lib_message_func_t message,
			    const char *stream_type,
			    const char *compressor,
			    int type,
			    int profile,
			    format_list_t *fptr,
			    const uint8_t *userdata,
			    uint32_t userdata_size,
			    CConfigSet *pConfig)
{
  int ret_val = -1;

  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) {
    if (strcasecmp(compressor, "mp4v") == 0 ||
	strcasecmp(compressor, "encv") == 0 &&
	(type == MP4_MPEG4_VIDEO_TYPE || type == MP4_H263_VIDEO_TYPE)) {
      ret_val =  1;
    }
  }

  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    // find format. If matches, call parse_fmtp_for_mpeg4, look at
    // profile level.
#if 1
    if (strcmp(fptr->fmt, "34") == 0) {
      ret_val =  1;
    }
#endif
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "MP4V-ES") == 0 || 
          strcasecmp(fptr->rtpmap_name, "enc-mpeg4-generic") == 0) {
	ret_val =  1;
      }
    }
  }

  if (compressor != NULL) {
    const char **lptr = iso_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	ret_val =  1;
	break;
      }
      lptr++;
    }
  }
  if (ret_val == 1 && pConfig->GetBoolValue(CONFIG_USE_MPEG4_ISO_ONLY)) {
    
    ret_val = 255;
    message(LOG_DEBUG, "mpeg4iso", "Asserting mpeg4 iso only");
  }
  return ret_val;
}

VIDEO_CODEC_WITH_RAW_FILE_PLUGIN("MPEG4 ISO", 
				 iso_create,
				 iso_do_pause,
				 iso_decode,
				 NULL,
				 iso_close,
				 iso_codec_check,
				 iso_frame_is_sync,
				 mpeg4_iso_file_check,
				 divx_file_next_frame,
				 divx_file_used_for_frame,
				 divx_file_seek_to,
				 iso_skip_frame,
				 divx_file_eof,
				 MyConfigVariables,
				 sizeof(MyConfigVariables) / 
				 sizeof(*MyConfigVariables));
