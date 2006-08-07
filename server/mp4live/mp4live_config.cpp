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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "video_v4l_source.h"
#include "audio_oss_source.h"

CCapabilities *AudioCapabilities = NULL;
CVideoCapabilities *VideoCapabilities = NULL;

CLiveConfig::CLiveConfig(
	SConfigVariable* variables, 
	config_index_t numVariables, 
	const char* defaultFileName)
: CConfigSet(variables, numVariables, defaultFileName) 
{
	m_appAutomatic = false;
	m_videoCapabilities = NULL;
	m_videoPreviewWindowId = 0;
	m_videoMpeg4ConfigLength = 0;
	m_videoMpeg4Config = NULL;
	m_videoMaxVopSize = 128 * 1024;
	m_recordEstFileSize = 0;
	m_audioCapabilities = NULL;
	m_parentConfig = NULL;
}

CLiveConfig::~CLiveConfig()
{
  // we don't do the below any more - we hold on to the capabilities
  // in a global variable
  // delete m_videoCapabilities;
  // delete m_audioCapabilities;
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

	CalculateVideoFrameSize();

	//GenerateMpeg4VideoConfig(this);
}

void CLiveConfig::CalculateVideoFrameSize()
{
  m_videoHeight = GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
  m_videoWidth = GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
  m_ySize = m_videoHeight * m_videoWidth;
  m_uvSize = m_ySize / 4;
  m_yuvSize = (m_ySize * 3) / 2;
}

void CLiveConfig::UpdateAudio() 
{
}

void CLiveConfig::UpdateRecord() 
{
#if 0
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
#endif
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

CCapabilities *FindAudioCapabilitiesByDevice (const char *device)
{
  CCapabilities *ret = AudioCapabilities;
  while (ret != NULL) {
    if (ret->Match(device)) {
      return ret;
    }
    ret = ret->GetNext();
  }
  return NULL;
}

CVideoCapabilities *FindVideoCapabilitiesByDevice (const char *device)
{
  CVideoCapabilities *ret = VideoCapabilities;
  while (ret != NULL) {
    if (ret->Match(device)) {
      return ret;
    }
    ret = (CVideoCapabilities *)ret->GetNext();
  }
  return NULL;
}
