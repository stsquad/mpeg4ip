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
#include "video_ffmpeg.h"

CFfmpegVideoEncoder::CFfmpegVideoEncoder()
{
	memset(&m_avctx, 0, sizeof(m_avctx));
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;
}

bool CFfmpegVideoEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
	m_pConfig = pConfig;

	// use ffmpeg "divx" aka mpeg4 encoder
	m_avctx.frame_number = 0;
	m_avctx.width = m_pConfig->m_videoWidth;
	m_avctx.height = m_pConfig->m_videoHeight;
	m_avctx.rate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE);
	m_avctx.bit_rate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000;
	m_avctx.gop_size = m_avctx.rate * 2;
	m_avctx.want_key_frame = 0;
	m_avctx.flags = 0;
	m_avctx.codec = &divx_encoder;
	m_avctx.priv_data = malloc(m_avctx.codec->priv_data_size);
	memset(m_avctx.priv_data, 0, m_avctx.codec->priv_data_size);

	divx_encoder.init(&m_avctx);

	return true;
}

bool CFfmpegVideoEncoder::EncodeImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV, bool wantKeyFrame)
{
	m_vopBuffer = (u_int8_t*)malloc(m_pConfig->m_videoMaxVopSize);
	if (m_vopBuffer == NULL) {
		return false;
	}

	u_int8_t* yuvPlanes[3];
	yuvPlanes[0] = pY;
	yuvPlanes[1] = pU;
	yuvPlanes[2] = pV;

	m_avctx.want_key_frame = wantKeyFrame;

	m_vopBufferLength = divx_encoder.encode(
		&m_avctx, m_vopBuffer, m_pConfig->m_videoMaxVopSize, yuvPlanes);

	m_avctx.frame_number++;

	return true;
}

bool CFfmpegVideoEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength)
{
	*ppBuffer = m_vopBuffer;
	*pBufferLength = m_vopBufferLength;

	m_vopBuffer = NULL;
	m_vopBufferLength = 0;

	return true;
}

bool CFfmpegVideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{
	memcpy(pY, ((MpegEncContext*)m_avctx.priv_data)->current_picture[0],
		m_pConfig->m_ySize);
	memcpy(pU, ((MpegEncContext*)m_avctx.priv_data)->current_picture[1],
		m_pConfig->m_uvSize);
	memcpy(pV, ((MpegEncContext*)m_avctx.priv_data)->current_picture[2],
		m_pConfig->m_uvSize);
	
	return true;
}

void CFfmpegVideoEncoder::Stop()
{
	divx_encoder.close(&m_avctx);
	free(m_avctx.priv_data);
	m_avctx.priv_data = NULL;
}

