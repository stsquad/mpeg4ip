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

#include "mp4live.h"
#include "media_node.h"
#include "audio_encoder.h"

bool CMediaSource::AddSink(CMediaSink* pSink) 
{
	bool rc = false;

	if (SDL_LockMutex(m_pSinksMutex) == -1) {
		debug_message("AddSink LockMutex error");
		return rc;
	}
	for (int i = 0; i < MAX_SINKS; i++) {
		if (m_sinks[i] == NULL) {
			m_sinks[i] = pSink;
			rc = true;
			break;
		}
	}
	if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
		debug_message("UnlockMutex error");
	}
	return rc;
}

void CMediaSource::RemoveSink(CMediaSink* pSink) 
{
	if (SDL_LockMutex(m_pSinksMutex) == -1) {
		debug_message("RemoveSink LockMutex error");
		return;
	}
	for (int i = 0; i < MAX_SINKS; i++) {
		if (m_sinks[i] == pSink) {
			int j;
			for (j = i; j < MAX_SINKS - 1; j++) {
				m_sinks[j] = m_sinks[j+1];
			}
			m_sinks[j] = NULL;
			break;
		}
	}
	if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
		debug_message("UnlockMutex error");
	}
}

void CMediaSource::RemoveAllSinks(void) 
{
	if (SDL_LockMutex(m_pSinksMutex) == -1) {
		debug_message("RemoveAllSinks LockMutex error");
		return;
	}
	for (int i = 0; i < MAX_SINKS; i++) {
		if (m_sinks[i] == NULL) {
			break;
		}
		m_sinks[i] = NULL;
	}
	if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
		debug_message("UnlockMutex error");
	}
}

void CMediaSource::ForwardEncodedAudioFrames(
	CAudioEncoder* encoder,
	Timestamp baseTimestamp,
	u_int32_t* pNumSamples,
	u_int32_t* pNumFrames)
{
	u_int8_t* pFrame;
	u_int32_t frameLength;
	u_int32_t frameNumSamples;

	while (encoder->GetEncodedSamples(
	  &pFrame, &frameLength, &frameNumSamples)) {

		// sanity check
		if (pFrame == NULL || frameLength == 0) {
			break;
		}

		// forward the encoded frame to sinks
		CMediaFrame* pMediaFrame =
			new CMediaFrame(
				encoder->GetFrameType(),
				pFrame, 
				frameLength,
				baseTimestamp + SamplesToTicks(frameNumSamples),
				frameNumSamples,
				m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));
		ForwardFrame(pMediaFrame);
		delete pMediaFrame;

		(*pNumSamples) += frameNumSamples;
		(*pNumFrames)++;
	}
}
