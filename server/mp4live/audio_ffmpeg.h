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
 *		Bill May   wmay@cisco.com
 */

#ifndef __AUDIO_FFMPEG_H__
#define __AUDIO_FFMPEG_H__

#include "audio_encoder.h"
#include <avcodec.h>
#include <sdp.h>

media_desc_t *ffmpeg_create_audio_sdp(CLiveConfig *pConfig,
				    bool *mpeg4,
				    bool *isma_compliant,
				    uint8_t *audioProfile,
				    uint8_t **audioConfig,
				    uint32_t *audioConfigLen);
MediaType ffmpeg_mp4_fileinfo(CLiveConfig *pConfig,
			    bool *mpeg4,
			    bool *isma_compliant,
			    uint8_t *audioProfile,
			    uint8_t **audioConfig,
			    uint32_t *audioConfigLen,
			    uint8_t *mp4AudioType);

bool ffmpeg_get_audio_rtp_info (CLiveConfig *pConfig,
			      MediaType *audioFrameType,
			      uint32_t *audioTimeScale,
			      uint8_t *audioPayloadNumber,
			      uint8_t *audioPayloadBytesPerPacket,
			      uint8_t *audioPayloadBytesPerFrame,
			      uint8_t *audioQueueMaxCount,
			      audio_set_rtp_header_f *audio_set_header,
			      audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			      void **ud);

class CFfmpegAudioEncoder : public CAudioEncoder {
public:
	CFfmpegAudioEncoder();

	bool Init(
		CLiveConfig* pConfig, 
		bool realTime = true);

	MediaType GetFrameType(void) {
		return MP3AUDIOFRAME;
	}

	u_int32_t GetSamplesPerFrame();

	bool EncodeSamples(
		int16_t* pSamples, 
		u_int32_t numSamplesPerChannel,
		u_int8_t numChannels);

	bool GetEncodedFrame(
		u_int8_t** ppBuffer, 
		u_int32_t* pBufferLength,
		u_int32_t* pNumSamplesPerChannel);

	void Stop();

protected:
	AVCodec *m_codec;
	AVCodecContext *m_avctx;
	AVFrame *m_frame;
	u_int32_t			m_samplesPerFrame;
	u_int8_t*			m_mp3FrameBuffer;
	u_int32_t			m_mp3FrameBufferLength;
	u_int32_t			m_mp3FrameBufferSize;
	u_int32_t			m_mp3FrameMaxSize;
};

extern audio_encoder_table_t ffmpeg_audio_encoder_table;

#endif /* __AUDIO_FFMPEG_H__ */

