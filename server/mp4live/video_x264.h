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

#ifndef __VIDEO_X264_H__
#define __VIDEO_X264_H__ 1

#include "video_encoder.h"

#ifdef HAVE_X264
#ifdef __cplusplus
extern "C" {
#endif
#include <x264.h>
#ifdef __cplusplus
}
#endif

class CX264VideoEncoder : public CVideoEncoder {
 public:
	CX264VideoEncoder(CVideoProfile *vp, 
			  uint16_t mtu,
			  CVideoEncoder *next, 
			  bool realTime = true);

	MediaType GetFrameType(void) { return m_media_frame;}
	bool Init(void);
	bool CanGetEsConfig(void) { return true; };
	bool GetEsConfig(uint8_t **ppEsConfig, 
			 uint32_t *pEsConfigLen);

	media_free_f GetMediaFreeFunction(void);
 protected:
	bool EncodeImage(
		const u_int8_t* pY, const u_int8_t* pU, const u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame,
		Duration elapsedDuration,
		Timestamp srcFrameTimestamp);

	bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength,
		Timestamp *dts, Timestamp *pts);

	bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV);


	void StopEncoder(void);
	//#define OUTPUT_RAW
#ifdef OUTPUT_RAW
	FILE *m_outfile;
#endif
	MediaType m_media_frame;
	u_int8_t*			m_vopBuffer;
	u_int32_t			m_vopBufferLength;
	u_int8_t*  m_YUV;
	CTimestampPush *m_push;
	Duration m_frame_time;
	Duration m_pts_add;
	int m_count, m_key_frame_count;
	x264_t *m_h;
	x264_param_t m_param;
	x264_picture_t m_pic_input, m_pic_output;

	h264_nal_buf_t *m_nal_info;
	uint32_t m_nal_num;
	
};

void AddX264ConfigVariables(CVideoProfile *pConfig);
EXTERN_TABLE_F(x264_gui_options);
#endif

#endif /* __VIDEO_X264_H__ */

