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
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4live.h"
#ifdef HAVE_FFMPEG
#include "video_ffmpeg.h"
#include "mp4av.h"
//#include "encoder-h263.h"
//#include <dsputil.h>
//#include <mpegvideo.h>

#ifdef HAVE_AVCODECCONTEXT_TIME_BASE
static config_index_t CFG_FFMPEG_USE_STRICT;
static SConfigVariable ffmpegEncoderVariables[] = {
  CONFIG_BOOL(CFG_FFMPEG_USE_STRICT, "FfmpegUseStrictStdCompliance", false),
};

#endif
GUI_BOOL(gui_bframe, CFG_VIDEO_USE_B_FRAMES, "Use B Frames");
GUI_INT_RANGE(gui_bframenum, CFG_VIDEO_NUM_OF_B_FRAMES, "Number of B frames", 1, 4);
DECLARE_TABLE(ffmpeg_mpeg4_gui_options) = {
  TABLE_GUI(gui_bframe),
  TABLE_GUI(gui_bframenum),
};
DECLARE_TABLE_FUNC(ffmpeg_mpeg4_gui_options);

void AddFfmpegConfigVariables (CVideoProfile *pConfig)
{
#ifdef HAVE_AVCODECCONTEXT_TIME_BASE
  pConfig->AddConfigVariables(ffmpegEncoderVariables,
			      NUM_ELEMENTS_IN_ARRAY(ffmpegEncoderVariables));
#endif
}

CFfmpegVideoEncoder::CFfmpegVideoEncoder(CVideoProfile *vp, 
					 uint16_t mtu,
					 CVideoEncoder *next, 
					 bool realTime) :
  CVideoEncoder(vp, mtu, next, realTime)
{
  m_codec = NULL;
  m_avctx = NULL;
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;
	m_YUV = NULL;
	m_push = NULL;
#ifdef OUTPUT_RAW
	m_outfile = NULL;
#endif
}

bool CFfmpegVideoEncoder::Init (void)
{
  avcodec_init();
  avcodec_register_all();

  if (m_push != NULL) {
    delete m_push;
    m_push = NULL;
  }
  double rate;
  rate = TimestampTicks / Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE);

  m_frame_time = (Duration)rate;
  if (strcasecmp(Profile()->GetStringValue(CFG_VIDEO_ENCODING),
		 VIDEO_ENCODING_MPEG4) == 0) {
    m_push = new CTimestampPush(1);
    m_codec = avcodec_find_encoder(CODEC_ID_MPEG4);
    m_media_frame = MPEG4VIDEOFRAME;
#ifdef OUTPUT_RAW
    m_outfile = fopen("raw.m4v", FOPEN_WRITE_BINARY);
#endif
  } else if (strcasecmp(Profile()->GetStringValue(CFG_VIDEO_ENCODING),
			VIDEO_ENCODING_H263) == 0) {
    m_push = new CTimestampPush(1);
    m_codec = avcodec_find_encoder(CODEC_ID_H263);
    m_media_frame = H263VIDEOFRAME;
#ifdef OUTPUT_RAW
    m_outfile = fopen("raw.263", FOPEN_WRITE_BINARY);
#endif
  } else {
    m_push = new CTimestampPush(3);
    m_codec = avcodec_find_encoder(CODEC_ID_MPEG2VIDEO);
    m_media_frame = MPEG2VIDEOFRAME;
#ifdef OUTPUT_RAW
    m_outfile = fopen("raw.m2v", FOPEN_WRITE_BINARY);
#endif
  }

  if (m_codec == NULL) {
    error_message("Couldn't find codec");
    return false;
  }
  
  m_avctx = avcodec_alloc_context();
  m_picture = avcodec_alloc_frame();
  m_avctx->width = Profile()->m_videoWidth;
  m_avctx->height = Profile()->m_videoHeight;
  m_avctx->bit_rate = 
    Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000;
#ifndef HAVE_AVCODECCONTEXT_TIME_BASE
  m_avctx->frame_rate = (int)(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) + 0.5);
  m_avctx->frame_rate_base = 1;
#else
  m_avctx->time_base = (AVRational){1, (int)(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) + .5)};
  m_avctx->strict_std_compliance = -1;
  m_avctx->pix_fmt = PIX_FMT_YUV420P;
  m_avctx->profile = Profile()->m_videoMpeg4ProfileId;
#endif
  if (Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_WIDTH) > 0 &&
      Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_HEIGHT) > 0) {
#ifndef HAVE_AVRATIONAL
    float asp = (float)Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_WIDTH);
    asp /= (float)Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_HEIGHT);
    m_avctx->aspect_ratio = asp;
#else
    AVRational asp = 
      {Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_WIDTH),
       Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_HEIGHT)};
    m_avctx->sample_aspect_ratio = asp;
#endif
  }

#if 0
  debug_message("ffmpeg %u x %u bit rate %u media %d",
		Profile()->m_videoWidth, 
		Profile()->m_videoHeight, 
		m_avctx->bit_rate,
		m_media_frame);
#endif
  m_usingBFrames = false;
  m_BFrameCount = 0;
  if (m_media_frame == MPEG2VIDEOFRAME) {
    m_avctx->gop_size = 15;
    m_avctx->b_frame_strategy = 0;
    m_avctx->max_b_frames = 2;
    m_usingBFrames = true;
    m_BFrameCount = 2;
  } else {
    if (m_media_frame == H263VIDEOFRAME) {
      m_avctx->bit_rate = 
	Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 800;
      m_avctx->bit_rate_tolerance = 
	Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 200;
#if 0
      // this makes file writing difficult
      m_avctx->rtp_mode = true;
      m_avctx->rtp_payload_size = 
	Profile()->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
#endif
    } else if (m_media_frame == MPEG4VIDEOFRAME) {
      if (Profile()->GetBoolValue(CFG_VIDEO_USE_B_FRAMES)) {
	m_avctx->max_b_frames = Profile()->GetIntegerValue(CFG_VIDEO_NUM_OF_B_FRAMES);
	m_usingBFrames = true;
	m_BFrameCount = m_avctx->max_b_frames;
      }
    }
    m_key_frame_count = m_avctx->gop_size = (int)
      ((Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE)+0.5)
       * Profile()->GetFloatValue(CFG_VIDEO_KEY_FRAME_INTERVAL));
    m_avctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    debug_message("key frame count is %d", m_key_frame_count);
  }
  m_count = 0;
  if (avcodec_open(m_avctx, m_codec) < 0) {
    error_message("Couldn't open codec");
    return false;
  }
  m_first_frame = true;
  return true;
}

bool CFfmpegVideoEncoder::EncodeImage(
				      const u_int8_t* pY, 
				      const u_int8_t* pU, 
				      const u_int8_t* pV, 
	u_int32_t yStride, u_int32_t uvStride,
	bool wantKeyFrame, 
	Duration elapsedDuration,
	Timestamp srcFrameTimestamp)
{
  m_push->Push(srcFrameTimestamp);
	if (m_vopBuffer == NULL) {
		m_vopBuffer = (u_int8_t*)malloc(Profile()->m_videoMaxVopSize);
		if (m_vopBuffer == NULL) {
			return false;
		}
	}
	if (m_media_frame == H263VIDEOFRAME) {
	  m_count++;
	  if (m_count >= m_key_frame_count) {
	    wantKeyFrame = true;
	    m_count = 0;
	  }
	}
	if (wantKeyFrame) m_picture->pict_type = FF_I_TYPE; //m_picture->key_frame = 1;
	else //m_picture->key_frame = 0;
	  m_picture->pict_type = 0;

	m_picture->data[0] = (uint8_t *)pY;
	m_picture->data[1] = (uint8_t *)pU;
	m_picture->data[2] = (uint8_t *)pV;
	m_picture->linesize[0] = yStride;
	m_picture->linesize[1] = uvStride;
	m_picture->linesize[2] = uvStride;
	m_picture->pts = srcFrameTimestamp;
#if 0
	if (m_picture->key_frame == 1) {
	  debug_message("key frame "U64, srcFrameTimestamp);
	}
#endif

	
	m_vopBufferLength = avcodec_encode_video(m_avctx, 
						 m_vopBuffer, 
						 Profile()->m_videoMaxVopSize, 
						 m_picture);
	//debug_message(U64" ffmpeg len %d", srcFrameTimestamp, m_vopBufferLength);
#ifdef OUTPUT_RAW
	if (m_vopBufferLength) {
	  fwrite(m_vopBuffer, m_vopBufferLength, 1, m_outfile);
	}
#endif
	//	m_avctx.frame_number++;

	return true;
}


bool CFfmpegVideoEncoder::GetEncodedImage(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength,
	Timestamp *dts, Timestamp *pts)
{
  bool ret = true;
  *ppBuffer = m_vopBuffer;
  *pBufferLength = m_vopBufferLength;

  if (m_vopBufferLength != 0) {
    *pts = *dts = m_push->Pop();
#if 1
    if (m_media_frame == MPEG2VIDEOFRAME ||
	(m_usingBFrames && m_media_frame == MPEG4VIDEOFRAME)) {
      *pts = m_avctx->coded_frame->pts;
    }
#else
      
    if (m_media_frame == MPEG2VIDEOFRAME) {
      // special processing for mpeg2 - the pts is not when we
      // dts
      int ret, ftype;
      ret = MP4AV_Mpeg3FindPictHdr(m_vopBuffer, m_vopBufferLength, &ftype);
      if (ret >= 0 && ftype != 3) {
	*pts = *dts + (2 * m_frame_time);
      } else {
	*pts = *dts - m_frame_time;
      }
      //error_message("dts %llu pts %llu", *dts, *pts);
    } else if (m_usingBFrames && m_media_frame == MPEG4VIDEOFRAME) {
      uint8_t *vop = MP4AV_Mpeg4FindVop(m_vopBuffer, m_vopBufferLength);
      if (MP4AV_Mpeg4GetVopType(vop, m_vopBufferLength - (vop - m_vopBuffer))
	  != VOP_TYPE_B) {
	if (m_first_frame) {
	  // first time - IPBB
	  *pts = *dts + m_frame_time;
	  m_first_frame = false;
	} else
	  *pts = *dts + (m_BFrameCount * m_frame_time);
      } 
    } 
    debug_message("dts"U64" pts "U64" from ffmpeg "U64,
		  *dts,
		  *pts,
		  m_avctx->coded_frame->pts);
#endif
  } else {
    // return without clearing m_vopBuffer
    *dts = *pts = 0;
    *ppBuffer = NULL;
    *pBufferLength = 0;
    return false;
  }
  m_vopBuffer = NULL;
  m_vopBufferLength = 0;
  
  return ret;
}
media_free_f CFfmpegVideoEncoder::GetMediaFreeFunction(void)
{
  return NULL;
}

bool CFfmpegVideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{
  uint32_t w = Profile()->m_videoWidth;
  uint32_t uvw = w / 2;

  CopyYuv(m_avctx->coded_frame->data[0],
	  m_avctx->coded_frame->data[1],
	  m_avctx->coded_frame->data[2],
	  m_avctx->coded_frame->linesize[0],
	  m_avctx->coded_frame->linesize[1],
	  m_avctx->coded_frame->linesize[2],
	  pY, pU, pV, 
	  w, uvw, uvw, 
	  w, Profile()->m_videoHeight);
  return true;
}

void CFfmpegVideoEncoder::StopEncoder (void)
{
  avcodec_close(m_avctx);
  CHECK_AND_FREE(m_vopBuffer);
  CHECK_AND_FREE(m_YUV);
  CHECK_AND_FREE(m_picture);
  CHECK_AND_FREE(m_avctx);
#ifdef OUTPUT_RAW
  if (m_outfile) {
    fclose(m_outfile);
  }
#endif
  if (m_push != NULL) {
    delete m_push;
    m_push = NULL;
  }
	  
}

bool CFfmpegVideoEncoder::GetEsConfig (uint8_t **ppEsConfig, 
				       uint32_t *pEsConfigLen)
{
  if (m_avctx->extradata_size == 0) return false;
  *ppEsConfig = (uint8_t *)malloc(m_avctx->extradata_size);
  *pEsConfigLen = m_avctx->extradata_size;
  memcpy(*ppEsConfig, m_avctx->extradata, *pEsConfigLen);
  if (m_media_frame == MPEG4VIDEOFRAME) {
    RemoveUserdataFromVol(ppEsConfig, pEsConfigLen);
  }
  return true;
}
#endif
