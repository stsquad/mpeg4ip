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
extern "C" {
#ifdef HAVE_FFMPEG_INSTALLED
#include <ffmpeg/avcodec.h>
#else
#include <avcodec.h>
#endif
}
typedef struct pts_queue_t {
  pts_queue_t *next;
  uint8_t *frameBuffer;
  uint32_t frameBufferLen;
  bool needs_pts;
  Timestamp encodeTime;
} pts_queue_t;
  
class CFfmpegVideoEncoder : public CVideoEncoder {
 public:
	CFfmpegVideoEncoder(CVideoProfile *vp, 
			    uint16_t mtu,
			    CVideoEncoder *next, 
			    bool realTime = true);

	MediaType GetFrameType(void) { return m_media_frame;}
	bool Init(void);
	bool CanGetEsConfig (void) { return true; };
	bool GetEsConfig(uint8_t **ppEsConfig, uint32_t *pEsConfigLen);
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

	media_free_f GetMediaFreeFunction(void);
	
	void StopEncoder(void);
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

	int m_count, m_key_frame_count;
	bool m_usingBFrames;
	uint m_BFrameCount;
	pts_queue_t *m_pts_queue, *m_pts_queue_end;
};

void AddFfmpegConfigVariables(CVideoProfile *pConfig);
EXTERN_TABLE_F(ffmpeg_mpeg4_gui_options);

#endif /* __VIDEO_FFMPEG_H__ */

