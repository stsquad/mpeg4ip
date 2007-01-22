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
#ifdef HAVE_FAAC
#include "audio_faac.h"
#include "mp4.h"
#include "mp4av.h"

static const u_int32_t samplingRateAllValues[] = {
  7350, 8000, 11025, 12000, 16000, 22050, 
  24000, 32000, 44100, 48000, 64000, 88200, 96000
};

static const u_int32_t bitRateAllValues[] = {
  8000, 16000, 24000, 32000, 40000, 48000, 
  56000, 64000, 80000, 96000, 112000, 128000, 
  144000, 160000, 192000, 224000, 256000, 320000
};

static uint32_t *faac_bitrates_for_samplerate (uint32_t samplerate, 
					       uint8_t chans, 
					       uint32_t *ret_size)
{
  uint32_t *ret = (uint32_t *)malloc(sizeof(bitRateAllValues));

  memcpy(ret, bitRateAllValues, sizeof(bitRateAllValues));
  *ret_size = NUM_ELEMENTS_IN_ARRAY(bitRateAllValues);
  return ret;
}

GUI_BOOL(gui_rfc3016, CFG_RTP_RFC3016, "Transmit AAC using RFC-3016 (LATM - for 3gp)");

DECLARE_TABLE(faac_gui_options) = {
  TABLE_GUI(gui_rfc3016),
};

DECLARE_TABLE_FUNC(faac_gui_options);

audio_encoder_table_t faac_audio_encoder_table = {
  "AAC - FAAC", 
  AUDIO_ENCODER_FAAC,
  AUDIO_ENCODING_AAC,
  samplingRateAllValues,
  NUM_ELEMENTS_IN_ARRAY(samplingRateAllValues),
  faac_bitrates_for_samplerate,
  2,
  TABLE_FUNC(faac_gui_options)
};

MediaType faac_mp4_fileinfo (CAudioProfile *pConfig,
			     bool *mpeg4,
			     bool *isma_compliant,
			     uint8_t *audioProfile,
			     uint8_t **audioConfig,
			     uint32_t *audioConfigLen,
			     uint8_t *mp4AudioType)
{
  *mpeg4 = true;
  if(pConfig->GetBoolValue(CFG_RTP_RFC3016)) {
    *isma_compliant = false;
    *audioProfile = 0x0f;      // What does this mean?
    if (mp4AudioType) *mp4AudioType = MP4_MPEG4_AUDIO_TYPE;

    // TODO This has to change
    MP4AV_AacGetConfiguration_LATM(audioConfig,
			    audioConfigLen,
			    MP4AV_AAC_LC_PROFILE,
			    pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE),
			    pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS));
  } else {
    *isma_compliant = true;
    *audioProfile = 0x0f;
    if (mp4AudioType) *mp4AudioType = MP4_MPEG4_AUDIO_TYPE;

    MP4AV_AacGetConfiguration(audioConfig,
			    audioConfigLen,
			    MP4AV_AAC_LC_PROFILE,
			    pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE),
			    pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS));
  }
  return AACAUDIOFRAME;
}

media_desc_t *faac_create_audio_sdp (CAudioProfile *pConfig,
				     bool *mpeg4,
				     bool *isma_compliant,
				     uint8_t *audioProfile,
				     uint8_t **audioConfig,
				     uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;
  char audioFmtpBuf[512];

  faac_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile, audioConfig,
 	    audioConfigLen, NULL);

  sdpMediaAudio = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaAudio, 0, sizeof(*sdpMediaAudio));

  sdpMediaAudioFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaAudioFormat, 0, sizeof(*sdpMediaAudioFormat));

  sdpMediaAudioFormat->media = sdpMediaAudio;
  sdpMediaAudioFormat->fmt = strdup("97");

  sdpMediaAudioFormat->rtpmap_clock_rate = pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);

  char* sConfig = MP4BinaryToBase16(*audioConfig, *audioConfigLen);
  if(pConfig->GetBoolValue(CFG_RTP_RFC3016)) {
    sdpMediaAudioFormat->rtpmap_name = strdup("MP4A-LATM");
    sprintf(audioFmtpBuf, "profile-level-id=15;object=2;cpresent=0; config=%s ", sConfig); 

  } else {
    sdp_add_string_to_list(&sdpMediaAudio->unparsed_a_lines, "a=mpeg4-esid:10");
    sdpMediaAudioFormat->rtpmap_name = strdup("mpeg4-generic");

    sprintf(audioFmtpBuf,
	    "streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s; "
	    "SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;",
	    sConfig); 
  }

  free(sConfig);
  sdpMediaAudioFormat->fmt_param = strdup(audioFmtpBuf);
  sdpMediaAudio->fmt_list = sdpMediaAudioFormat;

  return sdpMediaAudio;
}

#define AAC_MAX_FRAME_IN_RTP_PAK 8
// Only used for RFC xxxx format
static bool faac_add_rtp_header (struct iovec *iov,
				 int queue_cnt,
				 void *ud, 
				 bool *m_bit)
{
  uint16_t numHdrBits = 16 * queue_cnt;
  int ix;
  uint8_t *aacHeader = (uint8_t *)ud;
  *m_bit = true;
  aacHeader[0] = numHdrBits >> 8;
  aacHeader[1] = numHdrBits & 0xff;

  for (ix = 1; ix <= queue_cnt; ix++) {
    aacHeader[2 + ((ix - 1) * 2)] = 
      iov[ix].iov_len >> 5;
    aacHeader[3 + ((ix - 1) * 2)] = 
      (iov[ix].iov_len & 0x1f) << 3;
  }
  iov[0].iov_base = aacHeader;
  iov[0].iov_len = 2 + (queue_cnt * 2);
  return true;
}

static bool faac_set_rtp_jumbo_frame (struct iovec *iov,
				      uint32_t dataOffset,
				      uint32_t bufferLen,
				      uint32_t rtpPayloadMax,
				      bool &mbit, 
				      void *ud)
{
  uint8_t *payloadHeader = (uint8_t *)ud;
  if (dataOffset == 0) {
    // first packet
    mbit = false;
    payloadHeader[0] = 0;
    payloadHeader[1] = 16;
    payloadHeader[2] = bufferLen >> 5;
    payloadHeader[3] = (bufferLen & 0x1f) << 3;
    iov[0].iov_base = payloadHeader;
    iov[0].iov_len = 4;
    iov[1].iov_len = MIN(rtpPayloadMax - 4, bufferLen);
    return true;
  } 
  mbit = false;
  iov[1].iov_len = MIN(bufferLen - dataOffset, rtpPayloadMax);
  return false;
}

// Compile RTP payload from a queue of frames
// TODO With the current implementation it is not possible to fragment a frame which is bigger than mtu
static int faac_rfc3016_set_rtp_payload(CMediaFrame** m_audioQueue,
 				      int queue_cnt,
 				      struct iovec *iov,
 				      void *ud,
				      bool *mbit)
{
  uint32_t data_size;
  uint8_t latm_hdr_size, *payloadHeader = (uint8_t *)ud;

  *mbit = 1; // Fragmenting not supported
  // RFC3016 recomends only one frame per rtp packet so queue_cnt will be 1
  for (int i = 0; i < queue_cnt*2; i+=2) {
    data_size = m_audioQueue[i]->GetDataLength();
    latm_hdr_size = (data_size / 255) + 1; 

		for (uint8_t j=0; j<latm_hdr_size; j++) {
			payloadHeader[j] = 255; 
		}
		payloadHeader[latm_hdr_size-1] = data_size % 255; 

    iov[i].iov_base = payloadHeader;
    iov[i].iov_len  = latm_hdr_size;

    // body of the frame
    iov[i + 1].iov_base = (uint8_t*)m_audioQueue[i]->GetData();
    iov[i + 1].iov_len  = m_audioQueue[i]->GetDataLength();
  }
  return true;
}

bool faac_get_audio_rtp_info (CAudioProfile *pConfig,
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
  *audioFrameType = AACAUDIOFRAME;
  *audioTimeScale = pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
  *audioPayloadNumber = 97;
  *audioPayloadBytesPerPacket = 2;
  *audioPayloadBytesPerFrame = 2;
  if(pConfig->GetBoolValue(CFG_RTP_RFC3016)) {
    *audioQueueMaxCount = 1;
    *ud = malloc(6); // This should be the maximum lengt of the LATM header
    *audio_set_rtp_payload = faac_rfc3016_set_rtp_payload;
  } else {
    *audioQueueMaxCount = AAC_MAX_FRAME_IN_RTP_PAK;
    *ud = malloc(2 + (2 * AAC_MAX_FRAME_IN_RTP_PAK));
    *audio_set_header = faac_add_rtp_header;
    *audio_set_jumbo = faac_set_rtp_jumbo_frame;
  }
  return true;
}

CFaacAudioEncoder::CFaacAudioEncoder(CAudioProfile *profile,
				     CAudioEncoder *next,
				     u_int8_t srcChannels,
				     u_int32_t srcSampleRate,
				     uint16_t mtu,
				     bool realTime) :
  CAudioEncoder(profile, next, srcChannels, srcSampleRate, mtu,realTime)
{
  m_faacHandle = NULL;
  m_samplesPerFrame = 1024;
  m_aacFrameBuffer = NULL;
  m_aacFrameBufferLength = 0;
  m_aacFrameMaxSize = 0;
}

bool CFaacAudioEncoder::Init(void)
{
  unsigned long spf, max;
  m_faacHandle = faacEncOpen(
                             Profile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE),
                             Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS),
			     &spf, &max);

  m_samplesPerFrame = spf;
  m_aacFrameMaxSize = max;

  if (m_faacHandle == NULL) {
    return false;
  }


  m_samplesPerFrame /= Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS);

  m_faacConfig = faacEncGetCurrentConfiguration(m_faacHandle);

  debug_message("version = %d", m_faacConfig->version);
  debug_message("name = %s", m_faacConfig->name);
  debug_message("allowMidside = %d", m_faacConfig->allowMidside);
  debug_message("useLfe = %d", m_faacConfig->useLfe);
  debug_message("useTns = %d", m_faacConfig->useTns);
  debug_message("bitRate = %lu", m_faacConfig->bitRate);
  debug_message("bandWidth = %d", m_faacConfig->bandWidth);
  debug_message("quantqual = %lu", m_faacConfig->quantqual);
  debug_message("outputFormat = %d", m_faacConfig->outputFormat);
  debug_message("psymodelidx = %d", m_faacConfig->psymodelidx);
  debug_message("inputFormat = %d", m_faacConfig->inputFormat);

  m_faacConfig->mpegVersion = MPEG4;
  m_faacConfig->aacObjectType = LOW;
  m_faacConfig->allowMidside = false;
  m_faacConfig->useLfe = false;
  m_faacConfig->useTns = false;
  m_faacConfig->inputFormat = FAAC_INPUT_16BIT;
  m_faacConfig->outputFormat = 0;    // raw
  m_faacConfig->quantqual = 0;    // use abr

  m_faacConfig->bitRate = 
    Profile()->GetIntegerValue(CFG_AUDIO_BIT_RATE)
    / Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS);

  faacEncSetConfiguration(m_faacHandle, m_faacConfig);
  Initialize();
  return true;
}

u_int32_t CFaacAudioEncoder::GetSamplesPerFrame()
{
  return m_samplesPerFrame;
}

bool CFaacAudioEncoder::EncodeSamples(
                                      int16_t* pSamples, 
                                      u_int32_t numSamplesPerChannel,
                                      u_int8_t numChannels)
{
  if (numChannels != 1 && numChannels != 2) {
    return false;	// invalid numChannels
  }

  // check for signal to end encoding
  if (pSamples == NULL) {
    // unlike lame, faac doesn't need to finish up anything
    return false;
  }

  int16_t* pInputBuffer = pSamples;
  bool inputBufferMalloced = false;

  // free old AAC buffer, just in case, should already be NULL
 CHECK_AND_FREE(m_aacFrameBuffer);

  // allocate the AAC buffer
  m_aacFrameBuffer = (u_int8_t*)Malloc(m_aacFrameMaxSize);
  // check for channel mismatch between src and dst
  if (numChannels != Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS)) {
    if (numChannels == 1) {
      // convert mono to stereo
      pInputBuffer = NULL;
      inputBufferMalloced = true;

      InterleaveStereoSamples(
                              pSamples,
                              pSamples,
                              numSamplesPerChannel,
                              &pInputBuffer);

    } else { // numChannels == 2
      // convert stereo to mono
      pInputBuffer = NULL;
      inputBufferMalloced = true;

      DeinterleaveStereoSamples(
				pSamples,
				numSamplesPerChannel,
				&pInputBuffer,
				NULL);
    }
  }

  int rc = faacEncEncode(
                         m_faacHandle,
                         (int32_t *)pInputBuffer,
                         numSamplesPerChannel
                         * Profile()->GetIntegerValue(CFG_AUDIO_CHANNELS),
                         m_aacFrameBuffer,
                         m_aacFrameMaxSize);

  if (inputBufferMalloced) {
    free(pInputBuffer);
    pInputBuffer = NULL;
  }

  if (rc < 0) {
    return false;
  }

  m_aacFrameBufferLength = rc;

  return true;
}

bool CFaacAudioEncoder::GetEncodedFrame(
                                        u_int8_t** ppBuffer, 
                                        u_int32_t* pBufferLength,
                                        u_int32_t* pNumSamplesPerChannel)
{
  if (m_aacFrameBufferLength == 0 && m_aacFrameBuffer) {
    free(m_aacFrameBuffer);
    m_aacFrameBuffer = NULL;
  }
  *ppBuffer = m_aacFrameBuffer;
  *pBufferLength = m_aacFrameBufferLength;
  *pNumSamplesPerChannel = m_samplesPerFrame;

  m_aacFrameBuffer = NULL;
  m_aacFrameBufferLength = 0;

  return true;
}

void CFaacAudioEncoder::StopEncoder (void)
{
  faacEncClose(m_faacHandle);
  m_faacHandle = NULL;
  CHECK_AND_FREE(m_aacFrameBuffer);
}

#endif
