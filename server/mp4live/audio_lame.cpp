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

CLameAudioEncoder::CLameAudioEncoder()
{
	m_leftBuffer = NULL;
	m_rightBuffer = NULL;
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

	m_pConfig->m_audioEncodedSampleRate = 
		m_lameParams.out_samplerate;

	if (m_pConfig->m_audioEncodedSampleRate > 24000) {
		m_pConfig->m_audioEncodedSamplesPerFrame = 
			MP3_MPEG1_SAMPLES_PER_FRAME;
	} else {
		m_pConfig->m_audioEncodedSamplesPerFrame = 
			MP3_MPEG2_SAMPLES_PER_FRAME;
	}

	// sanity check
	if (m_lameParams.in_samplerate < m_lameParams.out_samplerate) {
		return false;
	}

	u_int16_t rawSamplesPerFrame = (u_int16_t)
		((((float)m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE) 
		/ (float)m_pConfig->m_audioEncodedSampleRate)
		* m_pConfig->m_audioEncodedSamplesPerFrame) 
		+ 0.5);

	u_int32_t rawFrameSize = rawSamplesPerFrame
		* m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) 
		* sizeof(u_int16_t);

	m_mp3FrameMaxSize = 
		(u_int)(1.25 * m_pConfig->m_audioEncodedSamplesPerFrame) + 7200;

	m_mp3FrameBufferSize = 2 * m_mp3FrameMaxSize;

	m_mp3FrameBufferLength = 0;

	m_mp3FrameBuffer = (u_int8_t*)malloc(m_mp3FrameBufferSize);

	if (!m_mp3FrameBuffer) {
		return false;
	}

	m_leftBuffer = (u_int16_t*)malloc(rawFrameSize / 2);
	m_rightBuffer = (u_int16_t*)malloc(rawFrameSize / 2);

	if (!m_leftBuffer || !m_rightBuffer) {
		free(m_mp3FrameBuffer);
		free(m_leftBuffer);
		free(m_rightBuffer);
		return false;
	}

	if (m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) == 1) {
		memset(m_rightBuffer, 0, rawFrameSize / 2);
	}

	return true;
}

bool CLameAudioEncoder::EncodeSamples(
	u_int16_t* pBuffer, u_int32_t bufferLength)
{
	u_int32_t mp3DataLength = 0;

	if (pBuffer) {
		u_int16_t* leftBuffer;

		// de-interleave input if doing stereo
		if (m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) == 2) {
			// de-interleave raw frame buffer
			u_int16_t* s = pBuffer;
			for (int i = 0; i < m_pConfig->m_audioEncodedSamplesPerFrame; i++) {
				m_leftBuffer[i] = *s++;
				m_rightBuffer[i] = *s++;
			}
			leftBuffer = m_leftBuffer;
		} else {
			leftBuffer = pBuffer;
		}

		// call lame encoder
		mp3DataLength = lame_encode_buffer(
			&m_lameParams,
			(short*)leftBuffer, (short*)m_rightBuffer, 
			m_pConfig->m_audioEncodedSamplesPerFrame,
			(char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			m_mp3FrameBufferSize - m_mp3FrameBufferLength);

	} else { // pBuffer == NULL, signal to stop encoding
		mp3DataLength = lame_encode_finish(
			&m_lameParams,
			(char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			m_mp3FrameBufferSize - m_mp3FrameBufferLength);
	}

	if (mp3DataLength == 0) {
		return false;
	}

	m_mp3FrameBufferLength += mp3DataLength;
	return true;
}

bool CLameAudioEncoder::GetEncodedSamples(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength)
{
	u_int8_t* mp3Frame;
	u_int32_t mp3FrameLength;

	if (!Mp3FindNextFrame(m_mp3FrameBuffer, m_mp3FrameBufferLength, 
	  &mp3Frame, &mp3FrameLength, false)) {
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

	return true;
}

void CLameAudioEncoder::Stop()
{
	free(m_leftBuffer);
	m_leftBuffer = NULL;

	free(m_rightBuffer);
	m_rightBuffer = NULL;

	free(m_mp3FrameBuffer);
	m_mp3FrameBuffer = NULL;
}

