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
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"

#ifdef HAVE_LINUX_VIDEODEV2_H
#include "video_v4l2_source.h"
#else
#include "video_v4l_source.h"
#endif

#include "audio_oss_source.h"

CLiveConfig::CLiveConfig(
	SConfigVariable* variables, 
	config_index_t numVariables, 
	const char* defaultFileName)
: CConfigSet(variables, numVariables, defaultFileName) 
{
	m_appAutomatic = false;
	m_videoCapabilities = NULL;
	m_videoEncode = true;
	m_videoPreviewWindowId = 0;
	m_videoMaxWidth = 768;
	m_videoMaxHeight = 576;
	m_videoNeedRgbToYuv = false;
	m_videoMpeg4ConfigLength = 0;
	m_videoMpeg4Config = NULL;
	m_videoMaxVopSize = 128 * 1024;
	m_audioCapabilities = NULL;
	m_audioEncode = true;
	m_recordEstFileSize = 0;
}

CLiveConfig::~CLiveConfig()
{
	delete m_videoCapabilities;
	delete m_audioCapabilities;
	CHECK_AND_FREE(m_videoMpeg4Config);
}

// recalculate derived values
void CLiveConfig::Update() 
{
	UpdateVideo();
	UpdateAudio();
	UpdateRecord();
}

void CLiveConfig::UpdateFileHistory(const char* fileName)
{
	u_int8_t i;
	u_int8_t end = NUM_FILE_HISTORY - 1;

	// check if fileName is already in file history list
	for (i = 0; i < end; i++) {
		if (!strcmp(fileName, GetStringValue(CONFIG_APP_FILE_0 + i))) {
			end = i; 
		}
	}

	// move all entries down 1 position
	for (i = end; i > 0; i--) {
		SetStringValue(CONFIG_APP_FILE_0 + i, 
			GetStringValue(CONFIG_APP_FILE_0 + i - 1));
	}

	// put new value in first position
	SetStringValue(CONFIG_APP_FILE_0, fileName);
}

void CLiveConfig::UpdateVideo() 
{
	m_videoEncode =
		GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)
		|| GetBoolValue(CONFIG_RTP_ENABLE)
		|| (GetBoolValue(CONFIG_RECORD_ENABLE)
			&& GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO));

	CalculateVideoFrameSize();

	GenerateMpeg4VideoConfig(this);
}

void CLiveConfig::CalculateVideoFrameSize()
{
	u_int16_t frameHeight;
	float aspectRatio = GetFloatValue(CONFIG_VIDEO_ASPECT_RATIO);

	// crop video to appropriate aspect ratio modulo 16 pixels
	if ((aspectRatio - VIDEO_STD_ASPECT_RATIO) < 0.1) {
		frameHeight = GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	} else {
		frameHeight = (u_int16_t)(
			(float)GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH) 
			/ aspectRatio);

		if ((frameHeight % 16) != 0) {
			frameHeight += 16 - (frameHeight % 16);
		}

		if (frameHeight > GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT)) {
			// OPTION might be better to insert black lines 
			// to pad image but for now we crop down
			frameHeight = GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
			if ((frameHeight % 16) != 0) {
				frameHeight -= (frameHeight % 16);
			}
		}
	}

	m_videoWidth = GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
	m_videoHeight = frameHeight;

	m_ySize = m_videoWidth * m_videoHeight;
	m_uvSize = m_ySize / 4;
	m_yuvSize = (m_ySize * 3) / 2;

	UpdateVideoProfile();
}
void CLiveConfig::UpdateVideoProfile (void)
{
	uint32_t widthMB, heightMB;
	uint32_t bitrate;
       
	if (GetBoolValue(CONFIG_VIDEO_FORCE_PROFILE_ID)) {
	  m_videoMpeg4ProfileId = GetIntegerValue(CONFIG_VIDEO_PROFILE_ID);
	  return;
	}
	bitrate = GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000;

	widthMB = (m_videoWidth + 15) / 16;
	heightMB = (m_videoHeight + 15) / 16;
       
	float frame_rate = GetFloatValue(CONFIG_VIDEO_FRAME_RATE);

	frame_rate *= widthMB;
	frame_rate *= heightMB;
	
	uint32_t mbpSec = (uint32_t)ceil(frame_rate);
	uint16_t profile;
	if (bitrate <= 65536 && mbpSec < 1485) {
	  // profile is SP0, or SP1  SP0 is more accurate
	  profile = MPEG4_SP_L0;
	} else if (bitrate <= 131072 && mbpSec < 5940) {
	  // profile is SP2
	  profile = MPEG4_SP_L2;
	} else if (bitrate < 393216 && mbpSec < 11880) {
	  // profile is SP3
	  profile = MPEG4_SP_L3;
	} else if (bitrate < 131072 && mbpSec < 2970) {
	  // profile is ASP 0/1 - subsumed3 by SP2
	  profile = MPEG4_ASP_L0;
	} else if (bitrate < 393216 && mbpSec < 5940) {
	  // profile is ASP 2 - subsumed by SP3 
	  profile = MPEG4_ASP_L2;
	} else if (bitrate < 786432 && mbpSec < 11880) {
	  // profile is ASP 3
	  profile = MPEG4_ASP_L3;
	} else if (bitrate < 1536000 && mbpSec < 11880) {
	  // profile is ASP3b
	  profile = MPEG4_ASP_L3B;
	} else if (bitrate < 3000 * 1024 && mbpSec < 23760) {
	  // profile is ASP4
	  profile = MPEG4_ASP_L4;
	} else {
	  // profile is ASP5 - but may be higher
	  if (bitrate > 8000 * 1024 || mbpSec > 16384) {
	    error_message("Video statistics surpass ASP Level 5 - bit rate %u MpbSec %u", 
			  bitrate, mbpSec);
	  }
	  profile = MPEG4_ASP_L4;
	}
	m_videoMpeg4ProfileId = profile;
}

void CLiveConfig::UpdateAudio() 
{
	m_audioEncode =
		GetBoolValue(CONFIG_RTP_ENABLE)
		|| (GetBoolValue(CONFIG_RECORD_ENABLE)
			&& GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO));
}

void CLiveConfig::UpdateRecord() 
{
	u_int64_t videoBytesPerSec = 0;

	if (GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		if (GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {
			videoBytesPerSec += (u_int64_t)
				(((m_videoWidth * m_videoHeight * 3) / 2)
				* GetFloatValue(CONFIG_VIDEO_FRAME_RATE) + 0.5);
		}
		if (GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {
			videoBytesPerSec +=
				(GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000) / 8;
		}
	}

	u_int64_t audioBytesPerSec = 0;

	if (GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		if (GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {
			audioBytesPerSec +=
				GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE) 
				* GetIntegerValue(CONFIG_AUDIO_CHANNELS)
				* sizeof(u_int16_t);
		}
		if (GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {
			audioBytesPerSec +=
				(GetIntegerValue(CONFIG_AUDIO_BIT_RATE)) / 8;
		}
	}

	u_int64_t duration = GetIntegerValue(CONFIG_APP_DURATION) 
		* GetIntegerValue(CONFIG_APP_DURATION_UNITS);

	m_recordEstFileSize = (u_int64_t)videoBytesPerSec + audioBytesPerSec;
	m_recordEstFileSize *= duration;
	m_recordEstFileSize *= 1025;
	m_recordEstFileSize /= 1000;
}

bool CLiveConfig::IsOneSource()
{
	bool sameSourceType =
		!strcasecmp(GetStringValue(CONFIG_VIDEO_SOURCE_TYPE),
			GetStringValue(CONFIG_AUDIO_SOURCE_TYPE));

	if (!sameSourceType) {
		return false;
	}

	bool sameSourceName =
		!strcmp(GetStringValue(CONFIG_VIDEO_SOURCE_NAME),
			GetStringValue(CONFIG_AUDIO_SOURCE_NAME));

	return sameSourceName;
}

bool CLiveConfig::IsCaptureVideoSource()
{
	const char *sourceType =
		GetStringValue(CONFIG_VIDEO_SOURCE_TYPE);

	return !strcasecmp(sourceType, VIDEO_SOURCE_V4L);
}

bool CLiveConfig::IsCaptureAudioSource()
{
	const char *sourceType =
		GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);

	return !strcasecmp(sourceType, AUDIO_SOURCE_OSS);
}

bool CLiveConfig::IsFileVideoSource()
{
	const char *sourceType =
		GetStringValue(CONFIG_VIDEO_SOURCE_TYPE);

	return !strcasecmp(sourceType, FILE_SOURCE);
}

bool CLiveConfig::IsFileAudioSource()
{
	const char *sourceType =
		GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);

	return !strcasecmp(sourceType, FILE_SOURCE);
}

