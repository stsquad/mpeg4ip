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
#ifdef HAVE_TWOLAME
#include "audio_twolame.h"
#include <mp4av.h>

GUI_BOOL(gui_mp3use14, CFG_RTP_USE_MP3_PAYLOAD_14, "Transmit MP3 using RFC-2250");
DECLARE_TABLE(twolame_gui_options) = {
  TABLE_GUI(gui_mp3use14),
};
DECLARE_TABLE_FUNC(twolame_gui_options);

static const uint32_t twolame_sample_rates[] = {
  44100, 48000, 32000
};

static const uint twolame_bitrate_table[] = 
  { 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 };

#define NUM_BITRATES (NUM_ELEMENTS_IN_ARRAY(twolame_bitrate_table))

static uint32_t *twolame_bitrate_for_samplerate (uint32_t samplerate, 
						 uint8_t chans,
						 uint32_t *ret_size)
{
  uint iy;

  uint32_t *ret = (uint32_t *)malloc(NUM_BITRATES * sizeof(uint32_t));
  *ret_size = 0;
  twolame_options *twolameParams;

  for (iy = 0; iy < NUM_BITRATES; iy++) {
    twolameParams = twolame_init();
    twolame_set_num_channels(twolameParams, chans);
    twolame_set_in_samplerate(twolameParams, samplerate);
    twolame_set_mode(twolameParams,
		     (chans == 1 ? TWOLAME_MONO : TWOLAME_STEREO));	
    twolame_set_VBR(twolameParams, 0);
    twolame_set_brate(twolameParams, twolame_bitrate_table[iy]);

    if (twolame_init_params(twolameParams) != -1) {
      if (twolame_get_in_samplerate(twolameParams) == twolame_get_out_samplerate(twolameParams)) {
	ret[*ret_size] = twolame_bitrate_table[iy] * 1000;
	*ret_size = *ret_size + 1;
      }
    }
    twolame_close(&twolameParams);
  }
  return ret;
}

audio_encoder_table_t twolame_audio_encoder_table =  {
  "MP1 Layer 2 - twolame",
  AUDIO_ENCODER_TWOLAME,
  AUDIO_ENCODING_MP3,
  twolame_sample_rates,
  NUM_ELEMENTS_IN_ARRAY(twolame_sample_rates),
  twolame_bitrate_for_samplerate,
  2,
  twolame_gui_options_f,
};
    
MediaType twolame_mp4_fileinfo (CAudioProfile *pConfig,
				bool *mpeg4,
				bool *isma_compliant,
				uint8_t *audioProfile,
				uint8_t **audioConfig,
				uint32_t *audioConfigLen,
				uint8_t *mp4AudioType)
{
  *mpeg4 = false; // legal in an mp4 - create an iod
  *isma_compliant = false;
  *audioProfile = 0xfe;
  *audioConfig = NULL;
  *audioConfigLen = 0;
  if (mp4AudioType != NULL) {
    *mp4AudioType = MP4_MPEG1_AUDIO_TYPE;
  }
  return MP3AUDIOFRAME;
}

media_desc_t *twolame_create_audio_sdp (CAudioProfile *pConfig,
					bool *mpeg4,
					bool *isma_compliant,
					uint8_t *audioProfile,
					uint8_t **audioConfig,
					uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;
  
  twolame_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile,
		       audioConfig, audioConfigLen, NULL);
  
  sdpMediaAudio = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaAudio, 0, sizeof(*sdpMediaAudio));

  sdpMediaAudioFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaAudioFormat, 0, sizeof(*sdpMediaAudioFormat));
  sdpMediaAudio->fmt_list = sdpMediaAudioFormat;
  sdpMediaAudioFormat->media = sdpMediaAudio;
  
  if (pConfig->GetBoolValue(CFG_RTP_USE_MP3_PAYLOAD_14)) {
    sdpMediaAudioFormat->fmt = strdup("14");
    sdpMediaAudioFormat->rtpmap_clock_rate = 90000;
  } else {
    sdpMediaAudioFormat->rtpmap_clock_rate = 
      pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
    sdpMediaAudioFormat->fmt = strdup("97");
  }
  sdpMediaAudioFormat->rtpmap_name = strdup("MPA");
	
  return sdpMediaAudio;

}

static bool twolame_set_rtp_header (struct iovec *iov,
				 int queue_cnt,
				 void *ud, 
				 bool *mbit)
{
  *mbit = 1;
  *(uint32_t *)ud = 0;
  iov[0].iov_base = ud;
  iov[0].iov_len = 4;
  return true;
}

static bool twolame_set_rtp_jumbo (struct iovec *iov,
				uint32_t dataOffset,
				uint32_t bufferLen,
				uint32_t rtpPacketMax,
				bool &mbit,
				void *ud)
{
  uint8_t *payloadHeader = (uint8_t *)ud;
  uint32_t send;

  payloadHeader[0] = 0;
  payloadHeader[1] = 0;
  payloadHeader[2] = (dataOffset >> 8);
  payloadHeader[3] = (dataOffset & 0xff);

  send = MIN(bufferLen - dataOffset, rtpPacketMax - 4);

  iov[0].iov_base = payloadHeader;
  iov[0].iov_len = 4;
  iov[1].iov_len = send;

  mbit = (dataOffset == 0);
  return true;
}

bool twolame_get_audio_rtp_info (CAudioProfile *pConfig,
			      MediaType *audioFrameType,
			      uint32_t *audioTimeScale,
			      uint8_t *audioPayloadNumber,
			      uint8_t *audioPayloadBytesPerPacket,
			      uint8_t *audioPayloadBytesPerFrame,
			      uint8_t *audioQueueMaxCount,
			      audio_set_rtp_header_f *audio_set_header,
			      audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			      void **ud)
{
  *audioFrameType = MP3AUDIOFRAME;
  if (pConfig->GetBoolValue(CFG_RTP_USE_MP3_PAYLOAD_14)) {
    *audioPayloadNumber = 14;
    *audioTimeScale = 90000;
  } else {
    *audioPayloadNumber = 97;
    *audioTimeScale = pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
  }
  *audioPayloadBytesPerPacket = 4;
  *audioPayloadBytesPerFrame = 0;
  *audioQueueMaxCount = 8;
  *audio_set_header = twolame_set_rtp_header;
  *audio_set_jumbo = twolame_set_rtp_jumbo;
  *ud = malloc(4);
  memset(*ud, 0, 4);
  return true;
}


CTwoLameAudioEncoder::CTwoLameAudioEncoder(CAudioProfile *ap, 
				     CAudioEncoder *next, 
				     u_int8_t srcChannels,
				     u_int32_t srcSampleRate,
				     uint16_t mtu,
				     bool realTime) :
  CAudioEncoder(ap, next, srcChannels, srcSampleRate, mtu, realTime)
{
	m_mp3FrameBuffer = NULL;
}

bool CTwoLameAudioEncoder::Init (void)
{
	if ((m_twolameParams = twolame_init()) == NULL) {
		error_message("error: failed to get twolame_global_flags");
		return false;
	} 
	twolame_set_num_channels(m_twolameParams,
			      Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS));
	twolame_set_in_samplerate(m_twolameParams,
			       Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE));
	twolame_set_brate(m_twolameParams,
		       Profile()->GetIntegerValue(CFG_AUDIO_BIT_RATE) / 1000);
	twolame_set_mode(m_twolameParams,
		      (Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS) == 1 ? TWOLAME_MONO : TWOLAME_STEREO));		
	twolame_set_VBR(m_twolameParams, FALSE);
	//	twolame_set_quality(m_twolameParams,2);

	// no match for silent flag

	// no match for gtkflag

	// THIS IS VERY IMPORTANT. MP4PLAYER DOES NOT SEEM TO LIKE VBR
	//twolame_set_bWriteVbrTag(m_twolameParams,0);

	if (twolame_init_params(m_twolameParams) == -1) {
		error_message("error: failed init twolame params");
		return false;
	}
	if (twolame_get_in_samplerate(m_twolameParams) != twolame_get_out_samplerate(m_twolameParams)) {
		error_message("warning: twolame audio sample rate mismatch - wanted %d got %d",
			      twolame_get_in_samplerate(m_twolameParams), 
			      twolame_get_out_samplerate(m_twolameParams));
		Profile()->SetIntegerValue(CFG_AUDIO_SAMPLE_RATE,
			twolame_get_out_samplerate(m_twolameParams));
	}

	//error_message("twolame version is %d", twolame_get_version(m_twolameParams));
	m_samplesPerFrame = 1152; // always 1152

	m_mp3FrameMaxSize = (u_int)(1.25 * m_samplesPerFrame) + 7200;

	m_mp3FrameBufferSize = 2 * m_mp3FrameMaxSize;

	m_mp3FrameBufferLength = 0;

	m_mp3FrameBuffer = (u_int8_t*)malloc(m_mp3FrameBufferSize);

	if (!m_mp3FrameBuffer) {
		return false;
	}

	Initialize();
	return true;
}

u_int32_t CTwoLameAudioEncoder::GetSamplesPerFrame()
{
	return m_samplesPerFrame;
}

bool CTwoLameAudioEncoder::EncodeSamples(
	int16_t* pSamples, 
	u_int32_t numSamplesPerChannel,
	u_int8_t numChannels)
{
  if (numChannels != 1 && numChannels != 2) {
    return false;	// invalid numChannels
  }

  u_int32_t mp3DataLength = 0;

  if (pSamples != NULL) { 
    int16_t* pLeftBuffer = NULL;
    int16_t* pRightBuffer = NULL;

    if (numChannels == 1) {
      pLeftBuffer = pSamples;

      // both right and left need to be the same - can't 
      // pass NULL as pRightBuffer
      pRightBuffer = pSamples;
      mp3DataLength = twolame_encode_buffer(
					    m_twolameParams,
					    pLeftBuffer, 
					    pRightBuffer, 
					    m_samplesPerFrame,
					    (unsigned char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
					    m_mp3FrameBufferSize - m_mp3FrameBufferLength);

    } else { // numChannels == 2
      // let twolame handle stereo to mono conversion
      mp3DataLength = 
	twolame_encode_buffer_interleaved(m_twolameParams,
					  pSamples,
					  m_samplesPerFrame,
					  (unsigned char *)&m_mp3FrameBuffer[m_mp3FrameBufferLength],
					  m_mp3FrameBufferSize - m_mp3FrameBufferLength);
    } 
  } else { // pSamples == NULL
    // signal to stop encoding
    mp3DataLength = 
      twolame_encode_flush( m_twolameParams,
			    (unsigned char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			    m_mp3FrameBufferSize - m_mp3FrameBufferLength);
  }

  m_mp3FrameBufferLength += mp3DataLength;
  //debug_message("audio -return from twolame_encode_buffer is %d %d", mp3DataLength, m_mp3FrameBufferLength);

  return (mp3DataLength >= 0);
}

bool CTwoLameAudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, 
	u_int32_t* pBufferLength,
	u_int32_t* pNumSamplesPerChannel)
{
	const u_int8_t* mp3Frame;
	u_int32_t mp3FrameLength;

	if (!MP4AV_Mp3GetNextFrame(m_mp3FrameBuffer, m_mp3FrameBufferLength, 
	  &mp3Frame, &mp3FrameLength)) {
	  //debug_message("Can't find frame header - len %d", m_mp3FrameBufferLength);
		return false;
	}

	// check if we have all the bytes for the MP3 frame
	if (mp3FrameLength > m_mp3FrameBufferLength) {
	  //debug_message("Not enough in buffer - %d %d", m_mp3FrameBufferLength, mp3FrameLength);
		return false;
	}

	// need a buffer for this MP3 frame
	*ppBuffer = (u_int8_t*)malloc(mp3FrameLength);
	if (*ppBuffer == NULL) {
	  error_message("Cannot alloc memory");
		return false;
	}

	// copy the MP3 frame
	memcpy(*ppBuffer, mp3Frame, mp3FrameLength);
	*pBufferLength = mp3FrameLength;

	// shift what remains in the buffer down
	memmove(m_mp3FrameBuffer, 
		mp3Frame + mp3FrameLength, 
		m_mp3FrameBufferLength - mp3FrameLength);
	m_mp3FrameBufferLength -= mp3FrameLength;

	*pNumSamplesPerChannel = m_samplesPerFrame;

	return true;
}

void CTwoLameAudioEncoder::StopEncoder (void)
{
	free(m_mp3FrameBuffer);
	m_mp3FrameBuffer = NULL;
	twolame_close(&m_twolameParams);
	m_twolameParams = NULL;
}

#endif // HAVE_TWOLAME
