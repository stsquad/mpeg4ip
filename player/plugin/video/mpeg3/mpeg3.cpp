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
extern "C" {
#include <mpeg3protos.h>
}

//#define DEBUG_MPEG3_FRAME 1

static codec_data_t *mpeg3_create (format_list_t *media_fmt,
				   video_info_t *vinfo,
				   const unsigned char *userdata,
				   uint32_t ud_size,
				   video_vft_t *vft,
				   void *ifptr)
{
  mpeg3_codec_t *mpeg3;

  mpeg3 = (mpeg3_codec_t *)malloc(sizeof(mpeg3_codec_t));
  memset(mpeg3, 0, sizeof(*mpeg3));

  mpeg3->m_vft = vft;
  mpeg3->m_ifptr = ifptr;

  mpeg3->m_video = mpeg3video_new(NULL, 
#ifdef USE_MMX
				  1,
#else
				  0,
#endif
				  0, 
				  1);

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
}

static int mpeg3_decode (codec_data_t *ptr,
			uint64_t ts, 
			int from_rtp,
			int *sync_frame,
			unsigned char *buffer, 
			uint32_t buflen)
{
  int ret;
  mpeg3_codec_t *mpeg3 = (mpeg3_codec_t *)ptr;
  mpeg3video_t *video;
  video = mpeg3->m_video;

  buffer[buflen] = 0;
  buffer[buflen + 1] = 0;
  buffer[buflen + 2] = 1;
  buffer[buflen + 3] = 0;

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
      mpeg3bits_use_ptr_len(video->vstream, buffer, buflen + 3);
    } else 
      return buflen;
  }

  char *y, *u, *v;

  y = NULL;
  ret = mpeg3video_read_yuvframe_ptr(video,
				     0, 
				     &y,
				     &u,
				     &v);
  if (ret == 0 && y != NULL) {
#ifdef DEBUG_MPEG3_FRAME
    mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "frame %llu decoded", 
			  ts);
#endif
    ret = mpeg3->m_vft->video_have_frame(mpeg3->m_ifptr,
					 (unsigned char *)y, 
					 (unsigned char *)u, 
					 (unsigned char *)v, 
					 mpeg3->m_w, mpeg3->m_w / 2, ts);
  } else {
#ifdef DEBUG_MPEG3_FRAME
    mpeg3->m_vft->log_msg(LOG_DEBUG, "mpeg3", "frame %llu ret %d %p", 
			  ts, ret, y);
#endif
  }
    

  return (buflen);
}

static int mpeg3_codec_check (lib_message_func_t message,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const unsigned char *userdata,
			     uint32_t userdata_size)
{
  if (fptr != NULL) {
    if (strcmp(fptr->fmt, "32") == 0) {
      return 1;
    }
  }
  return -1;
}

VIDEO_CODEC_PLUGIN("mpeg3", 
		   mpeg3_create,
		   mpeg3_do_pause,
		   mpeg3_decode,
		   mpeg3_close,
		   mpeg3_codec_check);
