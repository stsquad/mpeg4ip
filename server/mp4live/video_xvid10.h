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
 */

#ifndef __VIDEO_XVID10_H__
#define __VIDEO_XVID10_H__

#include "video_encoder.h"
#include <xvid.h>
class CXvid10VideoEncoder : public CVideoEncoder {
public:
	CXvid10VideoEncoder();

	MediaType GetFrameType(void) { return MPEG4VIDEOFRAME; };

	bool Init(
		CLiveConfig* pConfig, bool realTime = true);

	bool EncodeImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame, Duration elapsed,
		Timestamp srcFrameTimestamp);

	bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength,
		Timestamp *dts, Timestamp *pts);

	bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV);

	void Stop();
	bool CanGetEsConfig(void) { return true; };
	bool GetEsConfig(CLiveConfig *pConfig, uint8_t **ppEsConfig,
			 uint32_t *pEsConfigLen);
protected:
	void*				m_xvidHandle;
	u_int8_t*			m_vopBuffer;
	u_int32_t			m_vopBufferLength;
	Timestamp               m_srcFrameTimestamp;
	xvid_enc_stats_t		m_xvidResult;
	//#define WRITE_RAW
#ifdef WRITE_RAW
	FILE *m_outfile;
#endif
};

#endif /* __VIDEO_XVID10_H__ */

