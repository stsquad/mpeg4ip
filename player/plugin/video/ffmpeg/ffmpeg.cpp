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
#define DECLARE_CONFIG_VARIABLES
#include "ffmpeg.h"
#include "mp4av.h"
#include "mp4av_h264.h"
#include <mpeg2t/mpeg2t_defines.h>

static SConfigVariable MyConfigVariables[] = {
  CONFIG_BOOL(CONFIG_USE_FFMPEG, "UseFFmpeg", false),
};

#define ffmpeg_message (ffmpeg->m_vft->log_msg)

//#define DEBUG_FFMPEG_FRAME 1
static enum CodecID ffmpeg_find_codec (const char *stream_type,
				       const char *compressor, 
				       int type, 
				       int profile, 
				       format_list_t *fptr,
				       const uint8_t *userdata,
				       uint32_t ud_size)
{
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) {
    if (strcasecmp(compressor, "avc1") == 0) {
      return CODEC_ID_H264;
    }
    if (strcasecmp(compressor, "mp4v") == 0) {
      if (MP4_IS_MPEG1_VIDEO_TYPE(type) ||
	  MP4_IS_MPEG2_VIDEO_TYPE(type))
	return CODEC_ID_MPEG2VIDEO;
      if (MP4_IS_MPEG4_VIDEO_TYPE(type))
	return CODEC_ID_MPEG4;
    }
    if (strcasecmp(compressor, "h263") == 0 ||
	strcasecmp(compressor, "s263") == 0) {
      return CODEC_ID_H263;
    }
    if (strcasecmp(compressor, "SVQ3") == 0) {
      return CODEC_ID_SVQ3;
    }
    if (strcasecmp(compressor, "jpeg") == 0) {
      return CODEC_ID_MJPEG;
    }
    return CODEC_ID_NONE;
  }
  if (strcasecmp(stream_type, STREAM_TYPE_MPEG_FILE) == 0) {
    return CODEC_ID_MPEG2VIDEO;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_MPEG2_TRANSPORT_STREAM) == 0) {
    if ((type == MPEG2T_ST_MPEG_VIDEO) ||
	(type == MPEG2T_ST_11172_VIDEO)) 
      return CODEC_ID_MPEG2VIDEO;
    if (type == MPEG2T_ST_H264_VIDEO) {
      return CODEC_ID_H264;
    }
    return CODEC_ID_NONE;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_AVI_FILE) == 0) {
    if (strcasecmp(compressor, "vssh") == 0) {
      return CODEC_ID_H264;
    }
    if (strcasecmp(compressor, "mjpg") == 0) {
      return CODEC_ID_MJPEG;
    }
    return CODEC_ID_NONE;
  }
  if (strcasecmp(stream_type, "QT FILE") == 0) {
    if (strcasecmp(compressor, "h263") == 0 ||
	strcasecmp(compressor, "s263") == 0) {
      return CODEC_ID_H263;
    }
    return CODEC_ID_NONE;
  }
  if ((strcasecmp(stream_type, STREAM_TYPE_RTP) == 0) && fptr != NULL) {
    if (strcmp(fptr->fmt, "32") == 0)
      return CODEC_ID_MPEG2VIDEO;
    if (fptr->rtpmap != NULL) {
      if (strcasecmp(fptr->rtpmap->encode_name, "h263-1998") == 0 ||
	  strcasecmp(fptr->rtpmap->encode_name, "h263-2000") == 0) {
	return CODEC_ID_H263;
      }
    }
    return CODEC_ID_NONE;
  }
  return CODEC_ID_NONE;
}

static bool ffmpeg_find_h264_size (ffmpeg_codec_t *ffmpeg, 
				   const uint8_t *ud, 
				   uint32_t ud_size)
{
  uint32_t offset = 0;
  do {
    if (h264_is_start_code(ud + offset)) {
      if (h264_nal_unit_type(ud + offset) == H264_NAL_TYPE_SEQ_PARAM) {
	h264_decode_t dec;
	memset(&dec, 0, sizeof(dec));
	if (h264_read_seq_info(ud + offset, ud_size - offset, &dec) == 0) {
	  ffmpeg->m_c->width = dec.pic_width;
	  ffmpeg->m_c->height = dec.pic_height;
	  return true;
	}
      }
    }
    offset += h264_find_next_start_code(ud + offset, ud_size - offset);
  } while (offset < ud_size);
  return false;
}

static codec_data_t *ffmpeg_create (const char *stream_type,
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
  ffmpeg_codec_t *ffmpeg;

  ffmpeg = MALLOC_STRUCTURE(ffmpeg_codec_t);
  memset(ffmpeg, 0, sizeof(*ffmpeg));

  ffmpeg->m_vft = vft;
  ffmpeg->m_ifptr = ifptr;
  avcodec_init();
  avcodec_register_all();

  ffmpeg->m_codecId = ffmpeg_find_codec(stream_type, compressor, type, 
					profile, media_fmt, userdata, ud_size);

  // must have a codecID - we checked it earlier
  ffmpeg->m_codec = avcodec_find_decoder(ffmpeg->m_codecId);
  ffmpeg->m_c = avcodec_alloc_context();
  ffmpeg->m_picture = avcodec_alloc_frame();
  bool open_codec = true;
  bool run_userdata = false;

  switch (ffmpeg->m_codecId) {
  case CODEC_ID_H264:
    // need to find height and width
    if (ud_size > 0) {
      open_codec = ffmpeg_find_h264_size(ffmpeg, userdata, ud_size);
      run_userdata = true;
    }
    break;
  case CODEC_ID_MPEG4:
    open_codec = false;
    if (ud_size > 0) {
      uint8_t *vol = MP4AV_Mpeg4FindVol((uint8_t *)userdata, ud_size);
      u_int8_t TimeBits;
      u_int16_t TimeTicks;
      u_int16_t FrameDuration;
      u_int16_t FrameWidth;
      u_int16_t FrameHeight;
      u_int8_t  aspectRatioDefine;
      u_int8_t  aspectRatioWidth;
      u_int8_t  aspectRatioHeight;
      if (vol) {
	if (MP4AV_Mpeg4ParseVol(vol,
				ud_size - (vol - userdata),
				&TimeBits,
				&TimeTicks,
				&FrameDuration,
				&FrameWidth,
				&FrameHeight,
				&aspectRatioDefine,
				&aspectRatioWidth,
				&aspectRatioHeight)) {
	  ffmpeg->m_c->width = FrameWidth;
	  ffmpeg->m_c->height = FrameHeight;
	  open_codec = true;
	  run_userdata = true;
	}
      }
    }
      
    break;
  case CODEC_ID_SVQ3:
    ffmpeg->m_c->extradata = (void *)userdata;
    ffmpeg->m_c->extradata_size = ud_size;
    if (vinfo != NULL) {
      ffmpeg->m_c->width = vinfo->width;
      ffmpeg->m_c->height = vinfo->height;
    }
    break;
  default:
    break;
  }
  if (open_codec) {
    if (avcodec_open(ffmpeg->m_c, ffmpeg->m_codec) < 0) {
      ffmpeg_message(LOG_CRIT, "ffmpeg", "failed to open codec");
      return NULL;
    }
    ffmpeg_message(LOG_DEBUG, "ffmpeg", "pixel format is %d",
		    ffmpeg->m_c->pix_fmt);
    ffmpeg->m_codec_opened = true;
    if (run_userdata) {
      uint32_t offset = 0;
      do {
	int got_picture;
	offset += avcodec_decode_video(ffmpeg->m_c, 
				       ffmpeg->m_picture,
				       &got_picture,
				       (uint8_t *)userdata + offset, 
				       ud_size - offset);
      } while (offset < ud_size);
    }
	
  }

  ffmpeg->m_did_pause = 1;
  return ((codec_data_t *)ffmpeg);
}


static void ffmpeg_close (codec_data_t *ifptr)
{
  ffmpeg_codec_t *ffmpeg;

  ffmpeg = (ffmpeg_codec_t *)ifptr;
  if (ffmpeg->m_c != NULL) {
    avcodec_close(ffmpeg->m_c);
    free(ffmpeg->m_c);
  }
  CHECK_AND_FREE(ffmpeg->m_picture);
  free(ffmpeg);
}


static void ffmpeg_do_pause (codec_data_t *ifptr)
{
  ffmpeg_codec_t *ffmpeg = (ffmpeg_codec_t *)ifptr;
  //mpeg2_reset(ffmpeg->m_decoder, 0);
  ffmpeg->m_did_pause = 1;
  ffmpeg->m_got_i = 0;
}

static int ffmpeg_frame_is_sync (codec_data_t *ifptr, 
				uint8_t *buffer, 
				uint32_t buflen,
				void *userdata)
{
  int ret;
  int ftype;
  ffmpeg_codec_t *ffmpeg = (ffmpeg_codec_t *)ifptr;
  switch (ffmpeg->m_codecId) {
  case CODEC_ID_H264:
    // look for idr nal
    break;
  case CODEC_ID_MPEG2VIDEO:
  // this would be for mpeg2
    ret = MP4AV_Mpeg3FindPictHdr(buffer, buflen, &ftype);
    if (ret >= 0 && ftype == 1) {
      return 1;
    }
    break;
  default:
    break;
  }
  return 0;
}

static int ffmpeg_decode (codec_data_t *ptr,
			    uint64_t ts, 
			    int from_rtp,
			    int *sync_frame,
			    uint8_t *buffer, 
			    uint32_t buflen,
			    void *ud)
{
  ffmpeg_codec_t *ffmpeg = (ffmpeg_codec_t *)ptr;
  uint32_t bytes_used = 0;
  int got_picture = 0;
  if (ffmpeg->m_codec_opened == false) {
    // look for header, like above, and open it
    bool open_codec = true;
    switch (ffmpeg->m_codecId) {
    case CODEC_ID_H264:
      open_codec = ffmpeg_find_h264_size(ffmpeg, buffer, buflen);
      break;
    default:
      break;
    }
    if (open_codec) {
      if (avcodec_open(ffmpeg->m_c, ffmpeg->m_codec) < 0) {
	ffmpeg_message(LOG_CRIT, "ffmpeg", "failed to open codec");
	return buflen;
      }
      ffmpeg->m_codec_opened = true;
    }
  }
  do {
    int local_got_picture;
    bytes_used += avcodec_decode_video(ffmpeg->m_c, 
				      ffmpeg->m_picture,
				      &local_got_picture,
				      buffer + bytes_used, 
				      buflen - bytes_used);
    got_picture |= local_got_picture;
  } while (bytes_used < buflen);

  if (got_picture != 0) {
    if (ffmpeg->m_video_initialized == false) {
      double aspect;
      if (ffmpeg->m_c->sample_aspect_ratio.den == 0) {
	aspect = 0.0; // don't have one
      } else {
	aspect = av_q2d(ffmpeg->m_c->sample_aspect_ratio);
      }
      if (ffmpeg->m_c->width == 0) {
	return buflen;
      }
      ffmpeg->m_vft->video_configure(ffmpeg->m_ifptr, 
				     ffmpeg->m_c->width,
				     ffmpeg->m_c->height,
				     VIDEO_FORMAT_YUV,
				     aspect);
      ffmpeg->m_video_initialized = true;
    }
    ffmpeg->m_vft->video_have_frame(ffmpeg->m_ifptr,
				    ffmpeg->m_picture->data[0],
				    ffmpeg->m_picture->data[1],
				    ffmpeg->m_picture->data[2],
				    ffmpeg->m_picture->linesize[0],
				    ffmpeg->m_picture->linesize[1],
				    ffmpeg->have_cached_ts ?
				    ffmpeg->cached_ts : ts);
    ffmpeg->cached_ts = ts;
  } else {
    ffmpeg->cached_ts = ts;
    ffmpeg->have_cached_ts = true;
  }
#ifdef DEBUG_FFMPEG_FRAME
  ffmpeg_message(LOG_DEBUG, "ffmpeg", "used %u of %u", bytes_used, buflen);
#endif
  return (buflen);
}

static int ffmpeg_codec_check (lib_message_func_t message,
				 const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr,
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			      CConfigSet *pConfig)
{
  enum CodecID fcodec;
  AVCodec *c;
  avcodec_init();
  avcodec_register_all();

  fcodec = ffmpeg_find_codec(stream_type, compressor, type, profile, 
			     fptr, userdata, userdata_size);

  if (fcodec == CODEC_ID_NONE)
    return -1;

  c = avcodec_find_decoder(fcodec);
  if (c == NULL) {
    return -1;
  }
  return pConfig->GetBoolValue(CONFIG_USE_FFMPEG) ? 10 : 1;
}

VIDEO_CODEC_PLUGIN("ffmpeg", 
		   ffmpeg_create,
		   ffmpeg_do_pause,
		   ffmpeg_decode,
		   NULL,
		   ffmpeg_close,
		   ffmpeg_codec_check,
		   ffmpeg_frame_is_sync,
		   MyConfigVariables,
		   sizeof(MyConfigVariables) /
		   sizeof(*MyConfigVariables));
