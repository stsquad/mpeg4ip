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
#include "audio_lame.h"
#include <mp4av.h>

CLameAudioEncoder::CLameAudioEncoder()
{
	m_mp3FrameBuffer = NULL;
}

bool CLameAudioEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
	m_pConfig = pConfig;

	lame_init(&m_lameParams);

	m_lameParams.num_channels = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);
	m_lameParams.in_samplerate = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
	m_lameParams.brate = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE);
	m_lameParams.mode = 0;
	m_lameParams.quality = 2;
	m_lameParams.silent = 1;
	m_lameParams.gtkflag = 0;

	lame_init_params(&m_lameParams);

	if (m_lameParams.in_samplerate != m_lameParams.out_samplerate) {
		error_message("warning: lame audio sample rate mismatch");	

		m_pConfig->SetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE, 
			m_lameParams.out_samplerate);
	}

	m_samplesPerFrame = MP4AV_Mp3GetSamplingWindow(
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

	m_mp3FrameMaxSize = (u_int)(1.25 * m_samplesPerFrame) + 7200;

	m_mp3FrameBufferSize = 2 * m_mp3FrameMaxSize;

	m_mp3FrameBufferLength = 0;

	m_mp3FrameBuffer = (u_int8_t*)malloc(m_mp3FrameBufferSize);

	if (!m_mp3FrameBuffer) {
		return false;
	}

	return true;
}

u_int32_t CLameAudioEncoder::GetSamplesPerFrame()
{
	return m_samplesPerFrame;
}

bool CLameAudioEncoder::EncodeSamples(
	u_int16_t* pBuffer, 
	u_int32_t bufferLength)
{
	if (m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) == 2) {
		bool rc;
		u_int16_t* leftBuffer = NULL;
		u_int16_t* rightBuffer = NULL;

		DeinterleaveStereoSamples(
			pBuffer, 
			bufferLength / 2, 
			&leftBuffer, 
			&rightBuffer);

		rc = EncodeSamples(leftBuffer, rightBuffer, bufferLength / 2);
		
		free(leftBuffer);
		free(rightBuffer);

		return rc; 
	} 

	// else channels == 1
	return EncodeSamples(pBuffer, NULL, bufferLength);
}

bool CLameAudioEncoder::EncodeSamples(
	u_int16_t* pLeftBuffer, 
	u_int16_t* pRightBuffer, 
	u_int32_t bufferLength)
{
	u_int32_t mp3DataLength = 0;

	if (pLeftBuffer) {
		// call lame encoder
		mp3DataLength = lame_encode_buffer(
			&m_lameParams,
			(short*)pLeftBuffer, 
			(short*)pRightBuffer, 
			m_samplesPerFrame,
			(char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			m_mp3FrameBufferSize - m_mp3FrameBufferLength);

	} else { // pLeftBuffer == NULL, signal to stop encoding
		mp3DataLength = lame_encode_finish(
			&m_lameParams,
			(char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			m_mp3FrameBufferSize - m_mp3FrameBufferLength);
	}

	m_mp3FrameBufferLength += mp3DataLength;

	return (mp3DataLength > 0);
}

bool CLameAudioEncoder::GetEncodedSamples(
	u_int8_t** ppBuffer, 
	u_int32_t* pBufferLength,
	u_int32_t* pNumSamples)
{
	u_int8_t* mp3Frame;
	u_int32_t mp3FrameLength;

	if (!MP4AV_Mp3GetNextFrame(m_mp3FrameBuffer, m_mp3FrameBufferLength, 
	  &mp3Frame, &mp3FrameLength)) {
		return false;
	}

	// check if we have all the bytes for the MP3 frame
	if (mp3FrameLength > m_mp3FrameBufferLength) {
		return false;
	}

	// need a buffer for this MP3 frame
	*ppBuffer = (u_int8_t*)malloc(mp3FrameLength);
	if (*ppBuffer == NULL) {
		return false;
	}

	// copy the MP3 frame
	memcpy(*ppBuffer, mp3Frame, mp3FrameLength);
	*pBufferLength = mp3FrameLength;

	// shift what remains in the buffer down
	memcpy(m_mp3FrameBuffer, 
		mp3Frame + mp3FrameLength, 
		m_mp3FrameBufferLength - mp3FrameLength);

	m_mp3FrameBufferLength -= mp3FrameLength;

	*pNumSamples = m_samplesPerFrame;

	return true;
}

void CLameAudioEncoder::Stop()
{
	free(m_mp3FrameBuffer);
	m_mp3FrameBuffer = NULL;
}

