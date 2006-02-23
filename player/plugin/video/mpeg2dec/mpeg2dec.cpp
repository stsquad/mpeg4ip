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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg2 video codec with libmpeg2 library
 */
#include "mpeg2dec.h"
#include "mp4av.h"
#include <mpeg2t/mpeg2t_defines.h>
#include <mpeg2ps/mpeg2_ps.h>

//#define DEBUG_MPEG2DEC_FRAME 1

static codec_data_t *mpeg2dec_create (const char *stream_type,
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
  mpeg2dec_codec_t *mpeg2dec;

  mpeg2dec = MALLOC_STRUCTURE(mpeg2dec_codec_t);
  memset(mpeg2dec, 0, sizeof(*mpeg2dec));

  mpeg2dec->m_vft = vft;
  mpeg2dec->m_ifptr = ifptr;

  mpeg2dec->m_decoder = mpeg2_init();

  mpeg2dec->m_did_pause = 1;
  return ((codec_data_t *)mpeg2dec);
}


static void mpeg2dec_close (codec_data_t *ifptr)
{
  mpeg2dec_codec_t *mpeg2dec;

  mpeg2dec = (mpeg2dec_codec_t *)ifptr;
  if (mpeg2dec->m_decoder) {
    mpeg2_close(mpeg2dec->m_decoder);
    mpeg2dec->m_decoder = NULL;
  }
  free(mpeg2dec);
}


static void mpeg2dec_do_pause (codec_data_t *ifptr)
{
  mpeg2dec_codec_t *mpeg2dec = (mpeg2dec_codec_t *)ifptr;
  mpeg2_reset(mpeg2dec->m_decoder, 0);
  mpeg2dec->m_did_pause = 1;
  mpeg2dec->m_got_i = 0;
}

static int mpeg2dec_frame_is_sync (codec_data_t *ifptr, 
				uint8_t *buffer, 
				uint32_t buflen,
				void *userdata)
{
  int ret;
  int ftype;
  ret = MP4AV_Mpeg3FindPictHdr(buffer, buflen, &ftype);
  if (ret >= 0 && ftype == 1) {
    mpeg2dec_do_pause(ifptr);
    return 1;
  }
  return 0;
}

static int mpeg2dec_decode (codec_data_t *ptr,
			    frame_timestamp_t *pts, 
			    int from_rtp,
			    int *sync_frame,
			    uint8_t *buffer, 
			    uint32_t buflen,
			    void *ud)
{
  mpeg2dec_codec_t *mpeg2dec = (mpeg2dec_codec_t *)ptr;
  mpeg2dec_t *decoder;
  const mpeg2_info_t *info;
  mpeg2_state_t state;
  uint64_t ts = pts->msec_timestamp;

  decoder = mpeg2dec->m_decoder;
  
#if 0
  mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "ts buflen %d "U64, buflen, ts);
  //if (mpeg2dec->m_did_pause != 0) 
 {
    for (uint32_t ix = 0; ix < buflen + 3; ix++) {
      if (buffer[ix] == 0 &&
	  buffer[ix + 1] == 0 &&
	  buffer[ix + 2] == 1) {
	mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "index %d - value %x %x %x", 
			      ix, buffer[ix + 3], buffer[ix + 4],
			      buffer[ix + 5]);
      }
    }
  }
#endif

 info = mpeg2_info(decoder);
 bool passed_buffer = false;
 bool finished_buffer = false;
 do {
   state = mpeg2_parse(decoder);
   //mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "state %d", state);
   const mpeg2_sequence_t *sequence;
   sequence = info->sequence;
   switch (state) {
   case STATE_BUFFER:
     if (passed_buffer == false) {
       mpeg2_buffer(decoder, buffer, buffer + buflen);
       passed_buffer = true;
     } else {
       finished_buffer = true;
     } 
     break;
   case STATE_SEQUENCE: {
     if (mpeg2dec->m_video_initialized == 0) {
       mpeg2dec->m_h = sequence->height;
       mpeg2dec->m_w = sequence->width;
       int have_mpeg2;
       uint32_t height;
       uint32_t width;
       double frame_rate;
       double bitrate;
       double aspect_ratio;
       uint8_t profile;
       if (MP4AV_Mpeg3ParseSeqHdr(buffer, 
				  buflen,
				  &have_mpeg2, 
				  &height, 
				  &width, 
				  &frame_rate,
				  &bitrate, 
				  &aspect_ratio,
				  &profile) < 0) {
	 
	 mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "pix w %u pix h %u", 
				  sequence->pixel_width, 
				  sequence->pixel_height);
	 aspect_ratio = sequence->pixel_width;
	 aspect_ratio *= mpeg2dec->m_w;
	 aspect_ratio /= (double)(sequence->pixel_height * mpeg2dec->m_h);
       }
       mpeg2dec->pts_convert.frame_rate = frame_rate;
       mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "%ux%u aspect %g", 
				mpeg2dec->m_w, mpeg2dec->m_h, 
				aspect_ratio);
       mpeg2dec->m_vft->video_configure(mpeg2dec->m_ifptr, 
					mpeg2dec->m_w,
					mpeg2dec->m_h,
					VIDEO_FORMAT_YUV,
					aspect_ratio);
       mpeg2dec->m_video_initialized = 1;
     }
     break;
    }
   case STATE_SLICE:
   case STATE_END:
   case STATE_INVALID_END:  
     // INVALID_END state means they found a new sequence header, with a 
     // new size
     
#ifdef DEBUG_MPEG2DEC_FRAME
     mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "frame "U64" decoded", 
			  mpeg2dec->cached_ts);
#endif
     if (info->display_fbuf) {
       mpeg2dec->m_vft->video_have_frame(mpeg2dec->m_ifptr,
					 info->display_fbuf->buf[0],
					 info->display_fbuf->buf[1],
					 info->display_fbuf->buf[2],
					 sequence->width, 
					 sequence->chroma_width,
					 mpeg2dec->m_cached_ts_invalid ? ts :
					 mpeg2dec->cached_ts);
     }
     break;
   case STATE_SEQUENCE_REPEATED: // we don't care here
   case STATE_GOP: 
   case STATE_PICTURE:
   case STATE_SLICE_1ST:
   case STATE_PICTURE_2ND:
   case STATE_INVALID: //
   default:
     break;
   } 
 } while (finished_buffer == false);
 
 mpeg2dec->m_cached_ts_invalid = false;
 if (pts->timestamp_is_pts) {
   if (info->current_picture == NULL ||
       mpeg3_find_dts_from_pts(&mpeg2dec->pts_convert,
			       ts,
			       info->current_picture->flags & PIC_MASK_CODING_TYPE, 
			       info->current_picture->temporal_reference,
			       &mpeg2dec->cached_ts) < 0) {
     mpeg2dec->m_cached_ts_invalid = true;
   }
#if 0
   mpeg2dec->m_vft->log_msg(LOG_DEBUG, "mpeg2dec", "pts "U64" dts "U64" temp %u type %u", 
			    pts->msec_timestamp, mpeg2dec->cached_ts, 
			    info->current_picture->temporal_reference, 
			    info->current_picture->flags & PIC_MASK_CODING_TYPE);
#endif

 } else {
   mpeg2dec->cached_ts = ts;
 }
 return (buflen);
}

static int mpeg2dec_codec_check (lib_message_func_t message,
				 const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			      CConfigSet *pConfig)
{
  int ret_val = 3;

  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    if (strcmp(fptr->fmt, "32") == 0) {
      return ret_val;
    }
  }
  if (strcasecmp(stream_type, STREAM_TYPE_MPEG_FILE) == 0) {
    if (type == MPEG_VIDEO_MPEG1 || type == MPEG_VIDEO_MPEG2)
      return ret_val;
  }
  if (strcasecmp(stream_type, STREAM_TYPE_MPEG2_TRANSPORT_STREAM) == 0) {
    if ((type == MPEG2T_ST_MPEG_VIDEO) ||
	(type == MPEG2T_ST_11172_VIDEO)) 
      return ret_val;
  }
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) {
    if (strcasecmp(compressor, "mp4v") == 0 &&
	(MP4_IS_MPEG1_VIDEO_TYPE(type) ||
	 MP4_IS_MPEG2_VIDEO_TYPE(type))) return ret_val;
  }
  return -1;
}

VIDEO_CODEC_PLUGIN("mpeg2dec", 
		   mpeg2dec_create,
		   mpeg2dec_do_pause,
		   mpeg2dec_decode,
		   NULL,
		   mpeg2dec_close,
		   mpeg2dec_codec_check,
		   mpeg2dec_frame_is_sync,
		   NULL,
		   0);
