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
#ifdef HAVE_LAME
#include "audio_lame.h"
#include <mp4av.h>

GUI_BOOL(gui_mp3use14, CFG_RTP_USE_MP3_PAYLOAD_14, "Transmit MP3 using RFC-2250");
DECLARE_TABLE(lame_gui_options) = {
  TABLE_GUI(gui_mp3use14),
};
DECLARE_TABLE_FUNC(lame_gui_options);

static const uint32_t lame_sample_rates[] = {
  8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 
};

static int get_mpeg_type_for_samplerate (int sr)
{
  for (uint x = 0; x < 3; x++) {
    for (uint y = 0; y < 4; y++) {
      if (samplerate_table[x][y] == sr) {
	return x;
      }
    }
  }
  return -1;
}

static uint32_t *lame_bitrate_for_samplerate (uint32_t samplerate, 
					      uint8_t chans,
					      uint32_t *ret_size)
{
  int ix = get_mpeg_type_for_samplerate(samplerate);
  int iy;

  if (ix < 0) {
    return NULL;
  }
  uint32_t *ret = (uint32_t *)malloc(16 * sizeof(uint32_t));
  *ret_size = 0;
  lame_global_flags *lameParams;

  for (iy = 0; iy < 16; iy++) {
    if (bitrate_table[ix][iy] > 0) {
      lameParams = lame_init();
      lame_set_num_channels(lameParams, chans);
      lame_set_in_samplerate(lameParams, samplerate);
      lame_set_mode(lameParams,
		    (chans == 1 ? MONO : STEREO));		
      lame_set_quality(lameParams,2);
      lame_set_bWriteVbrTag(lameParams,0);
      lame_set_brate(lameParams,
		     bitrate_table[ix][iy]);

      if (lame_init_params(lameParams) != -1) {
	if (lame_get_in_samplerate(lameParams) == lame_get_out_samplerate(lameParams)) {
	  ret[*ret_size] = bitrate_table[ix][iy] * 1000;
	  *ret_size = *ret_size + 1;
	}
      }
      lame_close(lameParams);
    }
  }
  return ret;
}

audio_encoder_table_t lame_audio_encoder_table =  {
  "MP3 - lame",
  AUDIO_ENCODER_LAME,
  AUDIO_ENCODING_MP3,
  lame_sample_rates,
  NUM_ELEMENTS_IN_ARRAY(lame_sample_rates),
  lame_bitrate_for_samplerate,
  2,
  lame_gui_options_f,
};
    
MediaType lame_mp4_fileinfo (CAudioProfile *pConfig,
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
    *mp4AudioType = MP4_MP3_AUDIO_TYPE;
  }
  return MP3AUDIOFRAME;
}

media_desc_t *lame_create_audio_sdp (CAudioProfile *pConfig,
				     bool *mpeg4,
				     bool *isma_compliant,
				     uint8_t *audioProfile,
				     uint8_t **audioConfig,
				     uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;

  lame_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile,
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

static bool lame_set_rtp_header (struct iovec *iov,
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

static bool lame_set_rtp_jumbo (struct iovec *iov,
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

bool lame_get_audio_rtp_info (CAudioProfile *pConfig,
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
  *audio_set_header = lame_set_rtp_header;
  *audio_set_jumbo = lame_set_rtp_jumbo;
  *ud = malloc(4);
  memset(*ud, 0, 4);
  return true;
}


CLameAudioEncoder::CLameAudioEncoder(CAudioProfile *ap, 
				     CAudioEncoder *next, 
				     u_int8_t srcChannels,
				     u_int32_t srcSampleRate,
				     uint16_t mtu,
				     bool realTime) :
  CAudioEncoder(ap, next, srcChannels, srcSampleRate, mtu, realTime)
{
	m_mp3FrameBuffer = NULL;
}

bool CLameAudioEncoder::Init (void)
{
	if ((m_lameParams = lame_init()) == NULL) {
		error_message("error: failed to get lame_global_flags");
		return false;
	} 
	lame_set_num_channels(m_lameParams,
			      Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS));
	lame_set_in_samplerate(m_lameParams,
			       Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE));
	lame_set_brate(m_lameParams,
		       Profile()->GetIntegerValue(CFG_AUDIO_BIT_RATE) / 1000);
	lame_set_mode(m_lameParams,
		      (Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS) == 1 ? MONO : STEREO));		
	lame_set_quality(m_lameParams,2);

	// no match for silent flag

	// no match for gtkflag

	// THIS IS VERY IMPORTANT. MP4PLAYER DOES NOT SEEM TO LIKE VBR
	lame_set_bWriteVbrTag(m_lameParams,0);

	if (lame_init_params(m_lameParams) == -1) {
		error_message("error: failed init lame params");
		return false;
	}
	if (lame_get_in_samplerate(m_lameParams) != lame_get_out_samplerate(m_lameParams)) {
		error_message("warning: lame audio sample rate mismatch - wanted %d got %d",
			      lame_get_in_samplerate(m_lameParams), 
			      lame_get_out_samplerate(m_lameParams));
		Profile()->SetIntegerValue(CFG_AUDIO_SAMPLE_RATE,
			lame_get_out_samplerate(m_lameParams));
	}

	//error_message("lame version is %d", lame_get_version(m_lameParams));
	m_samplesPerFrame = MP4AV_Mp3GetSamplingWindow(
		Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE));

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

u_int32_t CLameAudioEncoder::GetSamplesPerFrame()
{
	return m_samplesPerFrame;
}

bool CLameAudioEncoder::EncodeSamples(
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
		bool mallocedLeft = false;
		int16_t* pRightBuffer = NULL;
		bool mallocedRight = false;

		if (numChannels == 1) {
		  pLeftBuffer = pSamples;

		  // both right and left need to be the same - can't 
		  // pass NULL as pRightBuffer
		  pRightBuffer = pSamples;

		} else { // numChannels == 2
		  // let lame handle stereo to mono conversion
			DeinterleaveStereoSamples(
				pSamples, 
				numSamplesPerChannel,
				&pLeftBuffer, 
				&pRightBuffer);

			mallocedLeft = true;
			mallocedRight = true;
		} 

		// call lame encoder
		mp3DataLength = lame_encode_buffer(
			m_lameParams,
			pLeftBuffer, 
			pRightBuffer, 
			m_samplesPerFrame,
			(unsigned char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			m_mp3FrameBufferSize - m_mp3FrameBufferLength);
		if (mallocedLeft) {
			free(pLeftBuffer);
			pLeftBuffer = NULL;
		}
		if (mallocedRight) {
			free(pRightBuffer);
			pRightBuffer = NULL;
		}

	} else { // pSamples == NULL
		// signal to stop encoding
	  mp3DataLength = 
	    lame_encode_flush( m_lameParams,
			       (unsigned char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			       m_mp3FrameBufferSize - m_mp3FrameBufferLength);
	}

	m_mp3FrameBufferLength += mp3DataLength;
	//debug_message("audio -return from lame_encode_buffer is %d %d", mp3DataLength, m_mp3FrameBufferLength);

	return (mp3DataLength >= 0);
}

bool CLameAudioEncoder::GetEncodedFrame(
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

void CLameAudioEncoder::StopEncoder (void)
{
	free(m_mp3FrameBuffer);
	m_mp3FrameBuffer = NULL;
	lame_close(m_lameParams);
	m_lameParams = NULL;
}

#endif // HAVE_LAME
