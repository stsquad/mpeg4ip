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

class CAudioEncoder : public CMediaCodec {
public:
	CAudioEncoder() { };

	virtual u_int32_t GetSamplesPerFrame() = NULL;

	virtual bool EncodeSamples(
		int16_t* pSamples, 
		u_int32_t numSamplesPerChannel,
		u_int8_t numChannels) = NULL;

	virtual bool GetEncodedFrame(
		u_int8_t** ppBuffer, 
		u_int32_t* pBufferLength,
		u_int32_t* pNumSamplesPerChannel) = NULL;


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
};

CAudioEncoder* AudioEncoderCreate(const char* encoderName);
media_desc_t *create_audio_sdp(CLiveConfig *pConfig,
			       bool *mpeg4,
			       bool *isma_compliant,
			       uint8_t *audioProfile,
			       uint8_t **audioConfig,
			       uint32_t *audioConfigLen);

MediaType get_audio_mp4_fileinfo(CLiveConfig *pConfig,
				 bool *mpeg4,
				 bool *isma_compliant,
				 uint8_t *audioProfile,
				 uint8_t **audioConfig,
				 uint32_t *audioConfigLen,
				 uint8_t *mp4_audio_type);

void create_mp4_audio_hint_track(CLiveConfig *pConfig, 
				 MP4FileHandle mp4file,
				 MP4TrackId trackId);

typedef int (*audio_queue_frame_f)(u_int32_t **frameno,
					u_int32_t frameLength,
					u_int8_t audioQueueCount,
					u_int16_t audioQueueSize,
					u_int32_t rtp_payload_size);
typedef int (*audio_set_rtp_payload_f)(CMediaFrame** m_audioQueue,
					int queue_cnt,
					struct iovec *iov,
					void *ud);

typedef bool (*audio_set_rtp_header_f)(struct iovec *iov,
				       int queue_cnt,
				       void *ud);

typedef bool (*audio_set_rtp_jumbo_frame_f)(struct iovec *iov,
					    uint32_t dataOffset,
					    uint32_t bufferLen,
					    uint32_t rtpPayloadMax,
					    bool &mbit,
					    void *ud);

bool get_audio_rtp_info (CLiveConfig *pConfig,
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

CAudioEncoder* AudioEncoderBaseCreate(const char* encoderName);

media_desc_t *create_base_audio_sdp(CLiveConfig *pConfig,
				    bool *mpeg4,
				    bool *isma_compliant,
				    uint8_t *audioProfile,
				    uint8_t **audioConfig,
				    uint32_t *audioConfigLen);

MediaType get_base_audio_mp4_fileinfo(CLiveConfig *pConfig,
				      bool *mpeg4,
				      bool *isma_compliant,
				      uint8_t *audioProfile,
				      uint8_t **audioConfig,
				      uint32_t *audioConfigLen,
				      uint8_t *mp4_audio_type);

void create_base_mp4_audio_hint_track(CLiveConfig *pConfig, 
				      MP4FileHandle mp4file,
				      MP4TrackId trackId);


bool get_base_audio_rtp_info (CLiveConfig *pConfig,
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
} audio_encoder_table_t;

extern audio_encoder_table_t *audio_encoder_table[];
extern const uint32_t audio_encoder_table_size;
extern const uint32_t allSampleRateTable[];
extern const uint32_t allSampleRateTableSize;

#endif /* __AUDIO_ENCODER_H__ */

