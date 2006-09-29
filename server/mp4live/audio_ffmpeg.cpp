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
#include "ffmpeg_if.h"

#ifdef HAVE_AVRATIONAL
#define MAY_HAVE_AMR_CODEC 1
#else
#undef MAY_HAVE_AMR_CODEC
#endif
// these are for mpeg2
static const uint32_t ffmpeg_sample_rates[] = {
  44100, 48000, 
};

static const uint32_t ffmpeg_g711_sample_rates[] = { 8000, };
static const uint32_t ffmpeg_bit_rates[] = {
  112000, 128000, 160000, 192000, 224000, 256000, 320000
};

GUI_BOOL(gui_mp3use14, CFG_RTP_USE_MP3_PAYLOAD_14, "Transmit MP3 using RFC-2250");
DECLARE_TABLE(ffmpeg_gui_options) = {
  TABLE_GUI(gui_mp3use14),
};
DECLARE_TABLE_FUNC(ffmpeg_gui_options);

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

static uint32_t *g711_bitrate_for_samplerate (uint32_t samplerate, 
					      uint8_t chans,
					      uint32_t *ret_size)
{
  uint32_t *ret = MALLOC_STRUCTURE(uint32_t);
  *ret = 64000;
  *ret_size = 1;
  return ret;
}

// right now, use ffmpeg for mpeg1, layer 2, so that we'll work
// with mpeg2 and quicktime
static audio_encoder_table_t ffmpeg_audio_encoder_table =  {
  "MPEG Layer 2 - ffmpeg",
  VIDEO_ENCODER_FFMPEG,
  AUDIO_ENCODING_MP3,
  ffmpeg_sample_rates,
  NUM_ELEMENTS_IN_ARRAY(ffmpeg_sample_rates),
  ffmpeg_bitrate_for_samplerate,
  2,
  TABLE_FUNC(ffmpeg_gui_options)
};

static audio_encoder_table_t ffmpeg_alaw_audio_encoder_table =  {
  "G.711 alaw - ffmpeg",
  VIDEO_ENCODER_FFMPEG,
  AUDIO_ENCODING_ALAW,
  ffmpeg_g711_sample_rates,
  NUM_ELEMENTS_IN_ARRAY(ffmpeg_g711_sample_rates),
  g711_bitrate_for_samplerate,
  1,
  NULL,
};

static audio_encoder_table_t ffmpeg_ulaw_audio_encoder_table =  {
  "G.711 ulaw - ffmpeg",
  VIDEO_ENCODER_FFMPEG,
  AUDIO_ENCODING_ULAW,
  ffmpeg_g711_sample_rates,
  NUM_ELEMENTS_IN_ARRAY(ffmpeg_g711_sample_rates),
  g711_bitrate_for_samplerate,
  1,
  NULL,
};

// these are for AMR
 
static const uint32_t ffmpeg_amr_sample_rates[] = {
  8000, 16000, 
};
 
static uint32_t *ffmpeg_amr_bitrate_for_samplerate (uint32_t samplerate, 
 						    uint8_t chans,
 						    uint32_t *ret_size)
{
  uint32_t *ret = (uint32_t *)malloc(16 * sizeof(uint32_t));
   
  if (samplerate == 8000) {
    ret[0] = 4750 * chans;
    ret[1] = 5150 * chans;
    ret[2] = 5900 * chans;
    ret[3] = 6700 * chans;
    ret[4] = 7400 * chans;
    ret[5] = 7950 * chans;
    ret[6] = 10200 * chans;
    ret[7] = 12200 * chans;
    *ret_size = 8;
  }
  else if (samplerate == 16000) {
    ret[0] = 6600 * chans;
    ret[1] = 8850 * chans;
    ret[2] = 12650 * chans;
    ret[3] = 14250 * chans;
    ret[4] = 15850 * chans;
    ret[5] = 18250 * chans;
    ret[6] = 19850 * chans;
    ret[7] = 23050 * chans;
    ret[8] = 23850 * chans;
    *ret_size = 9;
  }
  else {
    *ret_size = 0;
  }
   
  return ret;
}
 
 audio_encoder_table_t ffmpeg_amr_audio_encoder_table =  {
   "AMR",
   VIDEO_ENCODER_FFMPEG,
   AUDIO_ENCODING_AMR,
   ffmpeg_amr_sample_rates,
   NUM_ELEMENTS_IN_ARRAY(ffmpeg_amr_sample_rates),
   ffmpeg_amr_bitrate_for_samplerate,
   1
 };
 
    
MediaType ffmpeg_mp4_fileinfo (CAudioProfile *pConfig,
			       bool *mpeg4,
			       bool *isma_compliant,
			       uint8_t *audioProfile,
			       uint8_t **audioConfig,
			       uint32_t *audioConfigLen,
			       uint8_t *mp4AudioType)
{
  const char *encodingName = pConfig->GetStringValue(CFG_AUDIO_ENCODING);
  if (!strcasecmp(encodingName, AUDIO_ENCODING_MP3)) {
    *mpeg4 = true; // legal in an mp4 - create an iod
    *isma_compliant = false;
    *audioProfile = 0xfe;
    *audioConfig = NULL;
    *audioConfigLen = 0;
    if (mp4AudioType != NULL) {
      *mp4AudioType = MP4_MPEG1_AUDIO_TYPE;
    } 
    return MP3AUDIOFRAME;
  } else {
    *mpeg4 = false;
    *isma_compliant = false;
    *audioProfile = 0;
    *audioConfig = NULL;
    *audioConfigLen = 0;
    if (mp4AudioType != NULL) {
	*mp4AudioType = 0;
    }
    if (!strcasecmp(encodingName, AUDIO_ENCODING_AMR)) {
      if (pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE) == 8000) {
	return AMRNBAUDIOFRAME;
      } 
      return AMRWBAUDIOFRAME;
    } else if (!strcasecmp(encodingName, AUDIO_ENCODING_ULAW)) {
      if (mp4AudioType != NULL) {
	*mp4AudioType = MP4_ULAW_AUDIO_TYPE;
      }
      return ULAWAUDIOFRAME;
    } else if (!strcasecmp(encodingName, AUDIO_ENCODING_ALAW)) {
      if (mp4AudioType != NULL) {
	*mp4AudioType = MP4_ALAW_AUDIO_TYPE;
      }
      return ALAWAUDIOFRAME;
    }
  }
  return UNDEFINEDFRAME;
}

media_desc_t *ffmpeg_create_audio_sdp (CAudioProfile *pConfig,
				       bool *mpeg4,
				       bool *isma_compliant,
				       bool *is3gp,
				       uint8_t *audioProfile,
				       uint8_t **audioConfig,
				       uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;
  MediaType type;

  type = ffmpeg_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile,
			     audioConfig, audioConfigLen, NULL);

  sdpMediaAudio = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaAudio, 0, sizeof(*sdpMediaAudio));

  sdpMediaAudioFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaAudioFormat, 0, sizeof(*sdpMediaAudioFormat));
  sdpMediaAudio->fmt_list = sdpMediaAudioFormat;
  sdpMediaAudioFormat->media = sdpMediaAudio;
  

  if (type == ALAWAUDIOFRAME) {
    sdpMediaAudioFormat->fmt = strdup("8");
  } else if (type == ULAWAUDIOFRAME) {
    sdpMediaAudioFormat->fmt = strdup("0");
  } else {
    if (type == MP3AUDIOFRAME) {
      if (pConfig->GetBoolValue(CFG_RTP_USE_MP3_PAYLOAD_14)) {
	sdpMediaAudioFormat->fmt = strdup("14");
	sdpMediaAudioFormat->rtpmap_clock_rate = 90000;
      } else {
	sdpMediaAudioFormat->rtpmap_clock_rate = 
	  pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
	sdpMediaAudioFormat->fmt = strdup("97");
	sdp_add_string_to_list(&sdpMediaAudio->unparsed_a_lines,
			       "a=mpeg4-esid:10");
      }
      sdpMediaAudioFormat->rtpmap_name = strdup("MPA");
    } else if (type == AMRNBAUDIOFRAME) {
      *is3gp = true;
      sdpMediaAudioFormat->rtpmap_name = strdup("AMR");
      sdpMediaAudioFormat->rtpmap_clock_rate = 8000;
      sdpMediaAudioFormat->fmt = strdup("97");
      sdpMediaAudioFormat->fmt_param = strdup("octet-align=1");
      sdpMediaAudioFormat->rtpmap_encode_param = 1;
    } else if (type == AMRWBAUDIOFRAME) {
      *is3gp = true;
      sdpMediaAudioFormat->rtpmap_name = strdup("AMR-WB");
      sdpMediaAudioFormat->rtpmap_clock_rate = 16000;
      sdpMediaAudioFormat->fmt = strdup("97");
      sdpMediaAudioFormat->fmt_param = strdup("octet-align=1");
      sdpMediaAudioFormat->rtpmap_encode_param = 1;
    }
      
  }
	    
  return sdpMediaAudio;
}

static bool ffmpeg_set_rtp_header (struct iovec *iov,
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

 // compile RTP payload from a queue of frames
static int ffmpeg_amr_set_rtp_payload(CMediaFrame** m_audioQueue,
 				      int queue_cnt,
 				      struct iovec *iov,
 				      void *ud,
				      bool *mbit)
{
  uint8_t *payloadHeader = (uint8_t *)ud;
  
  *mbit = 0;
  payloadHeader[0] = 0xf0;
  
  for (int i = 0; i < queue_cnt; i++) {
    // extract mode field + quality bit & set the follow bit
    //    if (i > 0) payloadHeader[i] |= 0x80;
#if 0
    payloadHeader[i + 1] = (*(uint8_t*)m_audioQueue[i]->GetData() & 0x7C) | 0x80;
#else
    payloadHeader[i + 1] = (*(uint8_t*)m_audioQueue[i]->GetData() & 0x78) | 0x84;
#endif
    // body of the frame
    iov[i + 1].iov_base = (uint8_t*)m_audioQueue[i]->GetData() + 1;
    iov[i + 1].iov_len  = m_audioQueue[i]->GetDataLength() - 1;
    //    printf("data len is %d\n", iov[i + 1].iov_len);
  }
  // clear the follow bit in the last TOC entry
  payloadHeader[queue_cnt] &= 0x7F;
  iov[0].iov_base = payloadHeader;
  iov[0].iov_len  = queue_cnt + 1;
  //  printf("hdr  len is %d\n\n", queue_cnt + 1);
  return true;
}

bool ffmpeg_get_audio_rtp_info (CAudioProfile *pConfig,
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
  const char *encodingName = pConfig->GetStringValue(CFG_AUDIO_ENCODING);
  if (!strcasecmp(encodingName, AUDIO_ENCODING_MP3)) {

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
    *audio_set_header = ffmpeg_set_rtp_header;
    *audio_set_jumbo = ffmpeg_set_rtp_jumbo;
    *ud = malloc(4);
    memset(*ud, 0, 4);
    return true;
  }
  if (!strcasecmp(encodingName, AUDIO_ENCODING_AMR)) {
    *audioTimeScale = pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
    if (*audioTimeScale == 8000) {
      *audioFrameType = AMRNBAUDIOFRAME;
    } else {
      *audioFrameType = AMRWBAUDIOFRAME;
    }
    *audioPayloadNumber = 97;
    *audioPayloadBytesPerPacket = 4;
    *audioPayloadBytesPerFrame = 0;
    *audioQueueMaxCount = 5;
    *audio_set_rtp_payload = ffmpeg_amr_set_rtp_payload;
    *ud = malloc(10);
    memset(*ud, 0, 10);
    return true;
  }
  if (strcasecmp(encodingName, AUDIO_ENCODING_ALAW) == 0 ||
      strcasecmp(encodingName, AUDIO_ENCODING_ULAW) == 0) {
    if (strcasecmp(encodingName, AUDIO_ENCODING_ALAW) == 0) {
      *audioFrameType = ALAWAUDIOFRAME;
      *audioPayloadNumber = 8;
    } else {
      *audioFrameType = ULAWAUDIOFRAME;
      *audioPayloadNumber = 0;
    }
    *audioTimeScale = 8000;
    *audioPayloadBytesPerPacket = 0;
    *audioPayloadBytesPerFrame = 0;
    *audioQueueMaxCount = 1;
    *audio_set_header = NULL;
    *audio_set_jumbo = NULL;
    *ud = NULL;
    return true;
  }

  return false;
	    
}

CFfmpegAudioEncoder::CFfmpegAudioEncoder(CAudioProfile *ap,
					 CAudioEncoder *next, 
					 u_int8_t srcChannels,
					 u_int32_t srcSampleRate,
					 uint16_t mtu,
					 bool realTime) :
  CAudioEncoder(ap, next, srcChannels, srcSampleRate, mtu, realTime)
{
	m_FrameBuffer = NULL;
	m_codec = NULL;
	m_avctx = NULL;
}

bool CFfmpegAudioEncoder::Init (void)
{
  const char *encoding = Profile()->GetStringValue(CFG_AUDIO_ENCODING);

  avcodec_init();
  avcodec_register_all();

  if (strcasecmp(encoding,AUDIO_ENCODING_MP3) == 0) {
    m_codec = avcodec_find_encoder(CODEC_ID_MP2);
    m_media_frame = MP3AUDIOFRAME;
  } else if (strcasecmp(encoding, AUDIO_ENCODING_ALAW) == 0) {
    m_codec = avcodec_find_encoder(CODEC_ID_PCM_ALAW);
    m_media_frame = ALAWAUDIOFRAME;
  } else if (strcasecmp(encoding, AUDIO_ENCODING_ULAW) == 0) {
    m_codec = avcodec_find_encoder(CODEC_ID_PCM_MULAW);
    m_media_frame = ULAWAUDIOFRAME;
  } else if (strcasecmp(encoding, AUDIO_ENCODING_AMR) == 0) {
#ifdef MAY_HAVE_AMR_CODEC
    uint32_t samplingRate = Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
    if (samplingRate == 8000) {
      m_codec = avcodec_find_encoder(CODEC_ID_AMR_NB);
      m_media_frame= AMRNBAUDIOFRAME;
    } else {
      m_codec = avcodec_find_encoder(CODEC_ID_AMR_WB);
      m_media_frame = AMRWBAUDIOFRAME;
    }
#endif
  }
    

  if (m_codec == NULL) {
    error_message("Couldn't find audio codec");
    return false;
  }
  m_avctx = avcodec_alloc_context();
  m_frame = avcodec_alloc_frame();

  m_avctx->codec_type = CODEC_TYPE_AUDIO;
  switch (m_media_frame) {
  case MP3AUDIOFRAME:
    m_avctx->codec_id = CODEC_ID_MP2;
    m_samplesPerFrame = 
      MP4AV_Mp3GetSamplingWindow(Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE));
    m_FrameMaxSize = (u_int)(1.25 * m_samplesPerFrame) + 7200;
    break;
  case ALAWAUDIOFRAME:
    m_avctx->codec_id = CODEC_ID_PCM_ALAW;
    m_samplesPerFrame = 160;
    m_FrameMaxSize = m_samplesPerFrame / 2;
    break;
  case ULAWAUDIOFRAME:
    m_avctx->codec_id = CODEC_ID_PCM_MULAW;
    m_samplesPerFrame = 160;
    m_FrameMaxSize = m_samplesPerFrame / 2;
    break;
  case AMRNBAUDIOFRAME:
    m_avctx->codec_id = CODEC_ID_AMR_NB;
    m_samplesPerFrame = 
      MP4AV_AmrGetSamplingWindow(8000);
    m_FrameMaxSize = 64;
    break;
  case AMRWBAUDIOFRAME:
    m_avctx->codec_id = CODEC_ID_AMR_WB;
    m_samplesPerFrame = 
      MP4AV_AmrGetSamplingWindow(16000);
    m_FrameMaxSize = 64;
    break;
  }
  m_avctx->bit_rate = Profile()->GetIntegerValue(CFG_AUDIO_BIT_RATE);
  m_avctx->sample_rate = Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
  m_avctx->channels = Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS);

  ffmpeg_interface_lock();
  if (avcodec_open(m_avctx, m_codec) < 0) {
    ffmpeg_interface_unlock();
    error_message("Couldn't open ffmpeg codec");
    return false;
  }
  ffmpeg_interface_unlock();


  m_FrameBufferSize = 2 * m_FrameMaxSize;

  m_FrameBufferLength = 0;

  m_FrameBuffer = (u_int8_t*)malloc(m_FrameBufferSize);

  if (!m_FrameBuffer) {
    return false;
  }
  Initialize();
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

	u_int32_t DataLength = 0;

	if (pSamples != NULL) { 
	  DataLength = 
	    avcodec_encode_audio(m_avctx,
				 m_FrameBuffer,
				 m_FrameBufferSize,
				 pSamples);
	  //debug_message("ffmpeg encode %u", DataLength);
	} else {
	  return false;
	}

	m_FrameBufferLength += DataLength;
	//	error_message("audio -return from ffmpeg_encode_buffer is %d %d", mp3DataLength, m_mp3FrameBufferLength);

	return (DataLength >= 0);
}

bool CFfmpegAudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, 
	u_int32_t* pBufferLength,
	u_int32_t* pNumSamplesPerChannel)
{
	const u_int8_t* frame;
	u_int32_t frameLength;

	if (m_FrameBufferLength == 0) return false;

	// I'm not sure we actually need all this code; however, 
	// it doesn't hurt.  It's copied from the lame interface
	if (m_media_frame == MP3AUDIOFRAME) {
	  if (!MP4AV_Mp3GetNextFrame(m_FrameBuffer, m_FrameBufferLength, 
				     &frame, &frameLength)) {
	    //debug_message("Can't find frame header - len %d", m_mp3FrameBufferLength);
	    return false;
	  }
	  
	  // check if we have all the bytes for the MP3 frame
	} else if ((m_media_frame == AMRNBAUDIOFRAME) ||
		   (m_media_frame == AMRWBAUDIOFRAME)) {
	  
	  frame = m_FrameBuffer;
	  if (!MP4AV_AmrGetNextFrame(m_FrameBuffer, m_FrameBufferLength,
				     &frameLength,
				     m_media_frame == AMRNBAUDIOFRAME)) {
	    return false;
	  }
	} else {
	  frameLength = m_FrameBufferLength;
	  frame = m_FrameBuffer;
	}
#if 0
	error_message("buffer %d size %d", 
		      m_FrameBufferLength,frameLength);
#endif
	
	if (frameLength > m_FrameBufferLength) {
	  //debug_message("Not enough in buffer - %d %d", m_mp3FrameBufferLength, mp3FrameLength);
	  return false;
	}
	// need a buffer for this MP3 frame
	*ppBuffer = (u_int8_t*)malloc(frameLength);
	if (*ppBuffer == NULL) {
	  error_message("Cannot alloc memory");
		return false;
	}

	// copy the MP3 frame
	memcpy(*ppBuffer, frame, frameLength);
	*pBufferLength = frameLength;

	// shift what remains in the buffer down
	memcpy(m_FrameBuffer, 
		frame + frameLength, 
		m_FrameBufferLength - frameLength);
	//	error_message("vers %d layer %d", MP4AV_Mp3GetHdrVersion(MP4AV_Mp3HeaderFromBytes(*ppBuffer)),
	//      MP4AV_Mp3GetHdrLayer(MP4AV_Mp3HeaderFromBytes(*ppBuffer)));
	m_FrameBufferLength -= frameLength;

	*pNumSamplesPerChannel = m_samplesPerFrame;

	return true;
}

void CFfmpegAudioEncoder::StopEncoder(void)
{
  ffmpeg_interface_lock();
  avcodec_close(m_avctx);
  ffmpeg_interface_unlock();
  m_avctx = NULL;
  free(m_FrameBuffer);
  m_FrameBuffer = NULL;
}

void InitFFmpegAudio (void)
{
  AddAudioEncoderTable(&ffmpeg_audio_encoder_table);
  AddAudioEncoderTable(&ffmpeg_alaw_audio_encoder_table);
  AddAudioEncoderTable(&ffmpeg_ulaw_audio_encoder_table);
#ifdef MAY_HAVE_AMR_CODEC
  avcodec_init();
  avcodec_register_all();
  bool have_amr_nb = avcodec_find_encoder(CODEC_ID_AMR_NB) != NULL;
  bool have_amr_wb = avcodec_find_encoder(CODEC_ID_AMR_WB) != NULL;

  if (have_amr_nb == false && have_amr_wb == false) return;

  if (have_amr_nb && have_amr_wb == false) {
    ffmpeg_amr_audio_encoder_table.num_sample_rates = 1;
  } else if (have_amr_nb == false && have_amr_wb) {
    ffmpeg_amr_audio_encoder_table.num_sample_rates = 1;
    ffmpeg_amr_audio_encoder_table.sample_rates = 
      &ffmpeg_amr_sample_rates[1];
  }
  AddAudioEncoderTable(&ffmpeg_amr_audio_encoder_table);
#endif
}
    
#endif
