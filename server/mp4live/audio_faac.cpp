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
#include "audio_faac.h"

CFaacAudioEncoder::CFaacAudioEncoder()
{
	m_faacHandle = NULL;
	m_aacFrameBuffer = NULL;
	m_aacFrameBufferLength = 0;
	m_aacFrameMaxSize = 0;
}

bool CFaacAudioEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
	m_pConfig = pConfig;

	m_pConfig->m_audioEncodedSampleRate =
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);

	u_int32_t inputSamples;

	m_faacHandle = faacEncOpen(
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE),
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS),
		(unsigned long*)&inputSamples,
		(unsigned long*)&m_aacFrameMaxSize);

	m_pConfig->m_audioEncodedSamplesPerFrame = 
		inputSamples / m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);

	if (m_faacHandle == NULL) {
		return false;
	}

	m_faacConfig = faacEncGetCurrentConfiguration(m_faacHandle);

	m_faacConfig->mpegVersion = MPEG4;
	m_faacConfig->aacObjectType = LOW;
	m_faacConfig->useAdts = false;

	m_faacConfig->bitRate = 
		(m_pConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE) * 1000)
		/ m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);

	faacEncSetConfiguration(m_faacHandle, m_faacConfig);

	return true;
}

bool CFaacAudioEncoder::EncodeSamples(
	u_int16_t* pBuffer, u_int32_t bufferLength)
{
	int rc = 0;

	// just in case, should be NULL
	free(m_aacFrameBuffer);

	m_aacFrameBuffer = (u_int8_t*)malloc(m_aacFrameMaxSize);

	if (m_aacFrameBuffer == NULL) {
		return false;
	}

	rc = faacEncEncode(
		m_faacHandle,
		(short*)pBuffer,
		bufferLength / 2,
		m_aacFrameBuffer,
		m_aacFrameMaxSize);

	if (rc < 0) {
		return false;
	}

	m_aacFrameBufferLength = rc;

	return true;
}

bool CFaacAudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength)
{
	*ppBuffer = m_aacFrameBuffer;
	*pBufferLength = m_aacFrameBufferLength;

	m_aacFrameBuffer = NULL;
	m_aacFrameBufferLength = 0;

	return true;
}

void CFaacAudioEncoder::Stop()
{
	faacEncClose(m_faacHandle);
	m_faacHandle = NULL;
}

