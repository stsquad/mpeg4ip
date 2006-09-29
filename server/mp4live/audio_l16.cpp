/*
 * The following code is subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this code
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
#include "audio_l16.h"
#include "mpeg4ip_byteswap.h"

static uint32_t *l16_bitrate_for_samplerate (uint32_t samplerate, 
					      uint8_t chans, 
					      uint32_t *ret_size)
{
  uint32_t *ret = MALLOC_STRUCTURE(uint32_t);

  *ret = samplerate * chans * 16;
  *ret_size = 1;
  return ret;
}

audio_encoder_table_t l16_audio_encoder_table = {
  "L16",
  AUDIO_ENCODER_L16,
  AUDIO_ENCODING_L16,
  allSampleRateTable,
  NUM_ELEMENTS_IN_ARRAY(allSampleRateTable),
  l16_bitrate_for_samplerate,
  2, 
  NULL
};


MediaType l16_mp4_fileinfo (CAudioProfile *pConfig,
			     bool *mpeg4,
			     bool *isma_compliant,
			     uint8_t *audioProfile,
			     uint8_t **audioConfig,
			     uint32_t *audioConfigLen,
			     uint8_t *mp4AudioType)
{
  *mpeg4 = false;
  *isma_compliant = false;
  *audioProfile = 0;
  *audioConfig = NULL;
  *audioConfigLen = 0;
  if (mp4AudioType != NULL) {
    *mp4AudioType = 0;
  }
  if (mp4AudioType != NULL) {
    *mp4AudioType = MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE;
  }
  return NETPCMAUDIOFRAME;
}

media_desc_t *l16_create_audio_sdp (CAudioProfile *pConfig,
				     bool *mpeg4,
				     bool *isma_compliant,
				     uint8_t *audioProfile,
				     uint8_t **audioConfig,
				     uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;
  MediaType type;

  type = l16_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile,
			     audioConfig, audioConfigLen, NULL);

  sdpMediaAudio = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaAudio, 0, sizeof(*sdpMediaAudio));

  sdpMediaAudioFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaAudioFormat, 0, sizeof(*sdpMediaAudioFormat));

  sdpMediaAudio->fmt_list = sdpMediaAudioFormat;
  sdpMediaAudioFormat->media = sdpMediaAudio;
  if (pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS) == 2) {
    sdpMediaAudioFormat->rtpmap_encode_param = 2;
  } else {
    sdpMediaAudioFormat->rtpmap_encode_param = 1;
  }
  sdpMediaAudioFormat->rtpmap_name = strdup("L16");
  sdpMediaAudioFormat->rtpmap_clock_rate = pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
  sdpMediaAudioFormat->fmt = strdup("97");

  return sdpMediaAudio;
}
bool l16_get_audio_rtp_info (CAudioProfile *pConfig,
			      MediaType *audioFrameType,
			      uint32_t *audioTimeScale,
			      uint8_t *audioPayloadNumber,
			      uint8_t *audioPayloadBytesPerPacket,
			      uint8_t *audioPayloadBytesPerFrame,
			      uint8_t *audioQueueMaxCount,
			      audio_set_rtp_payload_f *audio_set_rtp_payload,
			      audio_set_rtp_header_f *audio_set_header,
			      audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			      void **ud)
{
  *audioFrameType = NETPCMAUDIOFRAME;
  *audioPayloadNumber = 97;
  *audioTimeScale = pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
  *audioPayloadBytesPerPacket = 0;
  *audioPayloadBytesPerFrame = 0;
  *audioQueueMaxCount = 1;
  *audio_set_header = NULL;
  *audio_set_jumbo = NULL;
  *ud = NULL;
  return true;
}
  
CL16AudioEncoder::CL16AudioEncoder(CAudioProfile *ap,
				     CAudioEncoder *next, 
				     u_int8_t srcChannels,
				     u_int32_t srcSampleRate,
				     uint16_t mtu,
				     bool realTime) :
  CAudioEncoder(ap, next, srcChannels, srcSampleRate, mtu, realTime)
{
	m_pFrameBuffer = NULL;
	m_frameBufferLength = 0;
}

bool CL16AudioEncoder::Init (void)
{
  m_samplesPerFrame = m_pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE) * 2 * m_pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS);
  m_samplesPerFrame /= 100;

  m_samplesPerFrame = MIN(m_samplesPerFrame, m_mtu);

  Profile()->SetIntegerValue(CFG_AUDIO_BIT_RATE,
			     m_pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS)
			     * m_pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE) * 16);
  Initialize();
  return true;
}

u_int32_t CL16AudioEncoder::GetSamplesPerFrame()
{
	return m_samplesPerFrame;
}

bool CL16AudioEncoder::EncodeSamples(
	int16_t* pSamples, 
	u_int32_t numSamplesPerChannel,
	u_int8_t numChannels)
{
  // check for signal to end encoding
  if (pSamples == NULL) {
    // unlike lame, L16 doesn't need to finish up anything
    return false;
  }

  // get an output buffer setup
  CHECK_AND_FREE(m_pFrameBuffer);

  m_frameBufferLength = numSamplesPerChannel
    * m_pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS) * sizeof(int16_t);
  m_pFrameBuffer = (u_int16_t*)Malloc(m_frameBufferLength);
  
#ifdef WORDS_BIGENDIAN
  memcpy(m_pFrameBuffer, pSameples, m_frameBufferLength);
#else
  for (uint32_t ix = 0; ix < numSamplesPerChannel * m_audioDstChannels; ix++) {
    m_pFrameBuffer[ix] = B2N_16(pSamples[ix]);
  }
#endif
  return true;
}

bool CL16AudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, 
	u_int32_t* pBufferLength,
	u_int32_t* pNumSamplesPerChannel)
{
	*ppBuffer = (uint8_t *)m_pFrameBuffer;
	*pBufferLength = m_frameBufferLength;
	*pNumSamplesPerChannel = DstBytesToSamples(m_frameBufferLength);

	m_pFrameBuffer = NULL;
	m_frameBufferLength = 0;

	return true;
}

void CL16AudioEncoder::StopEncoder (void)
{
	free(m_pFrameBuffer);
	m_pFrameBuffer = NULL;
}


