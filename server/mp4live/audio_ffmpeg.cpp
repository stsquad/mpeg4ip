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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May   wmay@cisco.com
 */

#include "mp4live.h"
#ifdef HAVE_FFMPEG
#include "audio_ffmpeg.h"
#include <mp4av.h>

// these are for mpeg2
static const uint32_t ffmpeg_sample_rates[] = {
  44100, 48000, 
};

static const uint32_t ffmpeg_bit_rates[] = {
  112000, 128000, 160000, 192000, 224000, 256000, 320000
};

static int get_index_for_samplerate (uint32_t sr)
{
  unsigned int ix;
  for (ix = 0; ix < NUM_ELEMENTS_IN_ARRAY(ffmpeg_sample_rates); ix ++) {
    if (sr == ffmpeg_sample_rates[ix]) {
      return ix;
    }
  }
  return -1;
}


static uint32_t *ffmpeg_bitrate_for_samplerate (uint32_t samplerate, 
						uint8_t chans,
						uint32_t *ret_size)
{
  int ix = get_index_for_samplerate(samplerate);

  if (ix < 0) {
    return NULL;
  }
  uint32_t elements = NUM_ELEMENTS_IN_ARRAY(ffmpeg_bit_rates);
  elements -= ix;

  uint32_t *ret = (uint32_t *)malloc(elements * sizeof(uint32_t));
  *ret_size = elements;
  memcpy(ret, &ffmpeg_bit_rates[ix], elements * sizeof(uint32_t));
  return ret;
}

// right now, use ffmpeg for mpeg1, layer 2, so that we'll work
// with mpeg2 and quicktime
audio_encoder_table_t ffmpeg_audio_encoder_table =  {
  "MPEG Layer 2 - ffmpeg",
  VIDEO_ENCODER_FFMPEG,
  AUDIO_ENCODING_MP3,
  ffmpeg_sample_rates,
  NUM_ELEMENTS_IN_ARRAY(ffmpeg_sample_rates),
  ffmpeg_bitrate_for_samplerate,
  2
};
    
MediaType ffmpeg_mp4_fileinfo (CLiveConfig *pConfig,
			     bool *mpeg4,
			     bool *isma_compliant,
			     uint8_t *audioProfile,
			     uint8_t **audioConfig,
			     uint32_t *audioConfigLen,
			     uint8_t *mp4AudioType)
{
  *mpeg4 = true; // legal in an mp4 - create an iod
  *isma_compliant = false;
  *audioProfile = 0xfe;
  *audioConfig = NULL;
  *audioConfigLen = 0;
  if (mp4AudioType != NULL) {
    *mp4AudioType = MP4_MPEG1_AUDIO_TYPE;
  }
  return MP3AUDIOFRAME;
}

media_desc_t *ffmpeg_create_audio_sdp (CLiveConfig *pConfig,
				     bool *mpeg4,
				     bool *isma_compliant,
				     uint8_t *audioProfile,
				     uint8_t **audioConfig,
				     uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;
  rtpmap_desc_t *sdpAudioRtpMap;

  ffmpeg_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile,
		    audioConfig, audioConfigLen, NULL);

  sdpMediaAudio = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaAudio, 0, sizeof(*sdpMediaAudio));

  sdpMediaAudioFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaAudioFormat, 0, sizeof(*sdpMediaAudioFormat));
  sdpMediaAudio->fmt = sdpMediaAudioFormat;
  sdpMediaAudioFormat->media = sdpMediaAudio;
  
  sdpAudioRtpMap = MALLOC_STRUCTURE(rtpmap_desc_t);
  memset(sdpAudioRtpMap, 0, sizeof(*sdpAudioRtpMap));

		
  if (pConfig->GetBoolValue(CONFIG_RTP_USE_MP3_PAYLOAD_14)) {
    sdpMediaAudioFormat->fmt = strdup("14");
    sdpAudioRtpMap->clock_rate = 90000;
  } else {
    sdpAudioRtpMap->clock_rate = 
      pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
    sdpMediaAudioFormat->fmt = strdup("97");
    sdp_add_string_to_list(&sdpMediaAudio->unparsed_a_lines,
			   "a=mpeg4-esid:10");
  }
  sdpAudioRtpMap->encode_name = strdup("MPA");
  
  sdpMediaAudioFormat->rtpmap = sdpAudioRtpMap;
	    
	
  return sdpMediaAudio;

}

static bool ffmpeg_set_rtp_header (struct iovec *iov,
				 int queue_cnt,
				 void *ud)
{
  *(uint32_t *)ud = 0;
  iov[0].iov_base = ud;
  iov[0].iov_len = 4;
  return true;
}

static bool ffmpeg_set_rtp_jumbo (struct iovec *iov,
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

bool ffmpeg_get_audio_rtp_info (CLiveConfig *pConfig,
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
  if (pConfig->GetBoolValue(CONFIG_RTP_USE_MP3_PAYLOAD_14)) {
    *audioPayloadNumber = 14;
    *audioTimeScale = 90000;
  } else {
    *audioPayloadNumber = 97;
    *audioTimeScale = pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
  }
  *audioPayloadBytesPerPacket = 4;
  *audioPayloadBytesPerFrame = 0;
  *audioQueueMaxCount = 8;
  *audio_set_header = ffmpeg_set_rtp_header;
  *audio_set_jumbo = ffmpeg_set_rtp_jumbo;
  *ud = malloc(4);
  memset(*ud, 0, 4);
  return true;
}

CFfmpegAudioEncoder::CFfmpegAudioEncoder()
{
	m_mp3FrameBuffer = NULL;
	m_codec = NULL;
	m_avctx = NULL;
}

bool CFfmpegAudioEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
  avcodec_init();
  avcodec_register_all();

  m_pConfig = pConfig;

  m_codec = avcodec_find_encoder(CODEC_ID_MP2);
  if (m_codec == NULL) {
    error_message("Couldn't find codec");
    return false;
  }
  m_avctx = avcodec_alloc_context();
  m_frame = avcodec_alloc_frame();

  m_avctx->codec_type = CODEC_TYPE_AUDIO;
  m_avctx->codec_id = CODEC_ID_MP2;
  m_avctx->bit_rate = m_pConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE);
  m_avctx->sample_rate = m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
  m_avctx->channels = m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);

  if (avcodec_open(m_avctx, m_codec) < 0) {
    error_message("Couldn't open ffmpeg codec");
    return false;
  }
  m_samplesPerFrame = 
    MP4AV_Mp3GetSamplingWindow(m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

  m_mp3FrameMaxSize = (u_int)(1.25 * m_samplesPerFrame) + 7200;

  m_mp3FrameBufferSize = 2 * m_mp3FrameMaxSize;

  m_mp3FrameBufferLength = 0;

  m_mp3FrameBuffer = (u_int8_t*)malloc(m_mp3FrameBufferSize);

  if (!m_mp3FrameBuffer) {
    return false;
  }
  
  return true;
}

u_int32_t CFfmpegAudioEncoder::GetSamplesPerFrame()
{
	return m_samplesPerFrame;
}

bool CFfmpegAudioEncoder::EncodeSamples(
	int16_t* pSamples, 
	u_int32_t numSamplesPerChannel,
	u_int8_t numChannels)
{
	if (numChannels != 1 && numChannels != 2) {
		return false;	// invalid numChannels
	}

	u_int32_t mp3DataLength = 0;

	if (pSamples != NULL) { 
	  mp3DataLength = 
	    avcodec_encode_audio(m_avctx,
				 m_mp3FrameBuffer,
				 m_mp3FrameBufferSize,
				 pSamples);

	} else {
	  return false;
	}

	m_mp3FrameBufferLength += mp3DataLength;
	//	error_message("audio -return from ffmpeg_encode_buffer is %d %d", mp3DataLength, m_mp3FrameBufferLength);

	return (mp3DataLength >= 0);
}

bool CFfmpegAudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, 
	u_int32_t* pBufferLength,
	u_int32_t* pNumSamplesPerChannel)
{
	const u_int8_t* mp3Frame;
	u_int32_t mp3FrameLength;

	// I'm not sure we actually need all this code; however, 
	// it doesn't hurt.  It's copied from the lame interface

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
	memcpy(m_mp3FrameBuffer, 
		mp3Frame + mp3FrameLength, 
		m_mp3FrameBufferLength - mp3FrameLength);
	//	error_message("vers %d layer %d", MP4AV_Mp3GetHdrVersion(MP4AV_Mp3HeaderFromBytes(*ppBuffer)),
	//      MP4AV_Mp3GetHdrLayer(MP4AV_Mp3HeaderFromBytes(*ppBuffer)));
	m_mp3FrameBufferLength -= mp3FrameLength;

	*pNumSamplesPerChannel = m_samplesPerFrame;

	return true;
}

void CFfmpegAudioEncoder::Stop()
{
  avcodec_close(m_avctx);
  m_avctx = NULL;
  free(m_mp3FrameBuffer);
  m_mp3FrameBuffer = NULL;
}

#endif
