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

#ifndef __VIDEO_FFMPEG_H__
#define __VIDEO_FFMPEG_H__

#include "video_encoder.h"
#include <avcodec.h>
class CTimestampPush {
 public:
  CTimestampPush(uint max) {
    m_in = 0;
    m_out = 0;
    m_max = max;
    m_stack = (Timestamp *)malloc(max * sizeof(Timestamp));
  };
  ~CTimestampPush() {
    free(m_stack);
  };
  void Push (Timestamp t) {
    m_stack[m_in] = t;
    m_in++;
    if (m_in >= m_max) m_in = 0;
  };
  Timestamp Pop (void) {
    Timestamp ret;
    ret = m_stack[m_out];
    m_out++;
    if (m_out >= m_max) m_out = 0;
    return ret;
  };
 private:
  Timestamp *m_stack;
  uint m_in, m_out, m_max;
};

class CFfmpegVideoEncoder : public CVideoEncoder {
public:
	CFfmpegVideoEncoder();

	MediaType GetFrameType(void) { return m_media_frame;}
	bool Init(
		CLiveConfig* pConfig, bool realTime = true);

	bool EncodeImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame,
		Duration elapsedDuration,
		Timestamp srcFrameTimestamp);

	bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength,
		Timestamp *dts, Timestamp *pts);

	bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV);

	void Stop();
	media_free_f GetMediaFreeFunction(void);

protected:
//#define OUTPUT_RAW
#ifdef OUTPUT_RAW
	FILE *m_outfile;
#endif
	MediaType m_media_frame;
	AVCodec *m_codec;
	AVCodecContext		*m_avctx;
	AVFrame *m_picture;
	u_int8_t*			m_vopBuffer;
	u_int32_t			m_vopBufferLength;
	u_int8_t*  m_YUV;
	CTimestampPush *m_push;
	Duration m_frame_time;
};

#endif /* __VIDEO_FFMPEG_H__ */

