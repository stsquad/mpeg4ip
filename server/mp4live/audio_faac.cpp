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
}

bool CFaacAudioEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
	m_pConfig = pConfig;

	return false;
}

bool CFaacAudioEncoder::EncodeSamples(
	u_int16_t* pBuffer, u_int32_t bufferLength)
{
	return false;
}

bool CFaacAudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength)
{
	return false;
}

void CFaacAudioEncoder::Stop()
{
}

