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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include "media_codec.h"
#include "media_frame.h"

class CAudioEncoder : public CMediaCodec {
public:
	CAudioEncoder() { };

	virtual MediaType GetFrameType() = NULL;

	virtual u_int32_t GetSamplesPerFrame() = NULL;

	virtual bool EncodeSamples(
		u_int16_t* pBuffer, 
		u_int32_t bufferLength,
		u_int8_t numChannels) = NULL;

	virtual bool EncodeSamples(
		u_int16_t* pLeftBuffer, 
		u_int16_t* pRightBuffer, 
		u_int32_t bufferLength) = NULL;

	virtual bool GetEncodedSamples(
		u_int8_t** ppBuffer, 
		u_int32_t* pBufferLength,
		u_int32_t* pNumSamples) = NULL;


	// utility routines

	static bool InterleaveStereoSamples(
		u_int16_t* pLeftBuffer, 
		u_int16_t* pRightBuffer, 
		u_int32_t srcNumSamples,
		u_int16_t** ppDstBuffer);

	static bool DeinterleaveStereoSamples(
		u_int16_t* pSrcBuffer, 
		u_int32_t srcNumSamples,
		u_int16_t** ppLeftBuffer, 
		u_int16_t** ppRightBuffer); 
};

CAudioEncoder* AudioEncoderCreate(const char* encoderName);

#endif /* __AUDIO_ENCODER_H__ */

