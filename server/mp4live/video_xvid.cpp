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

CXvidVideoEncoder::CXvidVideoEncoder()
{
	m_xvidHandle = NULL;
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;
}

bool CXvidVideoEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
	m_pConfig = pConfig;

	XVID_INIT_PARAM xvidInitParams;

	memset(&xvidInitParams, 0, sizeof(xvidInitParams));

	if (xvid_init(NULL, 0, &xvidInitParams, NULL) != XVID_ERR_OK) {
		error_message("Failed to initialize Xvid");
		return false;
	}

	XVID_ENC_PARAM xvidEncParams;

	memset(&xvidEncParams, 0, sizeof(xvidEncParams));

	xvidEncParams.width = m_pConfig->m_videoWidth;
	xvidEncParams.height = m_pConfig->m_videoHeight;
	if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_TIMEBITS) == 0) {
	  xvidEncParams.fincr = 1;
	  xvidEncParams.fbase = 
	    (int)(m_pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE) + 0.5);
#ifndef HAVE_XVID_H
	  xvidEncParams.dont_simplify_fincr = 0;
#endif
	} else {
	  xvidEncParams.fincr = 
	    (int)(((double)m_pConfig->GetIntegerValue(CONFIG_VIDEO_TIMEBITS)) /
		  m_pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE));
	  xvidEncParams.fbase = 
	    m_pConfig->GetIntegerValue(CONFIG_VIDEO_TIMEBITS);
#ifndef HAVE_XVID_H
	  xvidEncParams.dont_simplify_fincr = 1;
#endif
	}

#ifndef HAVE_XVID_H
	xvidEncParams.bitrate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000;
	xvidEncParams.rc_buffersize = 16;
#else
	xvidEncParams.rc_bitrate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000;
#endif
	xvidEncParams.min_quantizer = 1;
	xvidEncParams.max_quantizer = 31;
	xvidEncParams.max_key_interval = (int)
		(m_pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE) 
		 * m_pConfig->GetFloatValue(CONFIG_VIDEO_KEY_FRAME_INTERVAL));
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
	if (!realTime) {
		m_xvidFrame.motion = 
			PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 
			| PMV_EARLYSTOP8 | PMV_HALFPELDIAMOND8;
	} else {
		m_xvidFrame.motion = PMV_QUICKSTOP16;
	}
		if (!realTime) {
			m_xvidFrame.general |= XVID_INTER4V;
		}
	m_xvidFrame.colorspace = XVID_CSP_I420;
	m_xvidFrame.quant = 0;

	return true;
}

bool CXvidVideoEncoder::EncodeImage(
	u_int8_t* pY, 
	u_int8_t* pU, 
	u_int8_t* pV, 
	u_int32_t yStride,
	u_int32_t uvStride,
	bool wantKeyFrame,
	Duration Elapsed)
{
	m_vopBuffer = (u_int8_t*)malloc(m_pConfig->m_videoMaxVopSize);
	if (m_vopBuffer == NULL) {
		return false;
	}

#ifndef HAVE_XVID_H
	m_xvidFrame.image_y = pY;
	m_xvidFrame.image_u = pU;
	m_xvidFrame.image_v = pV;
	m_xvidFrame.stride = yStride;
#else
	XVID_DEC_PICTURE decpict;
	decpict.y = pY;
	decpict.u = pU;
	decpict.v = pV;
	decpict.stride_y = yStride;
	decpict.stride_u = uvStride;
	m_xvidFrame.image = &decpict;
	m_xvidFrame.colorspace = XVID_CSP_USER;
#endif
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
	u_int8_t** ppBuffer, u_int32_t* pBufferLength)
{
	*ppBuffer = m_vopBuffer;
	*pBufferLength = m_vopBufferLength;

	m_vopBuffer = NULL;
	m_vopBufferLength = 0;

	return true;
}

bool CXvidVideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{

#ifndef HAVE_XVID_H	
	imgcpy(pY, (u_int8_t*)m_xvidResult.image_y,
		m_pConfig->m_videoWidth, 
		m_pConfig->m_videoHeight,
		m_xvidResult.stride_y);

	imgcpy(pU, (u_int8_t*)m_xvidResult.image_u,
		m_pConfig->m_videoWidth / 2, 
		m_pConfig->m_videoHeight / 2,
		m_xvidResult.stride_u);

	imgcpy(pV, (u_int8_t*)m_xvidResult.image_v,
		m_pConfig->m_videoWidth / 2, 
		m_pConfig->m_videoHeight / 2,
		m_xvidResult.stride_u);
#endif
	return true;
}

void CXvidVideoEncoder::Stop()
{
  CHECK_AND_FREE(m_vopBuffer);
  xvid_encore(m_xvidHandle, XVID_ENC_DESTROY, NULL, NULL);
  m_xvidHandle = NULL;
}

