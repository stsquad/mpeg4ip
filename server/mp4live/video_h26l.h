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

#ifndef __VIDEO_H26L_H__
#define __VIDEO_H26L_H__

#include "video_encoder.h"

#include <h26l.h>

class CH26LVideoEncoder : public CVideoEncoder {
public:
	CH26LVideoEncoder();

	MediaType GetFrameType(void) { return H26LVIDEOFRAME; };
	bool Init(
		CLiveConfig* pConfig, bool realTime = true);

	bool EncodeImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame,
		Duration elapsed);

	bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength);

	bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV);

	void Stop();

protected:
	u_int8_t*			m_vopBuffer;
	u_int32_t			m_vopBufferLength;
};

#endif /* __VIDEO_H26L_H__ */

