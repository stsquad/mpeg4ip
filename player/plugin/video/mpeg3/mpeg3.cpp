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
 * raw video codec
 */

#include "mpeg3.h"
#include <mpeg3videoprotos.h>
#include <bitstream.h>
#include "mp4av.h"

//#define DEBUG_MPEG3_FRAME 1

static codec_data_t *mpeg3_create (format_list_t *media_fmt,
				   video_info_t *vinfo,
				   const uint8_t *userdata,
				   uint32_t ud_size,
				   video_vft_t *vft,
				   void *ifptr)
{
  mpeg3_codec_t *mpeg3;

  mpeg3 = (mpeg3_codec_t *)malloc(sizeof(mpeg3_codec_t));
  memset(mpeg3, 0, sizeof(*mpeg3));

  mpeg3->m_vft = vft;
  mpeg3->m_ifptr = ifptr;

  mpeg3->m_video = mpeg3video_new(0, 1);

  mpeg3->m_did_pause = 1;
  return ((codec_data_t *)mpeg3);
}


static void mpeg3_close (codec_data_t *ifptr)
{
  mpeg3_codec_t *mpeg3;

  mpeg3 = (mpeg3_codec_t *)ifptr;
  if (mpeg3->m_video) {
    mpeg3video_delete(mpeg3->m_video);
    mpeg3->m_video = NULL;
  }
  free(mpeg3);
}


static void mpeg3_do_pause (codec_data_t *ifptr)
{
  mpeg3_codec_t *mpeg3 = (mpeg3_codec_t *)ifptr;
  mpeg3->m_did_pause = 1;
  mpeg3->m_got_i = 0;
  mpeg3->m_video->repeat_count = mpeg3->m_video->current_repeat = 0;
}

static int mpeg3_frame_is_sync (codec_data_t *ifptr, 
				uint8_t *buffer, 
				uint32_t buflen,
				void *userdata)
{
  int ret;

  ret = MP4AV_Mpeg3FindGopOrPictHdr(buffer, buflen, NULL);
  if (ret >= 0) {
    mpeg3_do_pause(ifptr);
    return 1;
  }
  return 0;
}

static int mpeg3_decode (codec_data_t *ptr,
			uint64_t ts, 
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer, 
			uint32_t buflen,
			 void *ud)
{
  int ret;
  int render = 1;
  mpeg3_codec_t *mpeg3 = (mpeg3_codec_t *)ptr;
  mpeg3video_t *video;
  video = mpeg3->m_video;

  buffer[buflen] = 0;
  buffer[buflen + 1] = 0;
  buffer[buflen + 2] = 1;
  buffer[buflen + 3] = 0;

#if 0
  mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "ts %llu", ts);
  //if (mpeg3->m_did_pause != 0) 
 {
    for (uint32_t ix = 0; ix < buflen + 3; ix++) {
      if (buffer[ix] == 0 &&
	  buffer[ix + 1] == 0 &&
	  buffer[ix + 2] == 1) {
	mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "index %d - value %x %x %x", 
			      ix, buffer[ix + 3], buffer[ix + 4],
			      buffer[ix + 5]);
      }
    }
  }
#endif
	
  mpeg3bits_use_ptr_len(video->vstream, buffer, buflen + 3);
  if (video->decoder_initted == 0) {
    mpeg3video_get_header(video, 1);
    if (video->found_seqhdr != 0) {
      mpeg3video_initdecoder(video);
      video->decoder_initted = 1;
      mpeg3->m_h = video->vertical_size;
      mpeg3->m_w = video->horizontal_size;
      mpeg3->m_vft->video_configure(mpeg3->m_ifptr, 
				    mpeg3->m_w,
				    mpeg3->m_h,
				    VIDEO_FORMAT_YUV);
      // Gross and disgusting, but it looks like it didn't clean up
      // properly - so just start from beginning of buffer and decode.
    } else {
      mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "didnt find seq header in frame %llu", ts);
      return buflen;
    }
    mpeg3->m_did_pause = 0;
  } else {
    if (mpeg3->m_did_pause) {
      if (mpeg3->m_got_i == 0) {
	int ret;
 	ret = MP4AV_Mpeg3FindGopOrPictHdr(buffer, buflen, NULL);

	if (ret >= 0) {
	  mpeg3->m_got_i = 1;
	  render = 0;
	} else
	  return (buflen);
      } else {
	mpeg3->m_got_i++;
	if (mpeg3->m_got_i == 4) {
	  mpeg3->m_did_pause = 0;
	} else {
	  render = 0;
	}
      }
    }
  }
  char *y, *u, *v;

#if 0
  if (from_rtp) {
    int ftype;
    ret = MP4AV_Mpeg3FindGopOrPictHdr(buffer, buflen, &ftype);
    if (ret <= 0) {
      mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "frame %llu - type %d", 
			    ts, ftype);
    } else {
      mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "frame %llu - return %d", 
			    ts, ret);
    }
      
  }
#endif
      

    
  y = NULL;
  ret = mpeg3video_read_yuvframe_ptr(video,
				     buffer,
				     buflen + 3,
				     &y,
				     &u,
				     &v);

  if (ret == 0 && y != NULL && render != 0) {
#ifdef DEBUG_MPEG3_FRAME
    mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "frame %llu decoded", 
			  ts);
#endif
    ret = mpeg3->m_vft->video_have_frame(mpeg3->m_ifptr,
					 (const uint8_t *)y, 
					 (const uint8_t *)u, 
					 (const uint8_t *)v, 
					 mpeg3->m_w, mpeg3->m_w / 2, 
					 mpeg3->cached_ts);
    mpeg3->cached_ts = ts;
  } else {
#ifdef DEBUG_MPEG3_FRAME
    if (render == 0) {
      mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "skip render");
    }
    mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "frame %llu ret %d %p", 
			  ts, ret, y);
#endif
    mpeg3->cached_ts = ts;
  }
    

  return (buflen);
}

static int mpeg3_codec_check (lib_message_func_t message,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size)
{
  if (fptr != NULL) {
    if (strcmp(fptr->fmt, "32") == 0) {
      return 1;
    }
  }
  if (compressor != NULL && strcmp(compressor, "MPEG FILE") == 0) {
    return 1;
  }
  return -1;
}

VIDEO_CODEC_PLUGIN("mpeg3", 
		   mpeg3_create,
		   mpeg3_do_pause,
		   mpeg3_decode,
		   mpeg3_close,
		   mpeg3_codec_check,
		   mpeg3_frame_is_sync);
