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
 *     SAR width and height: Peter Maersk-Moller peter@maersk-moller.net
 */

#include "mp4live.h"
#ifdef HAVE_X264
#include "video_x264.h"
#include "mp4av.h"
#include "mp4av_h264.h"
#include "encoder_gui_options.h"

//#define DEBUG_H264 1
#define USE_OUR_YUV 1

static config_index_t CFG_X264_FORCE_BASELINE;
static config_index_t CFG_X264_USE_CABAC;
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
static config_index_t CFG_X264_USE_CBR;
#endif
static config_index_t CFG_X264_BIT_RATE_TOLERANCE;
static config_index_t CFG_X264_USE_VBV;
static config_index_t CFG_X264_VBV_BITRATE_MULT;
static config_index_t CFG_X264_VBV_BUFFER_SIZE_MULT;
static config_index_t CFG_X264_THREADS;
static config_index_t CFG_X264_SAR_WIDTH;
static config_index_t CFG_X264_SAR_HEIGHT;

static SConfigVariable X264EncoderVariables[] = {
  CONFIG_BOOL(CFG_X264_FORCE_BASELINE, "x264ForceBaseline", false),
  CONFIG_BOOL(CFG_X264_USE_CABAC, "x264UseCabac", true),
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
  CONFIG_BOOL(CFG_X264_USE_CBR, "x264UseCbr", true),
#endif
  CONFIG_FLOAT(CFG_X264_BIT_RATE_TOLERANCE, "x264BitRateTolerance", 1.0),
  CONFIG_BOOL(CFG_X264_USE_VBV, "x264UseVbv", false),
  CONFIG_FLOAT(CFG_X264_VBV_BITRATE_MULT, "x264VbvBitRateMult", 1.0),
  CONFIG_FLOAT(CFG_X264_VBV_BUFFER_SIZE_MULT, "x264VbvBufferSizeMult", 10.0),
  CONFIG_INT(CFG_X264_THREADS, "x264Threads", 1),
  CONFIG_INT(CFG_X264_SAR_WIDTH, "x264SarWidth", 0),
  CONFIG_INT(CFG_X264_SAR_HEIGHT, "x264SarHeight", 0),
};

GUI_BOOL(gui_baseline, CFG_X264_FORCE_BASELINE, "Force Baseline (overrides below)");
GUI_BOOL(gui_cabac, CFG_X264_USE_CABAC, "Use Cabac");
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
GUI_BOOL(gui_cbr, CFG_X264_USE_CBR, "Use CBR");
#endif
GUI_BOOL(gui_bframe, CFG_VIDEO_USE_B_FRAMES, "Use B Frames");
GUI_INT_RANGE(gui_bframenum, CFG_VIDEO_NUM_OF_B_FRAMES, "Number of B frames", 1, 4);

GUI_FLOAT_RANGE(gui_brate, CFG_X264_BIT_RATE_TOLERANCE, "Bit Rate Tolerance", 
		.01, 100.0);

GUI_BOOL(gui_vbv, CFG_X264_USE_VBV, "Use VBV Settings");
GUI_FLOAT_RANGE(gui_vbvm, CFG_X264_VBV_BITRATE_MULT, 
		"VBV Bit Rate Multiplier", 1.0, 2.0);
GUI_FLOAT_RANGE(gui_vbv2, CFG_X264_VBV_BUFFER_SIZE_MULT,
		"VBV Buffer Size Multiplier", 0.0001, 100.0);
GUI_INT_RANGE(gui_threads, CFG_X264_THREADS, "Number of Threads", 1, 8);
GUI_INT_RANGE(gui_sar_w, CFG_X264_SAR_WIDTH, "Source Aspect Ratio Width", 0, 65536);
GUI_INT_RANGE(gui_sar_h, CFG_X264_SAR_HEIGHT, "Source Aspect Ratio Height", 0, 65536);

DECLARE_TABLE(x264_gui_options) = {
  TABLE_GUI(gui_baseline),
  TABLE_GUI(gui_cabac),
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
  TABLE_GUI(gui_cbr),
#endif
  TABLE_GUI(gui_bframe),
  TABLE_GUI(gui_bframenum),
  TABLE_GUI(gui_brate),
  TABLE_GUI(gui_vbv),
  TABLE_GUI(gui_vbvm),
  TABLE_GUI(gui_vbv2),
  TABLE_GUI(gui_threads),
  TABLE_GUI(gui_sar_w),
  TABLE_GUI(gui_sar_h),
};
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
  m_push = new CTimestampPush(6);

#ifdef OUTPUT_RAW
  m_outfile = fopen("raw.h264", FOPEN_WRITE_BINARY);
#endif

  if (Profile()->GetBoolValue(CFG_X264_FORCE_BASELINE)) {
    Profile()->SetBoolValue(CFG_X264_USE_CABAC, false);
    Profile()->SetBoolValue(CFG_VIDEO_USE_B_FRAMES, false);
    Profile()->SetBoolValue(CFG_X264_USE_VBV, true);
    if (Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) > 768) {
      Profile()->SetIntegerValue(CFG_VIDEO_BIT_RATE, 768);
    }
  }
  
  x264_param_default(&m_param);
  m_param.i_width = Profile()->m_videoWidth;
  m_param.i_height = Profile()->m_videoHeight;
  m_param.vui.i_sar_width = Profile()->GetIntegerValue(CFG_X264_SAR_WIDTH);
  m_param.vui.i_sar_height = Profile()->GetIntegerValue(CFG_X264_SAR_HEIGHT);
  m_param.i_fps_num = 
    (int)((Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) + .5) * 1000);
  m_param.i_fps_den = 1000;
  //m_param.i_maxframes = 0;
  m_key_frame_count = m_param.i_keyint_max = 
    (int)(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) * 
	  Profile()->GetFloatValue(CFG_VIDEO_KEY_FRAME_INTERVAL));
  if (Profile()->GetBoolValue(CFG_VIDEO_USE_B_FRAMES)) {
    m_param.i_bframe = Profile()->GetIntegerValue(CFG_VIDEO_NUM_OF_B_FRAMES);
  } else 
    m_param.i_bframe = 0;
  //debug_message("h264 b frames %d", m_param.i_bframe);
  m_param.rc.i_bitrate = Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE);
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
  m_param.rc.b_cbr = Profile()->GetBoolValue(CFG_X264_USE_CBR) ? 1 : 0;
#else
  m_param.rc.i_rc_method = X264_RC_ABR;
#endif
  m_param.rc.f_rate_tolerance = Profile()->GetFloatValue(CFG_X264_BIT_RATE_TOLERANCE);
  if (Profile()->GetBoolValue(CFG_X264_USE_VBV)) {
    if (Profile()->GetBoolValue(CFG_X264_FORCE_BASELINE)) {
      m_param.rc.i_vbv_max_bitrate = 768;
      m_param.rc.i_vbv_buffer_size = 2000;
      m_param.i_level_idc = 13;
    } else {
      m_param.rc.i_vbv_max_bitrate = (int)
	((float)m_param.rc.i_bitrate * 
	 Profile()->GetFloatValue(CFG_X264_VBV_BITRATE_MULT));
      float calc;
      calc = m_param.rc.i_bitrate;
      calc *= Profile()->GetFloatValue(CFG_X264_VBV_BUFFER_SIZE_MULT);
      calc /= Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE);
      m_param.rc.i_vbv_buffer_size = (int)calc;
    }
  }
  //m_param.rc.b_stat_write = 0;
  //m_param.analyse.inter = 0;
  m_param.analyse.b_psnr = 0;
  m_param.b_cabac = Profile()->GetBoolValue(CFG_X264_USE_CABAC) ? 1 : 0;
  m_param.pf_log = x264_log;

  m_param.i_threads = Profile()->GetIntegerValue(CFG_X264_THREADS);
  m_pts_add = m_param.i_bframe ? (m_param.b_bframe_pyramid ? 2 : 1) : 0;
  m_pts_add *= m_frame_time;
  debug_message("pts add "D64, m_pts_add);

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
  //debug_message("encoding at "U64, srcFrameTimestamp);
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
      m_nal_info[nal_on].unique = false;
      uint32_t header = 0;
      if (h264_is_start_code(vopBuffer)) {
	header = vopBuffer[2] == 1 ? 3 : 4;
      }
      m_nal_info[nal_on].nal_length -= header;
      m_nal_info[nal_on].nal_offset += header;
      // we will send picture or sequence header through - let the
      // sinks decide to send or not
      if (skip == false) {
#ifdef DEBUG_H264
	uint8_t nal_type = nal[ix].i_type;
	if (h264_nal_unit_type_is_slice(nal_type)) {
	  uint8_t stype;
	  h264_find_slice_type(vopBuffer, i_size, &stype, true);
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

#if 1
  Timestamp pts_try = m_pic_output.i_pts + m_pts_add;
  Timestamp closest_on_stack = m_push->Closest(pts_try, m_frame_time);
  if (closest_on_stack != 0) {
    *pts = closest_on_stack;
  } else {
    *pts = pts_try;
  }
  //  debug_message("try "U64" closest "U64, pts_try, closest_on_stack);
#else 
  *pts = m_pic_output.i_pts + m_pts_add;
#endif
  *dts = m_push->Pop();
  //  debug_message("dts "U64" pts "U64" "D64" type %u ", *dts, *pts, *pts - *dts, m_pic_output.i_type);
  if (*dts > *pts) *pts = *dts;
  else if (*pts - *dts < 6) *pts = *dts;
#if 0
  if (*dts != *pts) {
    debug_message("PTS "U64" not DTS "U64, 
		  *pts, *dts);
  }
#endif
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
#ifdef DEBUG_H264
  debug_message("Getting es config for x264");
#endif
  CHECK_AND_FREE(Profile()->m_videoMpeg4Config);
  Profile()->m_videoMpeg4ConfigLength = 0;

  x264_nal_t *nal;
  int nal_num;
  if (x264_encoder_headers(m_h, &nal, &nal_num) != 0) {
    error_message("x264 - can't create headers");
    StopEncoder();
    return false;
  }
  
  uint8_t *seqptr = m_vopBuffer;
  uint8_t *picptr = m_vopBuffer;
  uint32_t seqlen = 0, piclen = 0;
  bool found_seq = false, found_pic = false;
  if (m_vopBuffer == NULL) {
    m_vopBuffer = (u_int8_t*)malloc(Profile()->m_videoMaxVopSize);
  }
  uint8_t *vopBuffer = m_vopBuffer;
  int vopBufferLen = Profile()->m_videoMaxVopSize;

  for (int ix = 0; ix < nal_num; ix++) {
    int i_size;
    i_size = x264_nal_encode(vopBuffer, &vopBufferLen, 1, &nal[ix]);
    if (i_size > 0) {
      bool useit = false;
      uint header_size = 0;
      if (h264_is_start_code(vopBuffer)) {
	header_size = vopBuffer[2] == 1 ? 3 : 4;
      }
      if (nal[ix].i_type == H264_NAL_TYPE_SEQ_PARAM) {
	found_seq = true;
	seqlen = i_size - header_size;
	seqptr = vopBuffer + header_size;
	useit = true;
      } else if (nal[ix].i_type == H264_NAL_TYPE_PIC_PARAM) {
	found_pic = true;
	piclen = i_size - header_size;
	picptr = vopBuffer + header_size;
	useit = true;
      }
      if (useit) {
	vopBuffer += i_size;
	vopBufferLen -= i_size;
      }
    }
  }
	  
  if (found_seq == false) {
    error_message("Can't find seq pointer in x264 header");
    StopEncoder();
    return false;
  }
  if (found_pic == false) {
    error_message("Can't find pic pointer in x264 header");
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
