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
 *		Peter Maersk-Moller	peter@maersk-moller.net
 */

#include "mp4live.h"
#include "audio_encoder.h"
#include "mp4av.h"

#ifdef ADD_LAME_ENCODER
#include "audio_lame.h"
#endif

#ifdef ADD_FAAC_ENCODER
#include "audio_faac.h"
#endif
#ifdef HAVE_FFMPEG
#include "audio_ffmpeg.h"
#endif

CAudioEncoder* AudioEncoderBaseCreate(CAudioProfile *ap,
				      CAudioEncoder *next,
				      u_int8_t srcChannels,
				      u_int32_t srcSampleRate,
				      bool realTime)
{
  const char *encoderName = ap->GetStringValue(CFG_AUDIO_ENCODER);
  if (!strcasecmp(encoderName, AUDIO_ENCODER_FAAC)) {
#ifdef ADD_FAAC_ENCODER
    return new CFaacAudioEncoder(ap, next, srcChannels, srcSampleRate, realTime);
#else
    error_message("faac encoder not available in this build");
    return false;
#endif
  } else if (!strcasecmp(encoderName, AUDIO_ENCODER_LAME)) {
#ifdef ADD_LAME_ENCODER
    return new CLameAudioEncoder(ap, next, srcChannels, srcSampleRate, realTime);
#else
    error_message("lame encoder not available in this build");
#endif
  } else if (strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG) == 0) {
#ifdef HAVE_FFMPEG
    return new CFfmpegAudioEncoder(ap, next, srcChannels, srcSampleRate, realTime);
#else
    error_message("ffmpeg audio encoder not available in this build");
#endif
  } else {
    error_message("unknown audio encoder (%s) specified",encoderName);
  }
  return NULL;
}

MediaType get_base_audio_mp4_fileinfo (CAudioProfile *pConfig,
				       bool *mpeg4,
				       bool *isma_compliant,
				       uint8_t *audioProfile,
				       uint8_t **audioConfig,
				       uint32_t *audioConfigLen,
				       uint8_t *mp4_audio_type)
{
  const char *encoderName = pConfig->GetStringValue(CFG_AUDIO_ENCODER);

  if (!strcasecmp(encoderName, AUDIO_ENCODER_FAAC)) {
#ifdef ADD_FAAC_ENCODER
    return faac_mp4_fileinfo(pConfig, mpeg4,
			     isma_compliant, 
			     audioProfile, 
			     audioConfig,
			     audioConfigLen,
			     mp4_audio_type);
#else
    return UNDEFINEDFRAME;
#endif
  } else if (!strcasecmp(encoderName, AUDIO_ENCODER_LAME)) {
#ifdef ADD_LAME_ENCODER
    return lame_mp4_fileinfo(pConfig, mpeg4,
			     isma_compliant, 
			     audioProfile, 
			     audioConfig,
			     audioConfigLen,
			     mp4_audio_type);
#else
    return UNDEFINEDFRAME;
#endif

  } else if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef HAVE_FFMPEG
    return ffmpeg_mp4_fileinfo(pConfig, mpeg4,
			     isma_compliant, 
			     audioProfile, 
			     audioConfig,
			     audioConfigLen,
			     mp4_audio_type);
#else
    return UNDEFINEDFRAME;
#endif

  } else {
    error_message("unknown encoder specified");
  }
  return UNDEFINEDFRAME;
}

media_desc_t *create_base_audio_sdp (CAudioProfile *pConfig,
				bool *mpeg4,
				bool *isma_compliant,
				uint8_t *audioProfile,
				uint8_t **audioConfig,
				uint32_t *audioConfigLen)
{
  const char *encoderName = pConfig->GetStringValue(CFG_AUDIO_ENCODER);

  if (!strcasecmp(encoderName, AUDIO_ENCODER_FAAC)) {
#ifdef ADD_FAAC_ENCODER
    return faac_create_audio_sdp(pConfig, mpeg4,
				 isma_compliant, 
				 audioProfile, 
				 audioConfig,
				 audioConfigLen);
#else
    return NULL;
#endif
  } else if (!strcasecmp(encoderName, AUDIO_ENCODER_LAME)) {
#ifdef ADD_LAME_ENCODER
    return lame_create_audio_sdp(pConfig, mpeg4,
				 isma_compliant, 
				 audioProfile, 
				 audioConfig,
				 audioConfigLen);
#else
    return NULL;
#endif

  } else if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef HAVE_FFMPEG
    return ffmpeg_create_audio_sdp(pConfig, mpeg4,
				   isma_compliant, 
				   audioProfile, 
				   audioConfig,
				   audioConfigLen);
#else
    return NULL;
#endif

  } else {
    error_message("unknown encoder specified");
  }
  return NULL;
}
				
void create_base_mp4_audio_hint_track (CAudioProfile *pConfig, 
				  MP4FileHandle mp4file,
				  MP4TrackId trackId,
				       uint16_t mtu)
{
  const char *encodingName = pConfig->GetStringValue(CFG_AUDIO_ENCODING);

  if (!strcasecmp(encodingName, AUDIO_ENCODING_AAC)) {
    MP4AV_RfcIsmaHinter(mp4file, 
			trackId, 
			false, 
			mtu);
  } else if (!strcasecmp(encodingName, AUDIO_ENCODING_MP3)) {
    MP4AV_Rfc2250Hinter(mp4file,
			trackId,
			false, 
			mtu);
  } else if (!strcasecmp(encodingName, AUDIO_ENCODING_AMR)) {
    MP4AV_Rfc3267Hinter(mp4file, trackId, 
			mtu);
  }
}

bool get_base_audio_rtp_info (CAudioProfile *pConfig,
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
  const char *encoderName = pConfig->GetStringValue(CFG_AUDIO_ENCODER);

  if (!strcasecmp(encoderName, AUDIO_ENCODER_FAAC)) {
#ifdef ADD_FAAC_ENCODER
    return faac_get_audio_rtp_info(pConfig,
				   audioFrameType,
				   audioTimeScale,
				   audioPayloadNumber,
				   audioPayloadBytesPerPacket,
				   audioPayloadBytesPerFrame,
				   audioQueueMaxCount,
				   audio_set_header,
				   audio_set_jumbo,
				   ud);
#else
    return false;
#endif
  } else if (!strcasecmp(encoderName, AUDIO_ENCODER_LAME)) {
#ifdef ADD_LAME_ENCODER
    return lame_get_audio_rtp_info(pConfig,
				   audioFrameType,
				   audioTimeScale,
				   audioPayloadNumber,
				   audioPayloadBytesPerPacket,
				   audioPayloadBytesPerFrame,
				   audioQueueMaxCount,
				   audio_set_header,
				   audio_set_jumbo,
				   ud);
#else
    return false;
#endif
  } else if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef HAVE_FFMPEG
    return ffmpeg_get_audio_rtp_info(pConfig,
				     audioFrameType,
				     audioTimeScale,
				     audioPayloadNumber,
				     audioPayloadBytesPerPacket,
				     audioPayloadBytesPerFrame,
				     audioQueueMaxCount,
				     audio_set_rtp_payload,
				     audio_set_header,
				     audio_set_jumbo,
				     ud);
#else
    return false;
#endif
  } else {
    error_message("unknown encoder specified");
  }
  return false;
  
}

bool CAudioEncoder::InterleaveStereoSamples(
	int16_t* pLeftBuffer, 
	int16_t* pRightBuffer, 
	u_int32_t numSamplesPerChannel,
	int16_t** ppDstBuffer)
{
	if (*ppDstBuffer == NULL) {
		*ppDstBuffer = 
			(int16_t*)malloc(numSamplesPerChannel * 2 * sizeof(int16_t));

		if (*ppDstBuffer == NULL) {
			return false;
		}
	}

	for (u_int32_t i = 0; i < numSamplesPerChannel; i++) {
		(*ppDstBuffer)[(i << 1)] = pLeftBuffer[i]; 
		(*ppDstBuffer)[(i << 1) + 1] = pRightBuffer[i];
	}

	return true;
}

bool CAudioEncoder::DeinterleaveStereoSamples(
	int16_t* pSrcBuffer, 
	u_int32_t numSamplesPerChannel,
	int16_t** ppLeftBuffer, 
	int16_t** ppRightBuffer)
{
	bool mallocedLeft = false;

	if (ppLeftBuffer && *ppLeftBuffer == NULL) {
		*ppLeftBuffer = 
			(int16_t*)malloc(numSamplesPerChannel * sizeof(int16_t));
		if (*ppLeftBuffer == NULL) {
			return false;
		}
		mallocedLeft = true;
	}

	if (ppRightBuffer && *ppRightBuffer == NULL) {
		*ppRightBuffer = 
			(int16_t*)malloc(numSamplesPerChannel * sizeof(int16_t));
		if (*ppRightBuffer == NULL) {
			if (mallocedLeft) {
				free(*ppLeftBuffer);
				*ppLeftBuffer = NULL;
			}
			return false;
		}
	}

	for (u_int32_t i = 0; i < numSamplesPerChannel; i++) {
		if (ppLeftBuffer) {
			(*ppLeftBuffer)[i] = pSrcBuffer[(i << 1)];
		}
		if (ppRightBuffer) {
			(*ppRightBuffer)[i] = pSrcBuffer[(i << 1) + 1];
		}
	}

	return true;
}

audio_encoder_table_t *get_audio_encoder_table_from_dialog_name (const char *name)
{
  for (uint32_t ix = 0; ix < audio_encoder_table_size; ix++) {
    if (strcasecmp(name, audio_encoder_table[ix]->dialog_selection_name) == 0) {
      return audio_encoder_table[ix];
    }
  }
  return NULL;
}

void AddAudioEncoderTable (audio_encoder_table_t *new_table)
{
  audio_encoder_table_size++;
  audio_encoder_table = 
    (audio_encoder_table_t **)realloc(audio_encoder_table,
				     sizeof(audio_encoder_table_t *) *
				     audio_encoder_table_size);
  audio_encoder_table[audio_encoder_table_size - 1] = new_table;

}
