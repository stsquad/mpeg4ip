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

#ifndef __VIDEO_XVID10_H__
#define __VIDEO_XVID10_H__

#include "video_encoder.h"
#include <xvid.h>
class CXvid10VideoEncoder : public CVideoEncoder {
public:
	CXvid10VideoEncoder(CVideoProfile *vp, 
			    uint16_t mtu,
			    CVideoEncoder *next, 
			    bool realTime = true);

	MediaType GetFrameType(void) { return MPEG4VIDEOFRAME; };
	bool Init(void);
 protected:

	bool EncodeImage(
		const u_int8_t* pY, const u_int8_t* pU, const u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame, Duration elapsed,
		Timestamp srcFrameTimestamp);

	bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength,
		Timestamp *dts, Timestamp *pts);

	bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV);

 public:
	bool CanGetEsConfig(void) { return true; };
	bool GetEsConfig(uint8_t **ppEsConfig,
			 uint32_t *pEsConfigLen);
 protected:

	void StopEncoder(void);
	void*				m_xvidHandle;
	u_int8_t*			m_vopBuffer;
	u_int32_t			m_vopBufferLength;
	Timestamp               m_srcFrameTimestamp;
	xvid_enc_stats_t		m_xvidResult;
	uint8_t m_video_quality; // 0 to 6
	bool m_use_gmc;
	bool m_use_qpel;
	bool m_use_interlacing;
	bool m_use_lumimask_plugin;
	bool m_use_par;
	int m_par;
	int m_par_width, m_par_height;
	//#define WRITE_RAW
#ifdef WRITE_RAW
	FILE *m_outfile;
#endif
};

void AddXvid10ConfigVariables(CVideoProfile *pConfig);
EXTERN_TABLE_F(xvid_gui_options);
#endif /* __VIDEO_XVID10_H__ */

