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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */

#include "mp4live.h"
#ifdef HAVE_X264
#include "video_x264.h"
#include "mp4av.h"
#include "mp4av_h264.h"
#include "encoder_gui_options.h"

//#define DEBUG_H264 1
#define USE_OUR_YUV 1

static config_index_t CFG_X264_USE_CABAC;
static config_index_t CFG_X264_USE_CBR;

static SConfigVariable X264EncoderVariables[] = {
  CONFIG_BOOL(CFG_X264_USE_CABAC, "x264UseCabac", true),
  CONFIG_BOOL(CFG_X264_USE_CBR, "x264UseCbr", true),
};

GUI_BOOL(gui_cabac, CFG_X264_USE_CABAC, "Use Cabac");
GUI_BOOL(gui_cbr, CFG_X264_USE_CBR, "Use CBR");

DECLARE_TABLE(x264_gui_options) = {
  TABLE_GUI(gui_cabac),
  TABLE_GUI(gui_cbr),
};
DECLARE_TABLE_COUNT(x264_gui_options);
DECLARE_TABLE_FUNC(x264_gui_options);

void AddX264ConfigVariables (CVideoProfile *pConfig)
{
  pConfig->AddConfigVariables(X264EncoderVariables,
			      NUM_ELEMENTS_IN_ARRAY(X264EncoderVariables));
}

static void x264_log (void *p_unused, int ilevel, const char *fmt, va_list arg)
{
  int level;
  switch (ilevel) {
  case X264_LOG_ERROR: level = LOG_ERR; break;
  case X264_LOG_WARNING: level = LOG_WARNING; break;
  case X264_LOG_INFO: level = LOG_INFO; break;
  case X264_LOG_DEBUG: level = LOG_DEBUG; break;
  default: level = LOG_DEBUG; break;
  }
  library_message(level, "x264", fmt, arg);
}

CX264VideoEncoder::CX264VideoEncoder(CVideoProfile *vp, 
				     uint16_t mtu,
				     CVideoEncoder *next, 
				     bool realTime) :
  CVideoEncoder(vp, mtu, next, realTime)
{
  m_vopBuffer = NULL;
  m_vopBufferLength = 0;
  m_YUV = NULL;
  m_push = NULL;
  m_count = 0;
  m_nal_info = NULL;
  m_media_frame = H264VIDEOFRAME;
#ifdef OUTPUT_RAW
  m_outfile = NULL;
#endif
}

bool CX264VideoEncoder::Init (void)
{
  if (m_push != NULL) {
    delete m_push;
    m_push = NULL;
  }
  double rate;
  rate = TimestampTicks / Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE);

  m_frame_time = (Duration)rate;
  m_push = new CTimestampPush(3);

  m_videoH264Seq = NULL;
  m_videoH264SeqSize = 0;
  m_videoH264Pic = NULL;
  m_videoH264PicSize = 0;

#ifdef OUTPUT_RAW
  m_outfile = fopen("raw.h264", FOPEN_WRITE_BINARY);
#endif


  x264_param_default(&m_param);
  m_param.i_width = Profile()->m_videoWidth;
  m_param.i_height = Profile()->m_videoHeight;
  m_param.i_fps_num = 
    (int)((Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) + .5) * 1000);
  m_param.i_fps_den = 1000;
  //m_param.i_maxframes = 0;
  m_key_frame_count = m_param.i_keyint_max = 
    (int)(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) * 
	  Profile()->GetFloatValue(CFG_VIDEO_KEY_FRAME_INTERVAL));
  m_param.i_bframe = 0;
  m_param.rc.i_bitrate = Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE);
  m_param.rc.b_cbr = Profile()->GetBoolValue(CFG_X264_USE_CBR) ? 1 : 0;
  //m_param.rc.b_stat_write = 0;
  //m_param.analyse.inter = 0;
  m_param.analyse.b_psnr = 0;
  m_param.b_cabac = Profile()->GetBoolValue(CFG_X264_USE_CABAC) ? 1 : 0;
  m_param.pf_log = x264_log;
  
  m_h = x264_encoder_open(&m_param);
  if (m_h == NULL) {
    error_message("Couldn't init x264 encoder");
    return false;
  }

#ifdef USE_OUR_YUV
  m_pic_input.i_type = X264_TYPE_AUTO;
  m_pic_input.i_qpplus1 = 0;
  m_pic_input.img.i_csp = X264_CSP_I420;
#else
  x264_picture_alloc(&m_pic_input, X264_CSP_I420, Profile()->m_videoWidth,
		     Profile()->m_videoHeight);
#endif
  
  m_count = 0;
  
  return true;
}

bool CX264VideoEncoder::EncodeImage(
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
      error_message("Cannot malloc vop size");
      return false;
    }
  }
  m_count++;
  if (m_count >= m_key_frame_count) {
    wantKeyFrame = true;
    m_count = 0;
  }
#ifdef USE_OUR_YUV
  m_pic_input.img.plane[0] = (uint8_t *)pY;
  m_pic_input.img.i_stride[0] = yStride;
  m_pic_input.img.plane[1] = (uint8_t *)pU;
  m_pic_input.img.i_stride[1] = uvStride;
  m_pic_input.img.plane[2] = (uint8_t *)pV;
  m_pic_input.img.i_stride[2] = uvStride;
#else
  CopyYuv(pY, pU, pV, yStride, uvStride, uvStride,
	  m_pic_input.img.plane[0],m_pic_input.img.plane[1],m_pic_input.img.plane[2], 
	  m_pic_input.img.i_stride[0],m_pic_input.img.i_stride[1],m_pic_input.img.i_stride[2],
	  Profile()->m_videoWidth, Profile()->m_videoHeight);
#endif

  m_pic_input.i_type = wantKeyFrame ? X264_TYPE_IDR : X264_TYPE_AUTO;
  m_pic_input.i_pts = srcFrameTimestamp;

  x264_nal_t *nal;
  int i_nal;
  if (x264_encoder_encode(m_h, &nal, &i_nal, &m_pic_input, &m_pic_output) < 0) {
    error_message("x264_encoder_encode failed");
    return false;
  }
  CHECK_AND_FREE(m_nal_info);
  m_nal_info = (h264_nal_buf_t *)malloc(i_nal * sizeof(h264_nal_buf_t));
  uint8_t *vopBuffer = m_vopBuffer;
  int vopBufferLen = m_vopBufferLength;
  uint32_t loaded = 0;
  uint32_t nal_on = 0;

  // read the nals out of the encoder.  Nice...
  for (int ix = 0; ix < i_nal; ix++) {
    int i_size;
    bool skip = false;
    i_size = x264_nal_encode(vopBuffer, &vopBufferLen, 1, &nal[ix]);
    if (i_size > 0) {
      m_nal_info[nal_on].nal_length = i_size;
      m_nal_info[nal_on].nal_offset = loaded;
      m_nal_info[nal_on].nal_type = nal[ix].i_type;
      uint32_t header = 0;
      if (h264_is_start_code(vopBuffer)) {
	header = vopBuffer[2] == 1 ? 3 : 4;
      }
      m_nal_info[nal_on].nal_length -= header;
      m_nal_info[nal_on].nal_offset += header;
      // don't put seq or pic nal unless we have a change
      if (m_nal_info[nal_on].nal_type == H264_NAL_TYPE_SEQ_PARAM) {
	if (m_nal_info[nal_on].nal_length != m_videoH264SeqSize ||
	    memcmp(vopBuffer, m_videoH264Seq, m_videoH264SeqSize) != 0) {
	  CHECK_AND_FREE(m_videoH264Seq);
	  m_videoH264SeqSize = m_nal_info[nal_on].nal_length;
	  m_videoH264Seq = (uint8_t *)malloc(m_videoH264SeqSize);
	  memcpy(m_videoH264Seq, vopBuffer, m_videoH264SeqSize);
	} else {
	  skip = true;
	}
      } else if (m_nal_info[nal_on].nal_type == H264_NAL_TYPE_PIC_PARAM) {
	if (m_nal_info[nal_on].nal_length != m_videoH264PicSize ||
	      memcmp(vopBuffer, m_videoH264Pic, m_videoH264PicSize) != 0) {
	  CHECK_AND_FREE(m_videoH264Pic);
	  m_videoH264PicSize = m_nal_info[nal_on].nal_length;
	  m_videoH264Pic = (uint8_t *)malloc(m_videoH264PicSize);
	  memcpy(m_videoH264Pic, vopBuffer, m_videoH264PicSize);
	} else {
	  skip = true;
	}
      }
      if (skip == false) {
#ifdef DEBUG_H264
	uint8_t nal_type = nal[ix].i_type;
	if (h264_nal_unit_type_is_slice(nal_type)) {
	  uint8_t stype;
	  h264_find_slice_type(vopBuffer, i_size, &stype);
	  debug_message("nal %d - type %u slice type %u %d",
			ix, nal_type, stype, i_size);
	} else {
	  debug_message("nal %d - type %u %d", ix, nal_type, i_size);
	}
#endif
	nal_on++;
	loaded += i_size;
	vopBufferLen -= i_size;
	vopBuffer += i_size;
      } else {
#ifdef DEBUG_H264
	debug_message("skipped nal %u", nal[ix].i_type);
#endif
      }
    } else {
      error_message("Need to increase vop buffer size by %d", 
		    -i_size);
    }
  }
  m_nal_num = nal_on;
  m_vopBufferLength = loaded;
#ifdef DEBUG_H264
  debug_message("x264 loaded %d nals, %u len", 
		i_nal, loaded);
#endif
#ifdef OUTPUT_RAW
  if (m_vopBufferLength) {
    fwrite(m_vopBuffer, m_vopBufferLength, 1, m_outfile);
  }
#endif
	
  return true;
}

static void free_x264_frame (void *ifptr)
{
  h264_media_frame_t *mf = (h264_media_frame_t *)ifptr;

  if (mf != NULL) {
    CHECK_AND_FREE(mf->buffer);
    CHECK_AND_FREE(mf->nal_bufs);
    free(mf);
  }
}

media_free_f CX264VideoEncoder::GetMediaFreeFunction (void)
{
  return free_x264_frame;
}

bool CX264VideoEncoder::GetEncodedImage(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength,
	Timestamp *dts, Timestamp *pts)
{
  if (m_vopBufferLength == 0) return false;

  h264_media_frame_t *mf = MALLOC_STRUCTURE(h264_media_frame_t);

  if (mf == NULL) {
    CHECK_AND_FREE(m_vopBuffer);
    m_vopBufferLength = 0;
    return false;
  }
  
  mf->buffer = m_vopBuffer;
  mf->buffer_len = m_vopBufferLength;
  mf->nal_number = m_nal_num;
  mf->nal_bufs = m_nal_info;
  m_nal_info = NULL;
  m_vopBuffer = NULL;
  *ppBuffer = (uint8_t *)mf;
  *pBufferLength = 0;

  *dts = m_push->Pop();
  *pts = m_pic_input.i_pts;
  if (*dts != *pts) {
    debug_message("PTS "U64" not DTS "U64, 
		  *pts, *dts);
  }
  m_vopBuffer = NULL;
  m_vopBufferLength = 0;
  
  return true;
}

bool CX264VideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{
  uint32_t w = Profile()->m_videoWidth;
  uint32_t uvw = w / 2;

  CopyYuv(m_pic_input.img.plane[0],
	  m_pic_input.img.plane[1],
	  m_pic_input.img.plane[2], 
	  m_pic_input.img.i_stride[0],
	  m_pic_input.img.i_stride[1],
	  m_pic_input.img.i_stride[2],
	  pY, pU, pV, 
	  w, uvw, uvw, 
	  w, Profile()->m_videoHeight);
  return true;
}

void CX264VideoEncoder::StopEncoder (void)
{
#ifndef USE_OUR_YUV
  x264_picture_clean(&m_pic_input);
#endif
  x264_encoder_close(m_h);

  CHECK_AND_FREE(m_videoH264Seq);
  m_videoH264SeqSize = 0;
  CHECK_AND_FREE(m_videoH264Pic);
  m_videoH264PicSize = 0;

  CHECK_AND_FREE(m_vopBuffer);
  CHECK_AND_FREE(m_YUV);
  CHECK_AND_FREE(m_nal_info);
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

bool CX264VideoEncoder::GetEsConfig (uint8_t **ppEsConfig, 
				     uint32_t *pEsConfigLen)
{
  uint8_t *yuvbuf = (uint8_t *)malloc(Profile()->m_yuvSize);

#ifdef DEBUG_H264
  debug_message("Getting es config for x264");
#endif
  CHECK_AND_FREE(Profile()->m_videoMpeg4Config);
  Profile()->m_videoMpeg4ConfigLength = 0;

  error_message("Look at using x264_encoder_headers");
  if (yuvbuf == NULL) {
    error_message("xvid - Can't malloc memory for YUV for VOL");
    StopEncoder();
    return false;
  }
  // Create a dummy frame, and encode it, requesting an I frame
  // this should give us a VOL
  memset(yuvbuf, 0, Profile()->m_yuvSize);
  if (EncodeImage(yuvbuf,
		  yuvbuf + Profile()->m_ySize,
		  yuvbuf + Profile()->m_ySize + Profile()->m_uvSize,
		  Profile()->m_videoWidth,
		  Profile()->m_videoWidth / 2,
		  true,
		  0, 
		  0) == false &&
      m_vopBufferLength > 0) {
    error_message("x264 - encode image for param sets didn't work");
    free(yuvbuf);
    StopEncoder();
    return false;
  }
  free(yuvbuf);
  
  uint8_t *seqptr = m_vopBuffer;
  uint8_t *picptr = m_vopBuffer;
  uint32_t left = m_vopBufferLength;
  uint32_t seqlen = 0, piclen = 0;
  uint32_t nalsize;
  bool found_seq = false, found_pic = false;
  do {
    uint8_t nal_type = h264_nal_unit_type(seqptr);
    nalsize = h264_find_next_start_code(seqptr, left);
    if (nal_type == H264_NAL_TYPE_SEQ_PARAM) {
      found_seq = true;
      seqlen = nalsize == 0 ? left : nalsize;
    } else {
      seqptr += nalsize;
      if (nalsize == 0) 
	left = 0;
      else
	left -= nalsize;
    }
  } while (found_seq == false && left > 0);
  if (found_seq == false) {
    error_message("Couldn't find seq pointer in x264 frame");
    StopEncoder();
    return false;
  }

  left = m_vopBufferLength;
  do {
    uint8_t nal_type = h264_nal_unit_type(picptr);
    nalsize = h264_find_next_start_code(picptr, left);
    if (nal_type == H264_NAL_TYPE_PIC_PARAM) {
      found_pic = true;
      piclen = nalsize == 0 ? left : nalsize;
    } else {
      picptr += nalsize;
      if (nalsize == 0) 
	left = 0;
      else
	left -= nalsize;
    }
  } while (found_pic == false && left > 0);

  if (found_pic == false) {
    error_message("Couldn't find pic pointer in x264 frame");
    StopEncoder();
    return false;
  }

  uint8_t *p = seqptr;
  
  if (*p == 0 && p[1] == 0 && 
      (p[2] == 1 || (p[2] == 0 && p[3] == 1))) {
    if (p[2] == 0) p += 4;
    else p += 3;
  }
  Profile()->m_videoMpeg4ProfileId = p[1] << 16 |
    p[2] << 8 |
    p[3];

  debug_message("profile id %x", Profile()->m_videoMpeg4ProfileId);
  
  char *sprop = NULL;
  char *base64;
  base64 = MP4BinaryToBase64(seqptr, seqlen);
  sprop = strdup(base64);
  free(base64);
  base64 = MP4BinaryToBase64(picptr, piclen);
  sprop = (char *)realloc(sprop, strlen(sprop) + strlen(base64) + 1 + 1);
  strcat(sprop, ",");
  strcat(sprop, base64);
  free(base64);

  debug_message("sprop %s", sprop);
  Profile()->m_videoMpeg4Config = (uint8_t *)sprop;
  Profile()->m_videoMpeg4ConfigLength = strlen(sprop) + 1;
  StopEncoder();
  return true;
}

#endif
