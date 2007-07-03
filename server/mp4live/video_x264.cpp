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
static config_index_t CFG_X264_USE_PSNR;
static config_index_t CFG_X264_USE_SSIM;
static config_index_t CFG_X264_USE_8x8_DCT;
static config_index_t CFG_X264_LEVEL;
static config_index_t CFG_X264_PARTITIONS;
static config_index_t CFG_X264_ME;
static config_index_t CFG_X264_ME_RANGE;
static config_index_t CFG_X264_SUBME;
static config_index_t CFG_X264_USE_VBV;
static config_index_t CFG_X264_VBV_MAXRATE;
static config_index_t CFG_X264_VBV_BUFSIZE;
static config_index_t CFG_X264_BIT_RATE_TOLERANCE;
static config_index_t CFG_X264_THREADS;
static config_index_t CFG_X264_SAR_WIDTH;
static config_index_t CFG_X264_SAR_HEIGHT;

static SConfigVariable X264EncoderVariables[] = {
  CONFIG_BOOL_HELP(CFG_X264_FORCE_BASELINE, "x264ForceBaseline", false, "Turns off CABC and b-frames. Turns on VBV."),
  CONFIG_BOOL_HELP(CFG_X264_USE_CABAC, "x264UseCabac", true, "Turn on CABAC (Context-based Adaptive Binary Arithmetic Coding) (--no-cabac)"),
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
  CONFIG_BOOL(CFG_X264_USE_CBR, "x264UseCbr", true),
#endif
  CONFIG_BOOL_HELP(CFG_X264_USE_PSNR, "x264UsePsnr", true, "Enable Peak Signal-to-Noise Ratio computation (--no-psnr)"),
  CONFIG_BOOL_HELP(CFG_X264_USE_SSIM, "x264UseSsim", true, "Enable Structural Similarity Index computation (--no-ssim)"),
  CONFIG_BOOL_HELP(CFG_X264_USE_8x8_DCT, "x264Use8x8dct", false, "Adaptive spatial transform size (--8x8dct)"),
  CONFIG_STRING_HELP(CFG_X264_LEVEL, "x264Level", NULL, "Specify level (as defined by Annex A) (--level)"),
  CONFIG_STRING_HELP(CFG_X264_PARTITIONS, "x264Partitions", NULL, "Partitions to consider [\"p8x8,b8x8,i8x8,i4x4\"] (--partitions)\n     - p8x8, p4x4, b8x8, i8x8, i4x4\n     - none, all\n     (p4x4 requires p8x8. i8x8 requires --8x8dct.)"),
  CONFIG_STRING_HELP(CFG_X264_ME, "x264Me", NULL, "Integer pixel motion estimation method [\"hex\"] (--me)\n     - dia: diamond search, radius 1 (fast)\n     - hex: hexagonal search, radius 2\n     - umh: uneven multi-hexagon search\n     - esa: exhaustive search (slow)"),
  CONFIG_INT_HELP(CFG_X264_ME_RANGE, "x264MeRange", 16, "Maximum motion vector search range [16] (--merange)"),
  CONFIG_INT_HELP(CFG_X264_SUBME, "x264Subme", 5, "Subpixel motion estimation and partition decision quality: 1=fast, 7=best. [5] (--subme)"),
  CONFIG_BOOL(CFG_X264_USE_VBV, "x264UseVbv", false),
  CONFIG_INT_HELP(CFG_X264_VBV_MAXRATE, "x264VbvMaxrate", 0, "Max local bitrate (kbit/s) [0] (--vbv-maxrate)"),
  CONFIG_INT_HELP(CFG_X264_VBV_BUFSIZE, "x264VbvBufsize", 0, "Enable CBR and set size of the VBV buffer (kbit) [0] (--vbv-bufsize)"),
  CONFIG_FLOAT_HELP(CFG_X264_BIT_RATE_TOLERANCE, "x264BitRateTolerance", 1.0, "Allowed variance of average bitrate [1.0] (--ratetol)"),
  CONFIG_INT_HELP(CFG_X264_THREADS, "x264Threads", 0, "Parallel encoding (0=auto) (--threads)"),
  CONFIG_INT_HELP(CFG_X264_SAR_WIDTH, "x264SarWidth", 0, "Specify Sample Aspect Ratio Width (--sar width:height)"),
  CONFIG_INT_HELP(CFG_X264_SAR_HEIGHT, "x264SarHeight", 0, "Specify Sample Aspect Ratio Height (--sar width:height)"),
};

GUI_BOOL(gui_baseline, CFG_X264_FORCE_BASELINE, "Force Baseline");
GUI_BOOL(gui_cabac, CFG_X264_USE_CABAC, "Enable CABAC");
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
GUI_BOOL(gui_cbr, CFG_X264_USE_CBR, "Use CBR");
#endif
GUI_BOOL(gui_psnr, CFG_X264_USE_PSNR, "Enable PSNR");
GUI_BOOL(gui_ssim, CFG_X264_USE_SSIM, "Enable SSIM");
GUI_BOOL(gui_8x8dct, CFG_X264_USE_8x8_DCT, "Enable 8x8dct");
GUI_STRING(gui_level, CFG_X264_LEVEL, "Level");
GUI_STRING(gui_x264_partitions, CFG_X264_PARTITIONS, "Partitions");
GUI_STRING(gui_x264_me, CFG_X264_ME, "ME");
GUI_INT(gui_x264_me_range, CFG_X264_ME_RANGE, "ME Range");
GUI_INT(gui_x264_subme, CFG_X264_SUBME, "Subme");

GUI_BOOL(gui_bframe, CFG_VIDEO_USE_B_FRAMES, "Use B Frames");
GUI_INT_RANGE(gui_bframenum, CFG_VIDEO_NUM_OF_B_FRAMES, "Number of B frames", 1, 4);

GUI_BOOL(gui_vbv, CFG_X264_USE_VBV, "Use VBV Settings");
GUI_INT(gui_vbv_maxrate, CFG_X264_VBV_MAXRATE, "VBV Maxrate");
GUI_INT(gui_vbv_bufsize, CFG_X264_VBV_BUFSIZE, "VBV Buffer Size");
GUI_FLOAT_RANGE(gui_brate, CFG_X264_BIT_RATE_TOLERANCE, "Bit Rate Tolerance", .01, 100.0);
GUI_INT_RANGE(gui_threads, CFG_X264_THREADS, "Number of Threads", 0, 8);
GUI_INT_RANGE(gui_sar_w, CFG_X264_SAR_WIDTH, "Source Aspect Ratio Width", 0, 65536);
GUI_INT_RANGE(gui_sar_h, CFG_X264_SAR_HEIGHT, "Source Aspect Ratio Height", 0, 65536);

DECLARE_TABLE(x264_gui_options) = {
  TABLE_GUI(gui_baseline),
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
  TABLE_GUI(gui_cbr),
#endif
  TABLE_GUI(gui_level),
  TABLE_GUI(gui_x264_partitions),
  TABLE_GUI(gui_x264_me),
  TABLE_GUI(gui_x264_me_range),
  TABLE_GUI(gui_x264_subme),
  TABLE_GUI(gui_bframe),
  TABLE_GUI(gui_bframenum),
  TABLE_GUI(gui_cabac),
  TABLE_GUI(gui_psnr),
  TABLE_GUI(gui_ssim),
  TABLE_GUI(gui_8x8dct),
  TABLE_GUI(gui_vbv),
  TABLE_GUI(gui_vbv_maxrate),
  TABLE_GUI(gui_vbv_bufsize),
  TABLE_GUI(gui_brate),
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

static int x264_parse_enum( const char *arg, const char * const *names, int *dst ) {
    int i;
    for( i = 0; names[i]; i++ )
        if( !strcmp( arg, names[i] ) )
        {
            *dst = i;
            return 0;
        }
    return -1;
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
    Profile()->SetBoolValue(CFG_VIDEO_USE_B_FRAMES, false);
    Profile()->SetBoolValue(CFG_X264_USE_CABAC, false);
    Profile()->SetBoolValue(CFG_X264_USE_PSNR, false);
    Profile()->SetBoolValue(CFG_X264_USE_SSIM, false);
    Profile()->SetBoolValue(CFG_X264_USE_8x8_DCT, false);
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
  } else {
    m_param.i_bframe = 0;
  }
  //debug_message("h264 b frames %d", m_param.i_bframe);
  m_param.rc.i_bitrate = Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE);
#ifndef HAVE_X264_PARAM_T_RC_I_RC_METHOD
  m_param.rc.b_cbr = Profile()->GetBoolValue(CFG_X264_USE_CBR) ? 1 : 0;
#else
  m_param.rc.i_rc_method = X264_RC_ABR;
#endif
  m_param.rc.f_rate_tolerance = Profile()->GetFloatValue(CFG_X264_BIT_RATE_TOLERANCE);

  const char *level = Profile()->GetStringValue(CFG_X264_LEVEL);
  if(level != NULL && *level != '\0') {
  	if( strstr( level, "1b" ) ) {
      m_param.i_level_idc = 1;
  	} else {
      if( atof(level) < 6 )
        m_param.i_level_idc = (int)(10*atof(level)+.5);
      else
        m_param.i_level_idc = atoi(level);
  	}
  }
  
  const char *partitions = Profile()->GetStringValue(CFG_X264_PARTITIONS);
  if(partitions != NULL) {
    m_param.analyse.inter = 0;
    if( strstr( partitions, "none" ) )  m_param.analyse.inter =  0;
    if( strstr( partitions, "all" ) )   m_param.analyse.inter = ~0;

    if( strstr( partitions, "i4x4" ) )  m_param.analyse.inter |= X264_ANALYSE_I4x4;
    if( strstr( partitions, "i8x8" ) )  m_param.analyse.inter |= X264_ANALYSE_I8x8;
    if( strstr( partitions, "p8x8" ) )  m_param.analyse.inter |= X264_ANALYSE_PSUB16x16;
    if( strstr( partitions, "p4x4" ) )  m_param.analyse.inter |= X264_ANALYSE_PSUB8x8;
    if( strstr( partitions, "b8x8" ) )  m_param.analyse.inter |= X264_ANALYSE_BSUB16x16;
  }

  const char *me = Profile()->GetStringValue(CFG_X264_ME);
  if(me != NULL) {
    x264_parse_enum( me, x264_motion_est_names, &m_param.analyse.i_me_method );
  }

  m_param.analyse.i_me_range = Profile()->GetIntegerValue(CFG_X264_ME_RANGE);
  m_param.analyse.i_subpel_refine = Profile()->GetIntegerValue(CFG_X264_SUBME);
  
  if (Profile()->GetBoolValue(CFG_X264_USE_VBV)) {
    if (Profile()->GetBoolValue(CFG_X264_FORCE_BASELINE)) {
      switch(m_param.i_level_idc) {
      	case 10:
          if (Profile()->GetIntegerValue(CFG_X264_VBV_MAXRATE) > 64) {
            Profile()->SetIntegerValue(CFG_X264_VBV_MAXRATE, 64);
          }
          if (Profile()->GetIntegerValue(CFG_X264_VBV_BUFSIZE) > 175) {
            Profile()->SetIntegerValue(CFG_X264_VBV_BUFSIZE, 175);
          }
          break;
        case 1:
          // Acctually level 1b
          if (Profile()->GetIntegerValue(CFG_X264_VBV_MAXRATE) > 128) {
            Profile()->SetIntegerValue(CFG_X264_VBV_MAXRATE, 128);
          }
          if (Profile()->GetIntegerValue(CFG_X264_VBV_BUFSIZE) > 350) {
            Profile()->SetIntegerValue(CFG_X264_VBV_BUFSIZE, 350);
          }
          m_param.i_level_idc = 11;
          break;
      	case 11:
          if (Profile()->GetIntegerValue(CFG_X264_VBV_MAXRATE) > 192) {
            Profile()->SetIntegerValue(CFG_X264_VBV_MAXRATE, 192);
          }
          if (Profile()->GetIntegerValue(CFG_X264_VBV_BUFSIZE) > 500) {
            Profile()->SetIntegerValue(CFG_X264_VBV_BUFSIZE, 500);
          }
          break;
        case 12:
          if (Profile()->GetIntegerValue(CFG_X264_VBV_MAXRATE) > 384) {
            Profile()->SetIntegerValue(CFG_X264_VBV_MAXRATE, 384);
          }
          if (Profile()->GetIntegerValue(CFG_X264_VBV_BUFSIZE) > 1000) {
            Profile()->SetIntegerValue(CFG_X264_VBV_BUFSIZE, 1000);
          }
          break;
      	case 13:
          if (Profile()->GetIntegerValue(CFG_X264_VBV_MAXRATE) > 768) {
            Profile()->SetIntegerValue(CFG_X264_VBV_MAXRATE, 768);
          }
          if (Profile()->GetIntegerValue(CFG_X264_VBV_BUFSIZE) > 2000) {
            Profile()->SetIntegerValue(CFG_X264_VBV_BUFSIZE, 2000);
          }
          break;
      }
    }
    m_param.rc.i_vbv_max_bitrate = Profile()->GetIntegerValue(CFG_X264_VBV_MAXRATE);
    m_param.rc.i_vbv_buffer_size = Profile()->GetIntegerValue(CFG_X264_VBV_BUFSIZE);
  }
  //m_param.rc.b_stat_write = 0;
  //m_param.analyse.inter = 0;
  m_param.analyse.b_psnr = Profile()->GetBoolValue(CFG_X264_USE_PSNR) ? 1 : 0;
  m_param.analyse.b_ssim = Profile()->GetBoolValue(CFG_X264_USE_SSIM) ? 1 : 0;
  m_param.analyse.b_transform_8x8 = Profile()->GetBoolValue(CFG_X264_USE_8x8_DCT) ? 1 : 0;
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
