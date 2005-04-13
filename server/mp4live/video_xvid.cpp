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
#include "video_xvid.h"

CXvidVideoEncoder::CXvidVideoEncoder(CVideoProfile *vp, 
				     uint16_t mtu,
				     CVideoEncoder *next, 
				     bool realTime) :
  CVideoEncoder(vp, mtu, next, realTime)
{
	m_xvidHandle = NULL;
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;
}

bool CXvidVideoEncoder::Init (void)
{
	XVID_INIT_PARAM xvidInitParams;

	memset(&xvidInitParams, 0, sizeof(xvidInitParams));

	if (xvid_init(NULL, 0, &xvidInitParams, NULL) != XVID_ERR_OK) {
		error_message("Failed to initialize Xvid");
		return false;
	}

	XVID_ENC_PARAM xvidEncParams;

	memset(&xvidEncParams, 0, sizeof(xvidEncParams));

	xvidEncParams.width = Profile()->m_videoWidth;
	xvidEncParams.height = Profile()->m_videoHeight;
	if (Profile()->GetIntegerValue(CFG_VIDEO_TIMEBITS) == 0) {
	  xvidEncParams.fincr = 1;
	  xvidEncParams.fbase = 
	    (int)(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) + 0.5);
	} else {
	  xvidEncParams.fincr = 
	    (int)(((double)Profile()->GetIntegerValue(CFG_VIDEO_TIMEBITS)) /
		  Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE));
	  xvidEncParams.fbase = 
	    Profile()->GetIntegerValue(CFG_VIDEO_TIMEBITS);
	}

	xvidEncParams.rc_bitrate = 
		Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000;
	xvidEncParams.min_quantizer = 1;
	xvidEncParams.max_quantizer = 31;
	xvidEncParams.max_key_interval = (int)
		(Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE) 
		 * Profile()->GetFloatValue(CFG_VIDEO_KEY_FRAME_INTERVAL));
	if (xvidEncParams.max_key_interval == 0) {
		xvidEncParams.max_key_interval = 1;
	} 

	if (xvid_encore(NULL, XVID_ENC_CREATE, &xvidEncParams, NULL) 
	  != XVID_ERR_OK) {
		error_message("Failed to initialize Xvid encoder");
		return false;
	}

	m_xvidHandle = xvidEncParams.handle; 

	memset(&m_xvidFrame, 0, sizeof(m_xvidFrame));

	m_xvidFrame.general = XVID_HALFPEL | XVID_H263QUANT;
	if (!m_realTime) {
		m_xvidFrame.motion = 
			PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 
			| PMV_EARLYSTOP8 | PMV_HALFPELDIAMOND8;
	} else {
		m_xvidFrame.motion = PMV_QUICKSTOP16;
	}
		if (!m_realTime) {
			m_xvidFrame.general |= XVID_INTER4V;
		}
	m_xvidFrame.colorspace = XVID_CSP_I420;
	m_xvidFrame.quant = 0;
#ifdef WRITE_RAW
	m_raw_file = fopen("raw.xvid", FOPEN_WRITE_BINARY);
#endif
	return true;
}

bool CXvidVideoEncoder::EncodeImage(
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
	m_vopBuffer = (u_int8_t*)malloc(Profile()->m_videoMaxVopSize);
	if (m_vopBuffer == NULL) {
		return false;
	}

	XVID_DEC_PICTURE decpict;
	decpict.y = (void *)pY;
	decpict.u = (void *)pU;
	decpict.v = (void *)pV;
	decpict.stride_y = yStride;
	decpict.stride_u = uvStride;
	m_xvidFrame.image = &decpict;
	m_xvidFrame.colorspace = XVID_CSP_USER;

	m_xvidFrame.bitstream = m_vopBuffer;
	m_xvidFrame.intra = (wantKeyFrame ? 1 : -1);

	if (xvid_encore(m_xvidHandle, XVID_ENC_ENCODE, &m_xvidFrame, 
	  &m_xvidResult) != XVID_ERR_OK) {
		debug_message("Xvid can't encode frame!");
		CHECK_AND_FREE(m_vopBuffer);
		return false;
	}

	m_vopBufferLength = m_xvidFrame.length;

	return true;
}

bool CXvidVideoEncoder::GetEncodedImage(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength,
	Timestamp *dts, Timestamp *pts)
{
  // will have to change this if we do b frames
  *dts = *pts = m_srcFrameTimestamp; 
	*ppBuffer = m_vopBuffer;
	*pBufferLength = m_vopBufferLength;

#ifdef WRITE_RAW
	if (m_vopBufferLength > 0) {
	  fwrite(m_vopBuffer, 1, m_vopBufferLength, m_raw_file);
	}
#endif
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;

	return *pBufferLength != 0;
}

bool CXvidVideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{

	return false;
}

void CXvidVideoEncoder::StopEncoder()
{
  CHECK_AND_FREE(m_vopBuffer);
  xvid_encore(m_xvidHandle, XVID_ENC_DESTROY, NULL, NULL);
  m_xvidHandle = NULL;
#ifdef WRITE_RAW
  fclose(m_raw_file);
#endif
}

