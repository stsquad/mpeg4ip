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

#ifndef __AUDIO_FAAC_H__
#define __AUDIO_FAAC_H__

#include "audio_encoder.h"
#include <faac.h>

class CFaacAudioEncoder : public CAudioEncoder {
public:
	CFaacAudioEncoder();

	bool Init(
		CLiveConfig* pConfig, bool realTime = true);

	bool EncodeSamples(
		u_int16_t* pBuffer, u_int32_t bufferLength);

	bool GetEncodedSamples(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength);

	void Stop();

protected:
	faacEncHandle			m_faacHandle;
	faacEncConfigurationPtr	m_faacConfig;

	u_int8_t*				m_aacFrameBuffer;
	u_int32_t				m_aacFrameBufferLength;
	u_int32_t				m_aacFrameMaxSize;
};

#endif /* __AUDIO_FAAC_H__ */

