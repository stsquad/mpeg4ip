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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *	Bill May		wmay@cisco.com	
 *	Peter Maersk-Moller	peter@maersk-moller.net
 */

#include "mp4live.h"
#include "audio_encoder.h"
#include "mp4av.h"
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_RESAMPLER 1
//#define DEBUG_AUDIO_SYNC 1
/*
 * This looks like a fairly bogus set of routines; however, the
 * code makes it really easy to add your own codecs here, include
 * the mp4live library, write your own main, and go.
 * Just add the codecs you want before the call to the base routines
 */
const uint32_t allSampleRateTable[13] = {
  7350, 8000, 11025, 12000, 16000, 22050, 
  24000, 32000, 44100, 48000, 64000, 88200, 96000
};
const uint32_t allSampleRateTableSize = NUM_ELEMENTS_IN_ARRAY(allSampleRateTable);

void AudioProfileCheck(CAudioProfile *ap);

CAudioEncoder* AudioEncoderCreate(CAudioProfile *ap, 
				  CAudioEncoder *next, 
				  u_int8_t srcChannels,
				  u_int32_t srcSampleRate,
				  uint16_t mtu,
				  bool realTime)
{
  // add codecs here
  return AudioEncoderBaseCreate(ap, next, 
				srcChannels, srcSampleRate, mtu, realTime);
}

MediaType get_audio_mp4_fileinfo (CAudioProfile *pConfig,
				  bool *mpeg4,
				  bool *isma_compliant,
				  uint8_t *audioProfile,
				  uint8_t **audioConfig,
				  uint32_t *audioConfigLen,
				  uint8_t *mp4_audio_type)
{
  return get_base_audio_mp4_fileinfo(pConfig, mpeg4, isma_compliant, 
				     audioProfile, audioConfig, 
				     audioConfigLen, mp4_audio_type);
}

media_desc_t *create_audio_sdp (CAudioProfile *pConfig,
				bool *mpeg4,
				bool *isma_compliant,
				bool *audio_is_3gp,
				uint8_t *audioProfile,
				uint8_t **audioConfig,
				uint32_t *audioConfigLen)
{
  return create_base_audio_sdp(pConfig, mpeg4, isma_compliant,
			       audio_is_3gp,
			       audioProfile, audioConfig, audioConfigLen);
}
				
void create_mp4_audio_hint_track (CAudioProfile *pConfig, 
				  MP4FileHandle mp4file,
				  MP4TrackId trackId,
				  uint16_t mtu)
{
  create_base_mp4_audio_hint_track(pConfig, mp4file, trackId, mtu);
}

bool get_audio_rtp_info (CAudioProfile *pConfig,
			 MediaType *audioFrameType,
			 uint32_t *audioTimeScale,
			 uint8_t *audioPayloadNumber,
			 uint8_t *audioPayloadBytesPerPacket,
			 uint8_t *audioPayloadBytesPerFrame,
			 uint8_t *audioQueueMaxCount,
			 uint8_t *audioiovMaxCount,
			 audio_queue_frame_f *audio_queue_frame,
			 audio_set_rtp_payload_f *audio_set_rtp_payload,
			 audio_set_rtp_header_f *audio_set_header,
			 audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			 void **ud)
{
  return get_base_audio_rtp_info(pConfig, 
				 audioFrameType, 
				 audioTimeScale,
				 audioPayloadNumber, 
				 audioPayloadBytesPerPacket,
				 audioPayloadBytesPerFrame, 
				 audioQueueMaxCount,
				 audioiovMaxCount,
			 	 audio_queue_frame,
			         audio_set_rtp_payload,
				 audio_set_header, 
				 audio_set_jumbo, 
				 ud);
}

