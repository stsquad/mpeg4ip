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

#ifndef __AUDIO_LAME_H__
#define __AUDIO_LAME_H__

#include "audio_encoder.h"
#include <lame.h>

class CLameAudioEncoder : public CAudioEncoder {
public:
	CLameAudioEncoder();

	bool Init(
		CLiveConfig* pConfig, 
		bool realTime = true);

	MediaType GetFrameType() {
		return CMediaFrame::Mp3AudioFrame;
	}

	u_int32_t GetSamplesPerFrame();

	bool EncodeSamples(
		u_int16_t* pBuffer, 
		u_int32_t bufferLength,
		u_int8_t numChannels);

	bool EncodeSamples(
		u_int16_t* pLeftBuffer,
		u_int16_t* pRightBuffer, 
		u_int32_t bufferLength);

	bool GetEncodedSamples(
		u_int8_t** ppBuffer, 
		u_int32_t* pBufferLength,
		u_int32_t* pNumSamples);

	void Stop();

protected:
	lame_global_flags	m_lameParams;
	u_int32_t			m_samplesPerFrame;
	u_int8_t*			m_mp3FrameBuffer;
	u_int32_t			m_mp3FrameBufferLength;
	u_int32_t			m_mp3FrameBufferSize;
	u_int32_t			m_mp3FrameMaxSize;
};

#endif /* __AUDIO_LAME_H__ */

