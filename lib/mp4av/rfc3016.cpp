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

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4av_common.h>
#include <mp4av_mpeg4.h>

extern "C" MP4TrackId MP4AV_Rfc3016_HintTrackCreate (MP4FileHandle mp4File,
						     MP4TrackId mediaTrackId)
{
	MP4TrackId hintTrackId =
		MP4AddHintTrack(mp4File, mediaTrackId);

	if (hintTrackId == MP4_INVALID_TRACK_ID) {
		return MP4_INVALID_TRACK_ID;
	}

	u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;

	if (MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
				      "MP4V-ES", &payloadNumber, 0) == false) {
	  MP4DeleteTrack(mp4File, hintTrackId);
	  return MP4_INVALID_TRACK_ID;
	}

	/* get the mpeg4 video configuration */
	u_int8_t* pConfig;
	u_int32_t configSize;
	u_int8_t systemsProfileLevel = 0xFE;

	if (MP4GetTrackESConfiguration(mp4File, mediaTrackId, &pConfig, &configSize) == false) {
	  MP4DeleteTrack(mp4File, hintTrackId);
	  return MP4_INVALID_TRACK_ID;
	}

	if (pConfig) {
		// attempt to get a valid profile-level
		static u_int8_t voshStartCode[4] = { 
			0x00, 0x00, 0x01, MP4AV_MPEG4_VOSH_START 
		};
		if (configSize >= 5 && !memcmp(pConfig, voshStartCode, 4)) {
			systemsProfileLevel = pConfig[4];
		} 
		if (systemsProfileLevel == 0xFE) {
			u_int8_t iodProfileLevel = MP4GetVideoProfileLevel(mp4File);
			if (iodProfileLevel > 0 && iodProfileLevel < 0xFE) {
				systemsProfileLevel = iodProfileLevel;
			} else {
				systemsProfileLevel = 1;
			}
		} 

		/* convert it into ASCII form */
		char* sConfig = MP4BinaryToBase16(pConfig, configSize);
		free(pConfig);
		if (sConfig == NULL) {
		  MP4DeleteTrack(mp4File, hintTrackId);
			return MP4_INVALID_TRACK_ID;
		}

		/* create the appropriate SDP attribute */
		char* sdpBuf = (char*)malloc(strlen(sConfig) + 128);

		if (sdpBuf == NULL) {
		  free(sConfig);
		  MP4DeleteTrack(mp4File, hintTrackId);
		  return MP4_INVALID_TRACK_ID;
		}
		snprintf(sdpBuf,
			 strlen(sConfig) + 128,
			"a=fmtp:%u profile-level-id=%u; config=%s;\015\012",
				payloadNumber,
				systemsProfileLevel,
				sConfig); 
		free(sConfig);

		/* add this to the track's sdp */
		if (MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf) == false) {
		  MP4DeleteTrack(mp4File, hintTrackId);
		  hintTrackId = MP4_INVALID_TRACK_ID;
		}

		free(sdpBuf);
	}
	return hintTrackId;
}
						
extern "C" bool MP4AV_Rfc3016_HintAddSample (
					     MP4FileHandle mp4File,
					     MP4TrackId hintTrackId,
					     MP4SampleId sampleId,
					     uint8_t *pSampleBuffer,
					     uint32_t sampleSize,
					     MP4Duration duration,
					     MP4Duration renderingOffset,
					     bool isSyncSample,
					     uint16_t maxPayloadSize)
{
  bool isBFrame = 
    (MP4AV_Mpeg4GetVopType(pSampleBuffer, sampleSize) == VOP_TYPE_B);

  if (MP4AddRtpVideoHint(mp4File, hintTrackId, isBFrame, renderingOffset) == false)
    return false;

  if (sampleId == 1) {
    if (MP4AddRtpESConfigurationPacket(mp4File, hintTrackId) == false) return false;
  }

  u_int32_t offset = 0;
  u_int32_t remaining = sampleSize;

  // TBD should scan for resync markers (if enabled in ES config)
  // and packetize on those boundaries

  while (remaining) {
    bool isLastPacket = false;
    u_int32_t length;

    if (remaining <= maxPayloadSize) {
      length = remaining;
      isLastPacket = true;
    } else {
      length = maxPayloadSize;
    }

    if (MP4AddRtpPacket(mp4File, hintTrackId, isLastPacket) == false ||
	
	MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
			    offset, length) == false) return false;

    offset += length;
    remaining -= length;
  }

  return MP4WriteRtpHint(mp4File, hintTrackId, duration, isSyncSample);
}

extern "C" bool MP4AV_Rfc3016Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	u_int16_t maxPayloadSize)
{
	u_int32_t numSamples = MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
	u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
	
	if (numSamples == 0 || maxSampleSize == 0) {
		return false;
	}

	MP4TrackId hintTrackId = 
	  MP4AV_Rfc3016_HintTrackCreate(mp4File, mediaTrackId);

	if (hintTrackId == MP4_INVALID_TRACK_ID) {
	  return false;
	}

 	u_int8_t* pSampleBuffer = (u_int8_t*)malloc(maxSampleSize);
	if (pSampleBuffer == NULL) {
	  MP4DeleteTrack(mp4File, hintTrackId);
		return false;
	}

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
		u_int32_t sampleSize = maxSampleSize;
		MP4Timestamp startTime;
		MP4Duration duration;
		MP4Duration renderingOffset;
		bool isSyncSample;

		bool rc = MP4ReadSample(
			mp4File, mediaTrackId, sampleId, 
			&pSampleBuffer, &sampleSize, 
			&startTime, &duration, 
			&renderingOffset, &isSyncSample);

		if (rc == false ||
		    MP4AV_Rfc3016_HintAddSample(mp4File,
						hintTrackId,
						sampleId,
						pSampleBuffer,
						sampleSize,
						duration,
						renderingOffset,
						isSyncSample,
						maxPayloadSize) == false) {
		  MP4DeleteTrack(mp4File, hintTrackId);
		  CHECK_AND_FREE(pSampleBuffer);
		  return false;
		}
	}
	CHECK_AND_FREE(pSampleBuffer);

	return true;
}

extern "C" bool MP4AV_Rfc3016LatmHinter (MP4FileHandle mp4File, 
					 MP4TrackId mediaTrackId, 
					 u_int16_t maxPayloadSize)
{
  u_int32_t numSamples = MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
  u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
  MP4Duration sampleDuration = 
    MP4AV_GetAudioSampleDuration(mp4File, mediaTrackId);

  if (sampleDuration == MP4_INVALID_DURATION) {
    return false;
  }
	
  if (numSamples == 0 || maxSampleSize == 0) {
    return false;
  }
  

  /* get the mpeg4 video configuration */
  u_int8_t* pAudioSpecificConfig;
  u_int32_t AudioSpecificConfigSize;
  
  if (MP4GetTrackESConfiguration(mp4File, mediaTrackId, 
				 &pAudioSpecificConfig, 
				 &AudioSpecificConfigSize) == false) 
    return false;

  if (pAudioSpecificConfig == NULL || 
      AudioSpecificConfigSize == 0) return false;

  uint8_t channels = MP4AV_AacConfigGetChannels(pAudioSpecificConfig);
  uint32_t freq = MP4AV_AacConfigGetSamplingRate(pAudioSpecificConfig);
  uint8_t type = MP4AV_AacConfigGetAudioObjectType(pAudioSpecificConfig);

  uint8_t *pConfig;
  uint32_t configSize;

  MP4AV_LatmGetConfiguration(&pConfig, &configSize,
			     pAudioSpecificConfig, AudioSpecificConfigSize);
  free(pAudioSpecificConfig);

  if (pConfig == NULL || configSize == 0) {
    CHECK_AND_FREE(pConfig);
    return false;
  }

  MP4TrackId hintTrackId =
    MP4AddHintTrack(mp4File, mediaTrackId);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    free(pConfig);
    return false;
  }
  u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;

  char buffer[10];
  if (channels != 1) {
    snprintf(buffer, sizeof(buffer), "%u", channels);
  }
  
		/* convert it into ASCII form */
  char* sConfig = MP4BinaryToBase16(pConfig, configSize);
  free(pConfig);
  if (sConfig == NULL ||
      MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
				"MP4A-LATM", &payloadNumber, 0,
				channels != 1 ? buffer : NULL) == false) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return false;
  }

  uint32_t profile_level;
  // from gpac code
  switch (type) {
  case 2:
    if (channels <= 2) profile_level = freq <= 24000 ? 0x28 : 0x29;
    else profile_level = freq <= 48000 ? 0x2a : 0x2b;
    break;
  case 5:
    if (channels <= 2) profile_level = freq < 24000 ? 0x2c : 0x2d;
    else profile_level = freq <= 48000 ? 0x2e : 0x2f;
    break;
  default:
    if (channels <= 2) profile_level = freq < 24000 ? 0x0e : 0x0f;
    else profile_level = 0x10;
    break;
  }
			 
  /* create the appropriate SDP attribute */
  char* sdpBuf = (char*)malloc(strlen(sConfig) + 128);

  if (sdpBuf == NULL) {
    free(sConfig);
    MP4DeleteTrack(mp4File, hintTrackId);
    return false;
  }
  snprintf(sdpBuf,
	  strlen(sConfig) + 128,
	  "a=fmtp:%u profile-level-id=%u; cpresent=0; config=%s;\015\012",
	  payloadNumber,
	  profile_level,
	  sConfig); 

  /* add this to the track's sdp */
  bool val = MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);

  free(sConfig);
  free(sdpBuf);
  if (val == false) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return false;
  }

  for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
    uint8_t buffer[32];
    uint32_t offset = 0;
    uint32_t sampleSize = 
      MP4GetSampleSize(mp4File, mediaTrackId, sampleId);
    uint32_t size_left = sampleSize;

    while (size_left > 0) {
      if (size_left > 0xff) {
	size_left -= 0xff;
	buffer[offset] = 0xff;
      } else {
	buffer[offset] = size_left;
	size_left = 0;
      }
      offset++;
    }
    if (MP4AddRtpHint(mp4File, hintTrackId) == false ||
	MP4AddRtpPacket(mp4File, hintTrackId, true) == false ||
	MP4AddRtpImmediateData(mp4File, hintTrackId, 
			       buffer, offset) == false ||
	MP4AddRtpSampleData(mp4File, hintTrackId, 
			    sampleId, 0, sampleSize) == false ||
	MP4WriteRtpHint(mp4File, hintTrackId, sampleDuration) == false) {
      MP4DeleteTrack(mp4File, hintTrackId);
      return false;
    }
  }
  return true;

}

