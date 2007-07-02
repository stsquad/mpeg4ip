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
#define attribute_deprecated
#include "ffmpeg.h"
#include "mp4av.h"
#include "mp4av_h264.h"
#include <mpeg2t/mpeg2t_defines.h>
#include "mp4util/mpeg4_sdp.h"
#include "mp4util/h264_sdp.h"
#include <mpeg2ps/mpeg2_ps.h>
#include "ffmpeg_if.h"

static SConfigVariable MyConfigVariables[] = {
  CONFIG_BOOL(CONFIG_USE_FFMPEG, "UseFFmpeg", false),
};

#define ffmpeg_message (ffmpeg->m_vft->log_msg)

//#define DEBUG_FFMPEG_FRAME 1
//#define DEBUG_FFMPEG_PTS 1
static enum CodecID ffmpeg_find_codec (const char *stream_type,
				       const char *compressor, 
				       int type, 
				       int profile, 
				       format_list_t *fptr,
				       const uint8_t *userdata,
				       uint32_t ud_size)
{
  bool have_mp4_file = strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0;

  if (have_mp4_file) {
    if (strcasecmp(compressor, "avc1") == 0) {
      return CODEC_ID_H264;
    }
    if (strcasecmp(compressor, "264b") == 0) {
      return CODEC_ID_H264;
    }
    if (strcasecmp(compressor, "mp4v") == 0) {
      if (MP4_IS_MPEG1_VIDEO_TYPE(type) ||
	  MP4_IS_MPEG2_VIDEO_TYPE(type))
	return CODEC_ID_MPEG2VIDEO;
      if (MP4_IS_MPEG4_VIDEO_TYPE(type))
	return CODEC_ID_MPEG4;
    }
  }
  if (have_mp4_file ||
      strcasecmp(stream_type, "QT FILE") == 0) {
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
    if (type == MPEG_VIDEO_H264) return CODEC_ID_H264;
    return CODEC_ID_MPEG2VIDEO;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_MPEG2_TRANSPORT_STREAM) == 0) {
    if ((type == MPEG2T_ST_MPEG_VIDEO) ||
	(type == MPEG2T_ST_11172_VIDEO)) 
      return CODEC_ID_MPEG2VIDEO;
    if (type == MPEG2T_ST_H264_VIDEO) {
      return CODEC_ID_H264;
    }
    if (type == MPEG2T_ST_MPEG4_VIDEO) {
      return CODEC_ID_MPEG4;
    }
    return CODEC_ID_NONE;
  }

  if (strcasecmp(stream_type, STREAM_TYPE_AVI_FILE) == 0) {
    if (strcasecmp(compressor, "vssh") == 0) {
      return CODEC_ID_H264;
    }
    if (strcasecmp(compressor, "H263") == 0) {
      return CODEC_ID_H263;
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
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "h263-1998") == 0 ||
	  strcasecmp(fptr->rtpmap_name, "h263-2000") == 0) {
	return CODEC_ID_H263;
      }
      if (strcasecmp(fptr->rtpmap_name, "MP4V-ES") == 0) {
	// may want to check level and profile
	return CODEC_ID_MPEG4;
      }
      if (strcasecmp(fptr->rtpmap_name, "h264") == 0) {
	// may want to check for sprop-parameters
	return CODEC_ID_H264;
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
  ffmpeg_message(LOG_DEBUG, "ffmpeg", "start finding size");
  do {
    if (h264_is_start_code(ud + offset)) {
      uint8_t nal_type = h264_nal_unit_type(ud + offset);
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "nal %u", nal_type);
      if (nal_type == H264_NAL_TYPE_SEQ_PARAM) {
	h264_decode_t dec;
	memset(&dec, 0, sizeof(dec));
	if (h264_read_seq_info(ud + offset, ud_size - offset, &dec) == 0) {
	  ffmpeg->m_c->width = dec.pic_width;
	  ffmpeg->m_c->height = dec.pic_height;
	  return true;
	}
      }
    }
    uint32_t result;
    result = h264_find_next_start_code(ud + offset, ud_size - offset);
    if (result == 0) offset = ud_size;
    else offset += result;

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
  av_log_set_level(AV_LOG_QUIET);

  ffmpeg->m_codecId = ffmpeg_find_codec(stream_type, compressor, type, 
					profile, media_fmt, userdata, ud_size);

  // must have a codecID - we checked it earlier
  ffmpeg->m_codec = avcodec_find_decoder(ffmpeg->m_codecId);
  ffmpeg->m_c = avcodec_alloc_context();
  ffmpeg->m_picture = avcodec_alloc_frame();
  bool open_codec = true;
  bool run_userdata = false;
  bool free_userdata = false;

  switch (ffmpeg->m_codecId) {
  case CODEC_ID_MJPEG:
    break;
  case CODEC_ID_H264:
    // need to find height and width
    if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
      userdata = h264_sdp_parse_sprop_param_sets(media_fmt->fmt_param,
						 &ud_size, 
						 ffmpeg->m_vft->log_msg);
      if (userdata != NULL) free_userdata = true;
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "sprop len %d", ud_size);
    }
    if (ud_size > 0) {
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "userdata len %d", ud_size);
      open_codec = ffmpeg_find_h264_size(ffmpeg, userdata, ud_size);
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "open codec is %d", open_codec);
      run_userdata = true;
    } else {
      open_codec = false;
    }
    break;
  case CODEC_ID_MPEG4: {
    fmtp_parse_t *fmtp = NULL;
    open_codec = false;
    if (media_fmt != NULL) {
      fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param, 
				  ffmpeg->m_vft->log_msg);
      if (fmtp->config_binary != NULL) {
	userdata = fmtp->config_binary;
	ud_size = fmtp->config_binary_len;
	fmtp->config_binary = NULL;
	free_userdata = true;
      }
    }
      
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
    if (fmtp != NULL) {
      free_fmtp_parse(fmtp);
    }
  }
    break;
  case CODEC_ID_SVQ3:
    ffmpeg->m_c->extradata = (typeof(ffmpeg->m_c->extradata))userdata;
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
    ffmpeg_interface_lock();
    if (avcodec_open(ffmpeg->m_c, ffmpeg->m_codec) < 0) {
      ffmpeg_interface_unlock();
      ffmpeg_message(LOG_CRIT, "ffmpeg", "failed to open codec");
      return NULL;
    }
    ffmpeg_interface_unlock();
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

    if (free_userdata) {
      CHECK_AND_FREE(userdata);
    }
  ffmpeg->m_did_pause = 1;
  return ((codec_data_t *)ffmpeg);
}


static void ffmpeg_close (codec_data_t *ifptr)
{
  ffmpeg_codec_t *ffmpeg;

  ffmpeg = (ffmpeg_codec_t *)ifptr;
  if (ffmpeg->m_c != NULL) {
    ffmpeg_interface_lock();
    avcodec_close(ffmpeg->m_c);
    ffmpeg_interface_unlock();
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
  ffmpeg->m_got_i = false;
  ffmpeg->have_cached_ts = false;
  MP4AV_clear_dts_from_pts(&ffmpeg->pts_to_dts);
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
    // disable to just start right up
#if 0
    uint32_t offset;
    do {
      uint8_t nal_type = h264_nal_unit_type(buffer);
      ffmpeg_message(LOG_DEBUG, "ffmpeg", "nal type %u", nal_type);
      if (nal_type == H264_NAL_TYPE_SEQ_PARAM) return 1;
      if (h264_nal_unit_type_is_slice(nal_type)) {
	if (nal_type == H264_NAL_TYPE_IDR_SLICE) return 1;
	return 0;
      }
      offset = h264_find_next_start_code(buffer, buflen);
      buffer += offset;
      buflen -= offset;
    } while (offset != 0);
#else
    return 1;
#endif
    break;
  case CODEC_ID_MPEG2VIDEO:
    // this would be for mpeg2
    ret = MP4AV_Mpeg3FindPictHdr(buffer, buflen, &ftype);
    ffmpeg_message(LOG_ERR, "ffmpeg", "ret %u type %u", ret, ftype);
    if (ret >= 0 && ftype == 1) {
      return 1;
    }
    break;
  case CODEC_ID_MPEG4: {
    uint8_t *vop = MP4AV_Mpeg4FindVop(buffer, buflen);
    if (vop == NULL) return 0;
    if (MP4AV_Mpeg4GetVopType(vop, buflen - (vop - buffer)) == VOP_TYPE_I)
	return 1;
  }    
    break;
  default:
    // for every other, return that it is sync
    return 1;
  }
  return 0;
}

static int ffmpeg_decode (codec_data_t *ptr,
			  frame_timestamp_t *pts, 
			    int from_rtp,
			    int *sync_frame,
			    uint8_t *buffer, 
			    uint32_t buflen,
			    void *ud)
{
  ffmpeg_codec_t *ffmpeg = (ffmpeg_codec_t *)ptr;
  uint32_t bytes_used = 0;
  int got_picture = 0;
  uint64_t ts = pts->msec_timestamp;

  //ffmpeg_message(LOG_ERR, "ffmpeg", "%u timestamp "U64, buflen, ts);
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
      ffmpeg_interface_lock();
      if (avcodec_open(ffmpeg->m_c, ffmpeg->m_codec) < 0) {
	ffmpeg_interface_unlock();
	ffmpeg_message(LOG_CRIT, "ffmpeg", "failed to open codec");
	return buflen;
      }
      ffmpeg_interface_unlock();
      ffmpeg->m_codec_opened = true;
      ffmpeg_message(LOG_ERR, "ffmpeg", "opened codec");
    } else {
      ffmpeg_message(LOG_ERR, "ffmpeg", "no open %u "U64, buflen, ts);
      return buflen;
    }
  }

  // look and see if we have read the I frame. 
  if (ffmpeg->m_got_i == false) {
    if (ffmpeg_frame_is_sync(ptr, buffer, buflen, NULL) == 0) {
      return buflen;
    }
    ffmpeg->m_got_i = true;
  }

  int ret;
  do {
    int local_got_picture;
    ret = avcodec_decode_video(ffmpeg->m_c, 
			       ffmpeg->m_picture,
			       &local_got_picture,
			       buffer + bytes_used, 
			       buflen - bytes_used);
    bytes_used += ret;
    //ffmpeg_message(LOG_CRIT, "ffmpeg", "used %d %d", ret, local_got_picture);
    got_picture |= local_got_picture;
  } while (ret != -1 && bytes_used < buflen);

  if (pts->timestamp_is_pts) {
    //ffmpeg_message(LOG_ERR, "ffmpeg", "pts timestamp "U64, ts);
    if (ffmpeg->m_codecId == CODEC_ID_MPEG2VIDEO) {
      if (ffmpeg->pts_convert.frame_rate == 0.0) {
	int have_mpeg2;
	uint32_t h, w;
	double bitrate, aspect_ratio;
	uint8_t profile;
	MP4AV_Mpeg3ParseSeqHdr(buffer, buflen,
			       &have_mpeg2,
			       &h, &w, 
			       &ffmpeg->pts_convert.frame_rate,
			       &bitrate, &aspect_ratio,
			       &profile);
      }
			       
      int ftype;
      int header = MP4AV_Mpeg3FindPictHdr(buffer, buflen, &ftype);
      if (header >= 0) {
	uint16_t temp_ref = MP4AV_Mpeg3PictHdrTempRef(buffer + header);
	uint64_t ret;
	if (got_picture == 0 ||
	    mpeg3_find_dts_from_pts(&ffmpeg->pts_convert,
				    ts, 
				    ftype,
				    temp_ref, 
				    &ret) < 0) {
 	  ffmpeg->have_cached_ts = false;
	  return buflen;
	} 
#if 0
	ffmpeg->m_vft->log_msg(LOG_DEBUG, "ffmpeg", "pts "U64" dts "U64" temp %u type %u %u", 
			       ts, ret,
			       temp_ref, ftype, got_picture);
#endif
	ts = ret;
	//	ffmpeg_message(LOG_ERR, "ffmpeg", "type %d ref %u "U64, ftype, temp_ref, ret);
      }
    } else if (ffmpeg->m_codecId == CODEC_ID_MPEG4) {
      uint8_t *vopstart = MP4AV_Mpeg4FindVop(buffer, buflen);
      if (vopstart) {
	int ftype = MP4AV_Mpeg4GetVopType(vopstart, buflen);
	uint64_t dts;
	if (MP4AV_calculate_dts_from_pts(&ffmpeg->pts_to_dts,
					 ts,
					 ftype, 
					 &dts) < 0) {
	  ffmpeg->have_cached_ts = false;
#ifdef DEBUG_FFMPEG_PTS
	  ffmpeg_message(LOG_DEBUG, "ffmpeg", "type %d %d pts "U64" failed to calc",
			 ftype, got_picture, ts);
#endif
	  return buflen;
	}
#ifdef DEBUG_FFMPEG_PTS
	ffmpeg_message(LOG_DEBUG, "ffmpeg", "type %d %d pts "U64" dts "U64,
		       ftype, got_picture, ts, dts);
#endif
	ts = dts;
      }
    } else if (ffmpeg->m_codecId == CODEC_ID_H264) {
      uint8_t *nal_ptr = buffer;
      uint32_t len = buflen;
      bool have_b_nal = false;
      do {
	if (h264_nal_unit_type_is_slice(h264_nal_unit_type(nal_ptr))) {
	  uint8_t slice_type;
	  if (h264_find_slice_type(nal_ptr, len, &slice_type, false) >= 0) {
	    have_b_nal = H264_TYPE_IS_B(slice_type);
	  }
	}
	uint32_t offset = h264_find_next_start_code(nal_ptr, len);
	if (offset == 0) {
	  len = 0;
	} else {
	  nal_ptr += offset;
	  len -= offset;
	}
      } while (len > 0 && have_b_nal == false);
      uint64_t dts;
      if (MP4AV_calculate_dts_from_pts(&ffmpeg->pts_to_dts,
				       ts, 
				       have_b_nal ? VOP_TYPE_B : VOP_TYPE_P,
				       &dts) < 0) {
	ffmpeg->have_cached_ts = false;
#ifdef DEBUG_FFMPEG_PTS
	ffmpeg_message(LOG_DEBUG, "ffmpeg", "pts "U64" failed to calc",
			 ts);
#endif
	  return buflen;
      }
      ts = dts;
    }
  }
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

    if (ffmpeg->m_c->pix_fmt != PIX_FMT_YUV420P) {
      // convert the image from whatever it is to YUV 4:2:0
      AVPicture from, to;
      int ret;
      // get the buffer to copy into (put it right into the ring buffer)
      ret = ffmpeg->m_vft->video_get_buffer(ffmpeg->m_ifptr,
					    &to.data[0],
					    &to.data[1],
					    &to.data[2]);
      if (ret == 0) { 
	return buflen;
      }
      // set up the AVPicture structures
      to.linesize[0] = ffmpeg->m_c->width;
      to.linesize[1] = ffmpeg->m_c->width / 2;
      to.linesize[2] = ffmpeg->m_c->width / 2;
      for (int ix = 0; ix < 4; ix++) {
	from.data[ix] = ffmpeg->m_picture->data[ix];
	from.linesize[ix] = ffmpeg->m_picture->linesize[ix];
      }
      
      img_convert(&to, PIX_FMT_YUV420P,
		  &from, ffmpeg->m_c->pix_fmt,
		  ffmpeg->m_c->width, ffmpeg->m_c->height);
      ffmpeg->m_vft->video_filled_buffer(ffmpeg->m_ifptr,
					 ffmpeg->have_cached_ts ?
					 ffmpeg->cached_ts : ts);
    } else {
      ffmpeg->m_vft->video_have_frame(ffmpeg->m_ifptr,
				      ffmpeg->m_picture->data[0],
				      ffmpeg->m_picture->data[1],
				      ffmpeg->m_picture->data[2],
				      ffmpeg->m_picture->linesize[0],
				      ffmpeg->m_picture->linesize[1],
				      ffmpeg->have_cached_ts ?
				      ffmpeg->cached_ts : ts);
    }
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
  av_log_set_level(AV_LOG_QUIET);
  fcodec = ffmpeg_find_codec(stream_type, compressor, type, profile, 
			     fptr, userdata, userdata_size);

  if (fcodec == CODEC_ID_NONE)
    return -1;

  c = avcodec_find_decoder(fcodec);
  message(LOG_DEBUG, "ffmpeg", "codec value %p", c);
  if (c == NULL) {
    return -1;
  }
  return pConfig->GetBoolValue(CONFIG_USE_FFMPEG) ? 20 : 2;
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
