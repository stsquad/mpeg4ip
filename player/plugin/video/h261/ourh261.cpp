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
 * raw video codec
 */

#include "ourh261.h"
#include "h261_userdata.h"
//#define DEBUG_H261_FRAME 1

static codec_data_t *h261_create (const char *stream_type,
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
  h261_codec_t *h261;

  h261 = (h261_codec_t *)malloc(sizeof(h261_codec_t));
  memset(h261, 0, sizeof(*h261));

  h261->m_vft = vft;
  h261->m_ifptr = ifptr;

  h261->m_decoder = NULL;
  return ((codec_data_t *)h261);
}


static void h261_close (codec_data_t *ifptr)
{
  h261_codec_t *h261;

  h261 = (h261_codec_t *)ifptr;
  if (h261->m_decoder) {
    delete h261->m_decoder;
    h261->m_decoder = NULL;
  }
  free(h261);
}


static void h261_do_pause (codec_data_t *ifptr)
{
  h261_codec_t *h261 = (h261_codec_t *)ifptr;
  h261->m_did_pause = 1;
}

static int h261_frame_is_sync (codec_data_t *ifptr, 
				uint8_t *buffer, 
				uint32_t buflen,
				void *userdata)
{

  return 1;
}

static int h261_decode (codec_data_t *ptr,
			frame_timestamp_t *ts, 
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer, 
			uint32_t buflen,
			 void *ud)
{
  h261_codec_t *h261 = (h261_codec_t *)ptr;
  const uint8_t *y, *u, *v;
  h261_rtp_userdata_t *h261_data = (h261_rtp_userdata_t *)ud;

#if 0
  h261->m_vft->log_msg(LOG_DEBUG, "h261", "ts "U64", m bit %d", 
		       ts, h261_data->m_bit_value);
#endif
  if (h261->m_decoder == NULL) {
    if ((buffer[0] & 0x2) != 0) {
      h261->m_decoder = new IntraP64Decoder();
      h261->m_vft->log_msg(LOG_DEBUG, "h261", "starting intra decoder");
    } else {
      h261->m_decoder = new FullP64Decoder();
      h261->m_vft->log_msg(LOG_DEBUG, "h261", "starting full decoder");
    }
  }
  u_int32_t read = ntohl(*(uint32_t *)(buffer));
  int sbit = read >> 29;
  int ebit = (read >> 26) & 7;
  int quant = (read >> 10) & 0x1f;
  int mvdh = (read >> 5) & 0x1f;
  int mvdv = read & 0x1f;
  int mba, gob;
  mba = (read >> 15) & 0x1f;
  gob = (read >> 20) & 0xf;

  h261->m_decoder->decode(buffer + 4, buflen - 4, sbit, ebit, mba, gob, 
			   quant, mvdh, mvdv);

  if (h261->m_initialized == 0) {
    h261->m_h = h261->m_decoder->height();
    h261->m_w = h261->m_decoder->width();
    h261->m_vft->log_msg(LOG_DEBUG, "h261", "h %d w %d", 
			 h261->m_h, h261->m_w);
    h261->m_vft->video_configure(h261->m_ifptr, 
				 h261->m_w,
				 h261->m_h,
				 VIDEO_FORMAT_YUV,
				 0.0);
    h261->m_initialized = 1;
  }

  
  if (h261_data->m_bit_value != 0) {
    h261->m_decoder->sync();
    y = h261->m_decoder->frame();
    u = y + (h261->m_w * h261->m_h);
    v = u + ((h261->m_w * h261->m_h) / 4);
    

#if 0
    h261->m_vft->log_msg(LOG_DEBUG, "h261", "filling buffer ts "U64, ts);
#endif
    h261->m_vft->video_have_frame(h261->m_ifptr,
				  (const uint8_t *)y, 
				  (const uint8_t *)u, 
				  (const uint8_t *)v, 
				  h261->m_w, h261->m_w / 2, 
				  ts->msec_timestamp);
    h261->m_decoder->resetndblk();
  } 
  free(h261_data);

  return (buflen);
}

static int h261_codec_check (lib_message_func_t message,
			     const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			     CConfigSet *pConfig)
{
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    if (strcmp(fptr->fmt, "31") == 0) {
      return 1;
    }
  }
  return -1;
}

VIDEO_CODEC_PLUGIN("h261", 
		   h261_create,
		   h261_do_pause,
		   h261_decode,
		   NULL,
		   h261_close,
		   h261_codec_check,
		   h261_frame_is_sync,
		   NULL,
		   0);
