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

#include "rawv.h"
#include <mp4v2/mp4.h>


static codec_data_t *rawv_create (const char *stream_type,
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
  rawv_codec_t *rawv;

  if (vinfo == NULL) return NULL;

  rawv = (rawv_codec_t *)malloc(sizeof(rawv_codec_t));
  rawv->m_vft = vft;
  rawv->m_ifptr = ifptr;

  rawv->m_h = vinfo->height;
  rawv->m_w = vinfo->width;

  rawv->m_vft->video_configure(rawv->m_ifptr, 
			       vinfo->width,
			       vinfo->height,
			       VIDEO_FORMAT_YUV,
			       0.0);
  return ((codec_data_t *)rawv);
}


static void rawv_close (codec_data_t *ifptr)
{
  rawv_codec_t *rawv;

  rawv = (rawv_codec_t *)ifptr;

  free(rawv);
}



static void rawv_do_pause (codec_data_t *ifptr)
{
}

static int rawv_video_frame_is_sync (codec_data_t *ptr,
				     uint8_t *buffer, 
				     uint32_t buflen,
				     void *ud)
{
  return 1;
}

static int rawv_decode (codec_data_t *ptr,
			frame_timestamp_t *ts, 
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer, 
			uint32_t buflen,
			void *ud)
{
  rawv_codec_t *rawv = (rawv_codec_t *)ptr;

  uint32_t len;

  len = rawv->m_h * rawv->m_w;  
  uint8_t *y, *u, *v;
  y = buffer;
  u = buffer + len;
  v = u + (len / 4);
  if (len + (len / 2) != buflen) {
     rawv->m_vft->log_msg(LOG_ERR, "rawv", 
			  "error - raw video buflen not right len %d h %d w %d",
			  len, rawv->m_h, rawv->m_w);
     return -1;
  }
                       
  rawv->m_vft->video_have_frame(rawv->m_ifptr,
				y, u, v, rawv->m_w, rawv->m_w / 2, 
				ts->msec_timestamp);

  return (buflen);
}


static const char *rawv_compressors[] = {
  "i420", 
  NULL,
};

static int rawv_codec_check (lib_message_func_t message,
			     const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			     CConfigSet *pConfig)
{
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) {
    if (type == MP4_YUV12_VIDEO_TYPE) {
      return 1;
    }
    return -1;
  }

  if (compressor != NULL) {
    const char **lptr = rawv_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 1;
      }
      lptr++;
    }
  }
  return -1;
}

VIDEO_CODEC_PLUGIN("rawv", 
		   rawv_create,
		   rawv_do_pause,
		   rawv_decode,
		   NULL, 
		   rawv_close,
		   rawv_codec_check,
		   rawv_video_frame_is_sync,
		   NULL, 
		   0);
