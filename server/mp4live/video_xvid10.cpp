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
 *		Bill May wmay@cisco.com
 *              Peter Maersk-Moller peter@maersk-moller.net (interlace)
 */

#include "mp4live.h"
#include <mp4av.h>
#include "video_encoder.h"
#include "video_xvid10.h"

static config_index_t CFG_XVID10_VIDEO_QUALITY;
static config_index_t CFG_XVID10_USE_GMC;
static config_index_t CFG_XVID10_USE_QPEL;
static config_index_t CFG_XVID10_USE_LUMIMASK;
static config_index_t CFG_XVID10_USE_INTERLACING;


static SConfigVariable XvidEncoderVariables[] = {
  CONFIG_INT(CFG_XVID10_VIDEO_QUALITY, "xvid10VideoQuality", 6),
  CONFIG_BOOL(CFG_XVID10_USE_GMC, "xvid10UseGmc", false),
  CONFIG_BOOL(CFG_XVID10_USE_QPEL, "xvid10UseQpel", false),
  CONFIG_BOOL(CFG_XVID10_USE_LUMIMASK, "xvid10UseLumimask", false),
  CONFIG_BOOL(CFG_XVID10_USE_INTERLACING, "xvid10UseInterlacing", false),
};

GUI_BOOL(gui_gmc, CFG_XVID10_USE_GMC, "Use GMC");
GUI_BOOL(gui_qpel, CFG_XVID10_USE_QPEL, "Use QPel");
GUI_BOOL(gui_lumimask, CFG_XVID10_USE_LUMIMASK, "Use Lumimask");
GUI_BOOL(gui_interlace, CFG_XVID10_USE_INTERLACING, "Use Interlacing");
GUI_INT_RANGE(gui_vq, CFG_XVID10_VIDEO_QUALITY, "Video Quality", 0, 6);

DECLARE_TABLE(xvid_gui_options) = {
  TABLE_GUI(gui_gmc),
  TABLE_GUI(gui_qpel),
  TABLE_GUI(gui_lumimask),
  TABLE_GUI(gui_interlace),
  TABLE_GUI(gui_vq),
};
DECLARE_TABLE_FUNC(xvid_gui_options);

void AddXvid10ConfigVariables(CVideoProfile *pConfig) 
{
  pConfig->AddConfigVariables(XvidEncoderVariables,
			      NUM_ELEMENTS_IN_ARRAY(XvidEncoderVariables));
}

CXvid10VideoEncoder::CXvid10VideoEncoder(CVideoProfile *vp, 
					 uint16_t mtu,
					 CVideoEncoder *next, 
					 bool realTime) :
  CVideoEncoder(vp, mtu, next, realTime)
{
	m_xvidHandle = NULL;
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;
#ifdef WRITE_RAW
	m_outfile = fopen("raw.m4v", FOPEN_WRITE_BINARY);
#endif
	m_video_quality = 6;
	m_use_gmc = false;
	m_use_qpel = true;
	m_use_lumimask_plugin = false;
}

bool CXvid10VideoEncoder::Init(void)
{
	xvid_gbl_init_t xvid_gbl_init;

	m_video_quality = Profile()->GetIntegerValue(CFG_XVID10_VIDEO_QUALITY);
	if (m_video_quality > 6) m_video_quality = 6;
	m_use_gmc = Profile()->GetBoolValue(CFG_XVID10_USE_GMC);
	m_use_qpel = Profile()->GetBoolValue(CFG_XVID10_USE_QPEL);
	m_use_interlacing = Profile()->GetBoolValue(CFG_XVID10_USE_INTERLACING);
	m_use_lumimask_plugin = 
	  Profile()->GetBoolValue(CFG_XVID10_USE_LUMIMASK);
	m_use_par = 
	  Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_WIDTH) != 0 &&
	  Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_HEIGHT) != 0;
	if (m_use_par) {
	  int par_w = Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_WIDTH);
	  int par_h = Profile()->GetIntegerValue(CFG_VIDEO_MPEG4_PAR_HEIGHT);

	  if (par_w > 255 || par_h > 255) {
	    error_message("PAR parameters are > 255 - not used");
	    m_use_par = false;
	  } else {
	    m_par = XVID_PAR_11_VGA;
	    m_par_height = m_par_width = 0;
	    if (par_w == 12 && par_h == 11) m_par = XVID_PAR_43_PAL;
	    else if (par_w == 10 && par_h == 11) m_par = XVID_PAR_43_NTSC;
	    else if (par_w == 16 && par_h == 9) m_par = XVID_PAR_169_PAL;
	    else if (par_w == 40 && par_h == 33) m_par = XVID_PAR_169_NTSC;
	    else {
	      m_par = XVID_PAR_EXT;
	      m_par_width = par_w;
	      m_par_height = par_h;
	    }
	  }
	}

	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init));
	xvid_gbl_init.version = XVID_VERSION;
	xvid_global(NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL);
	xvid_gbl_info_t xvid_info;
	memset(&xvid_info, 0, sizeof(xvid_info));
	xvid_info.version = XVID_VERSION;
	xvid_global(NULL, XVID_GBL_INFO, &xvid_info, NULL);
	debug_message("xvid info - vers %x build %s flags %x threads %d", 
		      xvid_info.actual_version,
		      xvid_info.build,
		      xvid_info.cpu_flags, 
		      xvid_info.num_threads);
	xvid_enc_create_t enc_create;
	memset(&enc_create, 0, sizeof(enc_create));
	enc_create.version = XVID_VERSION;
	enc_create.width = Profile()->m_videoWidth;
	enc_create.height = Profile()->m_videoHeight;
	enc_create.profile = Profile()->m_videoMpeg4ProfileId;
       
	xvid_enc_zone_t zones;
	enc_create.zones = &zones;
	enc_create.num_zones = 0;
	// We might want to create our own plugin to get the
	// reconstructed image pointer.
	//
	// We might also consider using a 2 pass system, as well.
	xvid_enc_plugin_t plugins[2];
	xvid_plugin_single_t single;
	memset(&single, 0, sizeof(single));
	single.version = XVID_VERSION;
	single.bitrate = 
	  Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000;

	plugins[0].func = xvid_plugin_single;
	plugins[0].param = &single;
	if (m_use_lumimask_plugin) {
	  plugins[1].func = xvid_plugin_lumimasking;
	  plugins[1].param = NULL;
	  enc_create.num_plugins = 2;
	} else {
	  enc_create.num_plugins = 1;
	}
	enc_create.plugins = plugins;
	
	if (Profile()->GetIntegerValue(CFG_VIDEO_TIMEBITS) == 0) {
	  enc_create.fincr = 1;
	  enc_create.fbase = 
	    (int)(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) + 0.5);
	} else {
	  enc_create.fincr = 
	    (int)(((double)Profile()->GetIntegerValue(CFG_VIDEO_TIMEBITS)) /
		  Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE));
	  enc_create.fbase = 
	    Profile()->GetIntegerValue(CFG_VIDEO_TIMEBITS);
	}

	enc_create.max_key_interval = (int)
		(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) 
		 * Profile()->GetFloatValue(CFG_VIDEO_KEY_FRAME_INTERVAL));
	
	// We may be able to do b-frames at some point - this means
	// work in the rtp transmitter, as well as recording the pts
	// and changing the file recorder as well (see mpeg2 work)
	enc_create.max_bframes = 0;
	enc_create.bquant_ratio = 150;
	enc_create.bquant_offset = 100;

	// There are also a number of options for quality that we
	// may or may not want to deal with.  See the examples for that.
	int xerr = xvid_encore(NULL, XVID_ENC_CREATE, &enc_create, NULL);

	if (xerr) {
		error_message("Failed to initialize Xvid encoder %d", xerr);
		return false;
	}

	m_xvidHandle = enc_create.handle; 

	return true;
}

static struct {
  int vop;
  int motion;
} xvid10_quality[] = {
  { 0, 0, },
  { 0, XVID_ME_ADVANCEDDIAMOND16, },
  { XVID_VOP_HALFPEL, (XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16),},
  { (XVID_VOP_HALFPEL | XVID_VOP_INTER4V),
    (XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
     XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8), },
  { (XVID_VOP_HALFPEL | XVID_VOP_INTER4V),
    (XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
     XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
     XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP), },
  { (XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
     XVID_VOP_TRELLISQUANT),
    (XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
     XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
     XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP), },
  {(XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
    XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED),
   (XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |
    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP),},
};


bool CXvid10VideoEncoder::EncodeImage(
	const u_int8_t* pY, 
	const u_int8_t* pU, 
	const u_int8_t* pV, 
	u_int32_t yStride,
	u_int32_t uvStride,
	bool wantKeyFrame,
	Duration Elapsed,
	Timestamp srcFrameTimestamp)
{
  m_srcFrameTimestamp = srcFrameTimestamp;
	xvid_enc_frame_t 		xvidFrame;
	CHECK_AND_FREE(m_vopBuffer);
	m_vopBuffer = (u_int8_t*)malloc(Profile()->m_videoMaxVopSize);
	if (m_vopBuffer == NULL) {
		return false;
	}

	memset(&xvidFrame, 0, sizeof(xvidFrame));
	memset(&m_xvidResult, 0, sizeof(m_xvidResult));


	xvidFrame.version = XVID_VERSION;
	xvidFrame.vop_flags = xvid10_quality[m_video_quality].vop;
	xvidFrame.motion = xvid10_quality[m_video_quality].motion;
	if (m_use_gmc) {
	  xvidFrame.vol_flags |= XVID_VOL_GMC;
	  xvidFrame.motion |= XVID_ME_GME_REFINE;
	}
	if (m_use_qpel) {
	  xvidFrame.vol_flags |= XVID_VOL_QUARTERPEL;
	  xvidFrame.motion |= XVID_ME_QUARTERPELREFINE16;
	  if (xvidFrame.vop_flags & XVID_VOP_INTER4V) {
	    xvidFrame.motion |= XVID_ME_QUARTERPELREFINE8;
	  }
	}
	if (m_use_interlacing) {
	  xvidFrame.vol_flags |= XVID_VOL_INTERLACING;
	}
	  
	xvidFrame.bitstream = m_vopBuffer;
	xvidFrame.length = Profile()->m_videoMaxVopSize;
	xvidFrame.input.csp = XVID_CSP_PLANAR;
	xvidFrame.input.plane[0] = (void *)pY;
	xvidFrame.input.stride[0] = yStride;
	xvidFrame.input.plane[1] = (void *)pU;
	xvidFrame.input.stride[1] = uvStride;
	xvidFrame.input.plane[2] = (void *)pV;
	xvidFrame.input.stride[2] = uvStride;
	if (m_use_par) {
	  xvidFrame.par = m_par;
	  xvidFrame.par_width = m_par_width;
	  xvidFrame.par_height = m_par_height;
	}
	// if we do quality work, we will need to set the motion 
	// variable accordingly.
	m_xvidResult.version = XVID_VERSION;

	if (wantKeyFrame) {
	  xvidFrame.type = XVID_TYPE_IVOP;
	} else {
	  xvidFrame.type = XVID_TYPE_AUTO;
	}

	m_vopBufferLength = xvid_encore(m_xvidHandle, XVID_ENC_ENCODE, &xvidFrame, 
					&m_xvidResult);
#ifdef WRITE_RAW
	fwrite(m_vopBuffer, 1, m_vopBufferLength, m_outfile);
#endif
	//	message(LOG_DEBUG, "xvid", "return from encore %d %d", m_vopBufferLength, m_xvidResult.type);
	if (m_vopBufferLength <= 0) {
		debug_message("Xvid can't encode frame!");
		CHECK_AND_FREE(m_vopBuffer);
		return false;
	}

	return true;
}

bool CXvid10VideoEncoder::GetEncodedImage(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength,
	Timestamp *dts, Timestamp *pts)
{
  // will have to change this if we do b frames
  *dts = *pts = m_srcFrameTimestamp; 
	*ppBuffer = m_vopBuffer;
	*pBufferLength = m_vopBufferLength;

	m_vopBuffer = NULL;
	m_vopBufferLength = 0;

	return *pBufferLength != 0;
}

bool CXvid10VideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{
  // we may be able to do this with a plugin
  return false;
#if 0

	imgcpy(pY, (u_int8_t*)m_m_xvidResult.image_y,
		Profile()->m_videoWidth, 
		Profile()->m_videoHeight,
		m_m_xvidResult.stride_y);

	imgcpy(pU, (u_int8_t*)m_m_xvidResult.image_u,
		Profile()->m_videoWidth / 2, 
		Profile()->m_videoHeight / 2,
		m_m_xvidResult.stride_u);

	imgcpy(pV, (u_int8_t*)m_m_xvidResult.image_v,
		Profile()->m_videoWidth / 2, 
		Profile()->m_videoHeight / 2,
		m_m_xvidResult.stride_u);
	return true;
#endif
}

void CXvid10VideoEncoder::StopEncoder (void)
{
  CHECK_AND_FREE(m_vopBuffer);
  xvid_encore(m_xvidHandle, XVID_ENC_DESTROY, NULL, NULL);
  m_xvidHandle = NULL;
#ifdef WRITE_RAW
  if (m_outfile) {
    fclose(m_outfile);
    m_outfile = NULL;
  }
#endif
}

/*
 * Ah - fun - pull the vol out of the encoder - hope it doesn't
 * change...
 */
bool CXvid10VideoEncoder::GetEsConfig(uint8_t **ppEsConfig,
				      uint32_t *pEsConfigLen)
{
  uint8_t *yuvbuf = (uint8_t *)malloc(Profile()->m_yuvSize);

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
    error_message("xvid - encode image for VOL didn't work");
    free(yuvbuf);
    StopEncoder();
    return false;
  }
  free(yuvbuf);
  uint8_t *volptr, *vopptr;
  volptr = MP4AV_Mpeg4FindVol(m_vopBuffer, m_vopBufferLength);
  if (volptr == NULL) {
    error_message("xvid - didn't put vol in header");
    StopEncoder();
    return false;
  }
  int32_t offset;
  // I know, I'm using a mpeg1/2 routine, but it works
  if ((offset = 
       MP4AV_Mpeg4FindHeader(volptr + 1,
			     m_vopBufferLength - 1 - (volptr - m_vopBuffer))) 
      < 0) {

    error_message("xvid - can't find vop after vol");
    StopEncoder();
    return false;
  }
  // not really a vop pointer, but pointer to next frame
  vopptr = volptr + 1 + offset;

  *ppEsConfig = m_vopBuffer;
  *pEsConfigLen = (vopptr - m_vopBuffer);
  RemoveUserdataFromVol(ppEsConfig, pEsConfigLen);
  m_vopBuffer = NULL;
  StopEncoder();
  return true;
}
  
