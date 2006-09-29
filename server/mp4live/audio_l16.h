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

#ifndef __AUDIO_L16_H__
#define __AUDIO_L16_H__

#include "audio_encoder.h"

class CL16AudioEncoder : public CAudioEncoder {
public:
	CL16AudioEncoder(CAudioProfile *ap, 
			  CAudioEncoder *next,
			  u_int8_t srcChannels,
			  u_int32_t srcSampleRate,
			  uint16_t mtu,
			  bool realTime = true);

	bool Init(void);


	MediaType GetFrameType() {
	  return NETPCMAUDIOFRAME;
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

protected:
	void StopEncoder(void);
	u_int32_t			m_samplesPerFrame;
	u_int16_t*			m_pFrameBuffer;
	u_int32_t			m_frameBufferLength;
};

extern audio_encoder_table_t l16_audio_encoder_table;

media_desc_t *l16_create_audio_sdp(CAudioProfile *pConfig,
				    bool *mpeg4,
				    bool *isma_compliant,
				    uint8_t *audioProfile,
				    uint8_t **audioConfig,
				    uint32_t *audioConfigLen);
MediaType l16_mp4_fileinfo(CAudioProfile *pConfig,
			    bool *mpeg4,
			    bool *isma_compliant,
			    uint8_t *audioProfile,
			    uint8_t **audioConfig,
			    uint32_t *audioConfigLen,
			    uint8_t *mp4AudioType);

bool l16_get_audio_rtp_info(CAudioProfile *pConfig,
			       MediaType *audioFrameType,
			       uint32_t *audioTimeScale,
			       uint8_t *audioPayloadNumber,
			       uint8_t *audioPayloadBytesPerPacket,
			       uint8_t *audioPayloadBytesPerFrame,
			       uint8_t *audioQueueMaxCount,
			       audio_set_rtp_payload_f *audio_set_rtp_payload,
			       audio_set_rtp_header_f *audio_set_header,
			       audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			       void **ud);

#endif /* __AUDIO_L16_H__ */

