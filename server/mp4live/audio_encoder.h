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

#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include "media_codec.h"
#include "media_frame.h"
#include <sdp.h>
#include <mp4.h>
#include "profile_audio.h"
#include "resampl.h"
#include "encoder_gui_options.h"

class CAudioEncoder : public CMediaCodec {
 public:
  CAudioEncoder(CAudioProfile *profile,
		CAudioEncoder *next, 
		u_int8_t srcChannels,
		u_int32_t srcSampleRate,
		uint16_t mtu,
		bool realTime = true);

  virtual u_int32_t GetSamplesPerFrame() = 0;
	
  virtual bool Init(void) = 0;

  void AddRtpDestination(CMediaStream *stream,
			 bool disable_ts_offset,
			 uint16_t max_ttl,
			 in_port_t srcPort = 0);
 public:
  // utility routines

  static bool InterleaveStereoSamples(
				      int16_t* pLeftBuffer, 
				      int16_t* pRightBuffer, 
				      u_int32_t numSamplesPerChannel,
				      int16_t** ppDstBuffer);

  static bool DeinterleaveStereoSamples(
					int16_t* pSrcBuffer, 
					u_int32_t numSamplesPerChannel,
					int16_t** ppLeftBuffer, 
					int16_t** ppRightBuffer); 
  CAudioEncoder *GetNext(void) {
    return (CAudioEncoder *)CMediaCodec::GetNext();
  };
 protected:
  int ThreadMain(void);
  CAudioProfile *Profile(void) { return (CAudioProfile *)m_pConfig; } ;

  void Initialize(void);
  virtual bool EncodeSamples(
			     int16_t* pSamples, 
			     u_int32_t numSamplesPerChannel,
			     u_int8_t numChannels) = 0;

  virtual bool GetEncodedFrame(
			       u_int8_t** ppBuffer, 
			       u_int32_t* pBufferLength,
			       u_int32_t* pNumSamplesPerChannel) = 0;

  CRtpTransmitter *CreateRtpTransmitter(bool disable_ts) {
    return new CAudioRtpTransmitter(Profile(), m_mtu, disable_ts);
  };

  void ProcessAudioFrame(CMediaFrame *frame);

  void ResampleAudio(
		     const u_int8_t* frameData,
		     u_int32_t frameDataLength);

  void ForwardEncodedAudioFrames(void);

  void DoStopAudio();

  // audio utility routines

  inline Duration SrcSamplesToTicks(u_int64_t numSamples) {
    return (numSamples * TimestampTicks) / m_audioSrcSampleRate;
  }

  inline Duration DstSamplesToTicks(u_int64_t numSamples) {
    return (numSamples * TimestampTicks) / m_audioDstSampleRate;
  }

  inline u_int32_t SrcTicksToSamples(Duration duration) {
    return (duration * m_audioSrcSampleRate) / TimestampTicks;
  }

  inline u_int32_t DstTicksToSamples(Duration duration) {
    return (duration * m_audioDstSampleRate) / TimestampTicks;
  }

  inline u_int32_t SrcSamplesToBytes(u_int64_t numSamples) {
    return (numSamples * m_audioSrcChannels * sizeof(u_int16_t));
  }

  inline u_int32_t DstSamplesToBytes(u_int64_t numSamples) {
    return (numSamples * m_audioDstChannels * sizeof(u_int16_t));
  }

  inline u_int64_t SrcBytesToSamples(u_int32_t numBytes) {
    return (numBytes / (m_audioSrcChannels * sizeof(u_int16_t)));
  }

  inline u_int64_t DstBytesToSamples(u_int32_t numBytes) {
    return (numBytes / (m_audioDstChannels * sizeof(u_int16_t)));
  }


  void AddSilenceFrame(void);

  // Audio encoding variables (timing, etc)
	u_int8_t		m_audioSrcChannels;
	u_int32_t		m_audioSrcSampleRate;
	u_int32_t		m_audioSrcFrameNumber;

	// audio resampling info
	resample_t              *m_audioResample;

	// audio destination info
	MediaType		m_audioDstType;
	u_int8_t		m_audioDstChannels;
	u_int32_t		m_audioDstSampleRate;
	u_int16_t		m_audioDstSamplesPerFrame;
	u_int64_t		m_audioDstSampleNumber;
	u_int32_t		m_audioDstFrameNumber;

	// audio encoding info
	u_int8_t*		m_audioPreEncodingBuffer;
	u_int32_t		m_audioPreEncodingBufferLength;
	u_int32_t		m_audioPreEncodingBufferMaxLength;

	// audio timing info
	Timestamp		m_audioStartTimestamp;
	Timestamp               m_audioEncodingStartTimestamp;
	Duration		m_audioSrcElapsedDuration;
	Duration		m_audioDstElapsedDuration;
};

void AudioProfileCheck(CAudioProfile *ap);

CAudioEncoder* AudioEncoderCreate(CAudioProfile *ap, 
				  CAudioEncoder *next,
				  u_int8_t srcChannels,
				  u_int32_t srcSampleRate,
				  uint16_t mtu,
				  bool realTime = true);
media_desc_t *create_audio_sdp(CAudioProfile *pConfig,
			       bool *mpeg4,
			       bool *isma_compliant,
			       bool *audio_is_3gp,
			       uint8_t *audioProfile,
			       uint8_t **audioConfig,
			       uint32_t *audioConfigLen);

MediaType get_audio_mp4_fileinfo(CAudioProfile *pConfig,
				 bool *mpeg4,
				 bool *isma_compliant,
				 uint8_t *audioProfile,
				 uint8_t **audioConfig,
				 uint32_t *audioConfigLen,
				 uint8_t *mp4_audio_type);

void create_mp4_audio_hint_track(CAudioProfile *pConfig, 
				 MP4FileHandle mp4file,
				 MP4TrackId trackId,
				 uint16_t mtu);


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
			 audio_set_rtp_jumbo_frame_f *audio_set_jumbo_frame,
			 void **ud);

void AudioProfileCheckBase(CAudioProfile *ap);

CAudioEncoder* AudioEncoderBaseCreate(CAudioProfile *ap, 
				      CAudioEncoder *next, 
				      u_int8_t srcChannels,
				      u_int32_t srcSampleRate,
				      uint16_t mtu,
				      bool realTime = true);

media_desc_t *create_base_audio_sdp(CAudioProfile *pConfig,
				    bool *mpeg4,
				    bool *isma_compliant,
				    bool *audio_is_3gp,
				    uint8_t *audioProfile,
				    uint8_t **audioConfig,
				    uint32_t *audioConfigLen);

MediaType get_base_audio_mp4_fileinfo(CAudioProfile *pConfig,
				      bool *mpeg4,
				      bool *isma_compliant,
				      uint8_t *audioProfile,
				      uint8_t **audioConfig,
				      uint32_t *audioConfigLen,
				      uint8_t *mp4_audio_type);

void create_base_mp4_audio_hint_track(CAudioProfile *pConfig, 
				      MP4FileHandle mp4file,
				      MP4TrackId trackId,
				      uint16_t mtu);


bool get_base_audio_rtp_info (CAudioProfile *pConfig,
			      MediaType *audioFrameType,
			      uint32_t *audioTimeScale,
			      uint8_t *audioPayloadNumber,
			      uint8_t *audioPayloadBytesPerPacket,
			      uint8_t *audioPayloadBytesPerFrame,
			      uint8_t *audioQueueMaxCount,
			      uint8_t *audioiovMaxCount,
			      audio_queue_frame_f * audio_queue_frame,
			      audio_set_rtp_payload_f *audio_set_rtp_payload,
			      audio_set_rtp_header_f *audio_set_header,
			      audio_set_rtp_jumbo_frame_f *audio_set_jumbo_frame,
			      void **ud);

typedef uint32_t *(*bitrates_for_samplerate_f)(uint32_t samplerate, uint8_t chans, uint32_t *ret_size);

typedef struct audio_encoder_table_t {
  char *dialog_selection_name;
  char *audio_encoder;
  char *audio_encoding;
  const uint32_t *sample_rates;
  uint32_t num_sample_rates;
  bitrates_for_samplerate_f bitrates_for_samplerate;
  uint32_t max_channels;
  get_gui_options_list_f get_gui_options;
} audio_encoder_table_t;

void InitAudioEncoders(void);

void AddAudioEncoderTable(audio_encoder_table_t *new_table);

extern audio_encoder_table_t **audio_encoder_table;
extern uint32_t audio_encoder_table_size;
extern const uint32_t allSampleRateTable[13];
extern const uint32_t allSampleRateTableSize;

#endif /* __AUDIO_ENCODER_H__ */

