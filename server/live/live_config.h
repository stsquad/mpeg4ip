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

#ifndef __LIVE_CONFIG_H__
#define __LIVE_CONFIG_H__

#include <sys/types.h>
#include <linux/videodev.h>

#include "timestamp.h"
#include "tv_frequencies.h"

#define VIDEO_STD_ASPECT_RATIO 	1.33	// standard 4:3
#define VIDEO_LB1_ASPECT_RATIO 	2.35	// typical "widescreen" format
#define VIDEO_LB2_ASPECT_RATIO 	1.85	// approximately 16:9

// forward declarations
class CVideoCapabilities;

// some configuration utility routines
class CLiveConfig;
void GenerateMpeg4VideoConfig(CLiveConfig* pConfig);
char* BinaryToAscii(u_int8_t* buf, u_int32_t bufSize);
bool GenerateSdpFile(CLiveConfig* pConfig);

inline char* stralloc(const char* src) {
	char* dst = (char*)malloc(strlen(src)+1);
	if (dst) {
		strcpy(dst, src);
	}
	return dst;
}

class CLiveConfig {
public:
	CLiveConfig() {
		SetDefaults();
	}

	bool Read(char* fileName) {
		return true; // TEMP
	}
	bool ReadUser() {
		return Read(GetUserFileName());
	}

	bool Write(char* fileName, bool onlyNonDefaults) {
		return true; // TEMP
	}
	bool WriteUser() {
		return Write(GetUserFileName(), true);
	}

	char* GetUserFileName() {
		return "~/.mp4live_rc";
	}

	void SetDefaults(void) {
		m_audioEnable = true;
		m_audioEncode = true;
		m_audioDeviceName = stralloc("/dev/dsp");
		m_audioChannels = 2;
		m_audioSamplingRate = 44100; // Hz
		m_audioTargetBitRate = 128;	// Kbps

		m_videoEnable = true;
		m_videoEncode = true;
		m_videoDeviceName = stralloc("/dev/video0");
		m_videoCapabilities = NULL;
		m_videoInput = 0;
		m_videoSignal = VIDEO_MODE_NTSC;
		m_videoTuner = -1;		
		m_videoChannelListIndex = 0;	// US Broadcast
		m_videoChannelIndex = 1;		// Channel 3
		m_videoPreview = true;
		m_videoRawPreview = false;
		m_videoEncodedPreview = true;
		m_videoPreviewWindowId = 0;
		m_videoUseDivxEncoder = false;
		m_videoRawWidth = 320;
		m_videoRawHeight = 240;
		m_videoWidth = m_videoRawWidth;
		m_videoHeight = m_videoRawHeight;
		m_videoMaxWidth = 352;
		m_videoMaxHeight = 288;
		m_videoTargetFrameRate = 24;
		m_videoTargetBitRate = 750;	// Kbps
		m_videoAspectRatio = VIDEO_STD_ASPECT_RATIO;
		m_videoProfileId = 1;		// Simple Profile
		m_videoProfileLevelId = 3;	// Simple Profile @ L3
		m_videoMpeg4ConfigLength = 0;
		m_videoMpeg4Config = NULL;
		GenerateMpeg4VideoConfig(this);

		m_recordEnable = true;
		m_recordRaw = false;
		m_recordPcmFileName = stralloc("capture.pcm");
		m_recordYuvFileName = stralloc("capture.yuv");
		m_recordMp4 = true;
		m_recordMp4FileName = stralloc("capture.mp4");

		m_rtpEnable = false;
		m_rtpDestAddress = stralloc("224.1.2.3");
		m_rtpAudioDestPort = 32770;
		m_rtpVideoDestPort = 32768;
		m_rtpPayloadSize = 1460;
		m_rtpMulticastTtl = 15;
		m_rtpDisableTimestampOffset = false;
		m_rtpUseSSM = false;
		m_sdpFileName = stralloc("capture.sdp");
	}

public:
	// audio configuration
	bool		m_audioEnable;
	bool		m_audioEncode;
	char*		m_audioDeviceName;
	u_int8_t	m_audioChannels;
	u_int32_t	m_audioSamplingRate;
	u_int16_t	m_audioTargetBitRate;

	// video configuration
	bool		m_videoEnable;
	bool		m_videoEncode;
	char*		m_videoDeviceName;
	CVideoCapabilities* m_videoCapabilities;
	u_int16_t	m_videoInput;
	u_int16_t	m_videoSignal;
	int16_t		m_videoTuner;
	u_int16_t	m_videoChannelListIndex;
	u_int16_t	m_videoChannelIndex;
	bool		m_videoPreview;
	bool		m_videoRawPreview;
	bool		m_videoEncodedPreview;
	u_int32_t	m_videoPreviewWindowId;
	// Note use of SDL means one must choose raw xor encoded preview
	bool		m_videoUseDivxEncoder;
	u_int16_t	m_videoRawWidth;
	u_int16_t	m_videoRawHeight;
	u_int16_t	m_videoWidth;
	u_int16_t	m_videoHeight;
	u_int16_t	m_videoMaxWidth;
	u_int16_t	m_videoMaxHeight;
	u_int16_t	m_videoTargetFrameRate;
	u_int16_t	m_videoTargetBitRate;
	float		m_videoAspectRatio;
	u_int8_t	m_videoProfileId;
	u_int8_t	m_videoProfileLevelId;
	u_int16_t	m_videoMpeg4ConfigLength;
	u_int8_t*	m_videoMpeg4Config;

	// recording configuration
	bool		m_recordEnable;
	bool		m_recordRaw;
	char*		m_recordPcmFileName;
	char*		m_recordYuvFileName;
	bool		m_recordMp4;
	char*		m_recordMp4FileName;

	// transmitting configuration
	bool		m_rtpEnable;
	char*		m_rtpDestAddress;
	u_int16_t	m_rtpAudioDestPort;
	u_int16_t	m_rtpVideoDestPort;
	u_int16_t	m_rtpPayloadSize;
	u_int8_t	m_rtpMulticastTtl;
	bool		m_rtpDisableTimestampOffset;
	bool		m_rtpUseSSM;
	char*		m_sdpFileName;
};

#endif /* __LIVE_CONFIG_H__ */

