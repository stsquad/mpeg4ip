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

#define CONFIG_SAFETY 0		// go fast, live dangerously
#include "config_set.h"

#include "timestamp.h"
#include "tv_frequencies.h"

#define AUDIO_ENCODER_FAAC		"faac"
#define AUDIO_ENCODER_LAME		"lame"

#define VIDEO_ENCODER_FFMPEG	"ffmpeg"
#define VIDEO_ENCODER_DIVX		"divx"
#define VIDEO_ENCODER_XVID		"xvid"

#define VIDEO_STD_ASPECT_RATIO 	((float)1.33)	// standard 4:3
#define VIDEO_LB1_ASPECT_RATIO 	((float)2.35)	// typical "widescreen" format
#define VIDEO_LB2_ASPECT_RATIO 	((float)1.85)	// alternate widescreen format
#define VIDEO_LB3_ASPECT_RATIO 	((float)1.78)	// hdtv 16:9

// forward declarations
class CVideoCapabilities;
class CLiveConfig;

// some configuration utility routines
void CalculateVideoFrameSize(CLiveConfig* pConfig);
void GenerateMpeg4VideoConfig(CLiveConfig* pConfig);
char* BinaryToAscii(u_int8_t* buf, u_int32_t bufSize);
bool GenerateSdpFile(CLiveConfig* pConfig);

enum {
	CONFIG_APP_USE_REAL_TIME,
	CONFIG_APP_DURATION,
	CONFIG_APP_DURATION_UNITS,

	CONFIG_AUDIO_ENABLE,
	CONFIG_AUDIO_DEVICE_NAME,
	CONFIG_AUDIO_MIXER_NAME,
	CONFIG_AUDIO_INPUT_NAME,
	CONFIG_AUDIO_CHANNELS,
	CONFIG_AUDIO_SAMPLE_RATE,
	CONFIG_AUDIO_BIT_RATE,
	CONFIG_AUDIO_ENCODER,

	CONFIG_VIDEO_ENABLE,
	CONFIG_VIDEO_DEVICE_NAME,
	CONFIG_VIDEO_INPUT,
	CONFIG_VIDEO_SIGNAL,
	CONFIG_VIDEO_TUNER,
	CONFIG_VIDEO_CHANNEL_LIST_INDEX,
	CONFIG_VIDEO_CHANNEL_INDEX,
	CONFIG_VIDEO_PREVIEW,
	CONFIG_VIDEO_RAW_PREVIEW,
	CONFIG_VIDEO_ENCODED_PREVIEW,
	CONFIG_VIDEO_ENCODER,
	CONFIG_VIDEO_RAW_WIDTH,
	CONFIG_VIDEO_RAW_HEIGHT,
	CONFIG_VIDEO_ASPECT_RATIO,
	CONFIG_VIDEO_FRAME_RATE,
	CONFIG_VIDEO_BIT_RATE,
	CONFIG_VIDEO_PROFILE_ID,
	CONFIG_VIDEO_PROFILE_LEVEL_ID,

	CONFIG_RECORD_ENABLE,
	CONFIG_RECORD_RAW_AUDIO,
	CONFIG_RECORD_RAW_VIDEO,
	CONFIG_RECORD_ENCODED_AUDIO,
	CONFIG_RECORD_ENCODED_VIDEO,
	CONFIG_RECORD_MP4_FILE_NAME,
	CONFIG_RECORD_MP4_HINT_TRACKS,

	CONFIG_RTP_ENABLE,
	CONFIG_RTP_DEST_ADDRESS,
	CONFIG_RTP_AUDIO_DEST_PORT,
	CONFIG_RTP_VIDEO_DEST_PORT,
	CONFIG_RTP_PAYLOAD_SIZE,
	CONFIG_RTP_MCAST_TTL,
	CONFIG_RTP_DISABLE_TS_OFFSET,
	CONFIG_RTP_USE_SSM,
	CONFIG_SDP_FILE_NAME,

	CONFIG_TRANSCODE_ENABLE,
	CONFIG_TRANSCODE_SRC_FILE_NAME,
	CONFIG_TRANSCODE_DST_FILE_NAME,
	CONFIG_TRANSCODE_SRC_AUDIO_ENCODING,
	CONFIG_TRANSCODE_DST_AUDIO_ENCODING,
	CONFIG_TRANSCODE_SRC_VIDEO_ENCODING,
	CONFIG_TRANSCODE_DST_VIDEO_ENCODING,
};

// normally this would be in a .cpp file
// but we have it here to make it easier to keep
// the enumerator list and the variables in sync

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable MyConfigVariables[] = {
	// APP

	{ CONFIG_APP_USE_REAL_TIME, "useKernelRealTimeExtensions", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_APP_DURATION, "duration", 
		CONFIG_TYPE_INTEGER, (config_integer_t)1, },

	{ CONFIG_APP_DURATION_UNITS, "durationUnits", 
		CONFIG_TYPE_INTEGER, (config_integer_t)60, },

	// AUDIO

	{ CONFIG_AUDIO_ENABLE, "audioEnable", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_AUDIO_DEVICE_NAME, "audioDevice", 
		CONFIG_TYPE_STRING, "/dev/dsp", },

	{ CONFIG_AUDIO_MIXER_NAME, "audioMixer", 
		CONFIG_TYPE_STRING, "/dev/mixer", },

	{ CONFIG_AUDIO_INPUT_NAME, "audioInput", 
		CONFIG_TYPE_STRING, "line", },

	{ CONFIG_AUDIO_CHANNELS, "audioChannels", 
		CONFIG_TYPE_INTEGER, (config_integer_t)2, },

	{ CONFIG_AUDIO_SAMPLE_RATE, "audioSampleRate", 
		CONFIG_TYPE_INTEGER, (config_integer_t)44100, },

	{ CONFIG_AUDIO_BIT_RATE, "audioBitRate", 
		CONFIG_TYPE_INTEGER, (config_integer_t)128, },

	{ CONFIG_VIDEO_ENCODER, "audioEncoder",
		CONFIG_TYPE_STRING, "lame", },

	// VIDEO

	{ CONFIG_VIDEO_ENABLE, "videoEnable", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_VIDEO_DEVICE_NAME, "videoDevice", 
		CONFIG_TYPE_STRING, "/dev/video0", },

	{ CONFIG_VIDEO_INPUT, "videoInput",
		CONFIG_TYPE_INTEGER, (config_integer_t)1, },

	{ CONFIG_VIDEO_SIGNAL, "videoSignal",
		CONFIG_TYPE_INTEGER, (config_integer_t)VIDEO_MODE_NTSC, },

	{ CONFIG_VIDEO_TUNER, "videoTuner",
		CONFIG_TYPE_INTEGER, (config_integer_t)-1, },

	{ CONFIG_VIDEO_CHANNEL_LIST_INDEX, "videoChannelListIndex",
		CONFIG_TYPE_INTEGER, (config_integer_t)0, },

	{ CONFIG_VIDEO_CHANNEL_INDEX, "videoChannelIndex",
		CONFIG_TYPE_INTEGER, (config_integer_t)1, },

	{ CONFIG_VIDEO_PREVIEW, "videoPreview",
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_VIDEO_RAW_PREVIEW, "videoRawPreview",
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_VIDEO_ENCODED_PREVIEW, "videoEncodedPreview",
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_VIDEO_ENCODER, "videoEncoder",
		CONFIG_TYPE_STRING, "ffmpeg", },

	{ CONFIG_VIDEO_RAW_WIDTH, "videoRawWidth",
		CONFIG_TYPE_INTEGER, (config_integer_t)320, },

	{ CONFIG_VIDEO_RAW_HEIGHT, "videoRawHeight",
		CONFIG_TYPE_INTEGER, (config_integer_t)240, },

	{ CONFIG_VIDEO_ASPECT_RATIO, "videoAspectRatio",
		CONFIG_TYPE_FLOAT, VIDEO_STD_ASPECT_RATIO },

	{ CONFIG_VIDEO_FRAME_RATE, "videoFrameRate", 
		CONFIG_TYPE_INTEGER, (config_integer_t)15, },

	{ CONFIG_VIDEO_BIT_RATE, "videoBitRate",
		CONFIG_TYPE_INTEGER, (config_integer_t)500, },

	{ CONFIG_VIDEO_PROFILE_ID, "videoProfileId",
		CONFIG_TYPE_INTEGER, (config_integer_t)1, },

	{ CONFIG_VIDEO_PROFILE_LEVEL_ID, "videoProfileLevelId",
		CONFIG_TYPE_INTEGER, (config_integer_t)3, },

	// RECORD

	{ CONFIG_RECORD_ENABLE, "recordEnable", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_RECORD_RAW_AUDIO, "recordRawAudio", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RECORD_RAW_VIDEO, "recordRawVideo", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RECORD_ENCODED_AUDIO, "recordEncodedAudio", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_RECORD_ENCODED_VIDEO, "recordEncodedVideo", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_RECORD_MP4_FILE_NAME, "recordMp4File", 
		CONFIG_TYPE_STRING, "capture.mp4", },

	{ CONFIG_RECORD_MP4_HINT_TRACKS, "recordMp4HintTracks", 
		CONFIG_TYPE_BOOL, true, },

	// RTP

	{ CONFIG_RTP_ENABLE, "rtpEnable", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RTP_DEST_ADDRESS, "rtpDestAddress",
		CONFIG_TYPE_STRING, "224.1.2.3", },

	{ CONFIG_RTP_AUDIO_DEST_PORT, "rtpAudioDestPort",
		CONFIG_TYPE_INTEGER, (config_integer_t)20002, },

	{ CONFIG_RTP_VIDEO_DEST_PORT, "rtpVideoDestPort",
		CONFIG_TYPE_INTEGER, (config_integer_t)20000, },

	{ CONFIG_RTP_PAYLOAD_SIZE, "rtpPayloadSize",
		CONFIG_TYPE_INTEGER, (config_integer_t)1460, },

	{ CONFIG_RTP_MCAST_TTL, "rtpMulticastTtl",
		CONFIG_TYPE_INTEGER, (config_integer_t)15, },

	{ CONFIG_RTP_DISABLE_TS_OFFSET, "rtpDisableTimestampOffset", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RTP_USE_SSM, "rtpUseSingleSourceMulticast", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_SDP_FILE_NAME, "sdpFile", 
		CONFIG_TYPE_STRING, "capture.sdp", },

	// Transcode

	{ CONFIG_TRANSCODE_ENABLE, "transcodeEnable", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_TRANSCODE_SRC_FILE_NAME, "transcodeSrcFile",
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_TRANSCODE_DST_FILE_NAME, "transcodeDstFile",
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_TRANSCODE_SRC_AUDIO_ENCODING, "transcodeSrcAudioEncoding",
		CONFIG_TYPE_STRING, "raw", },

	{ CONFIG_TRANSCODE_DST_AUDIO_ENCODING, "transcodeDstAudioEncoding",
		CONFIG_TYPE_STRING, "aac", },

	{ CONFIG_TRANSCODE_SRC_VIDEO_ENCODING, "transcodeSrcVideoEncoding",
		CONFIG_TYPE_STRING, "raw", },

	{ CONFIG_TRANSCODE_DST_VIDEO_ENCODING, "transcodeDstVideoEncoding",
		CONFIG_TYPE_STRING, "mpeg4", },

};
#endif /* DECLARE_CONFIG_VARIABLES */


class CLiveConfig : public CConfigSet {
public:
	CLiveConfig(SConfigVariable* variables, 
		config_index_t numVariables, const char* defaultFileName);

	// recalculate derived values
	void Update();
	void UpdateVideo();
	void UpdateAudio();
	void UpdateRecord();

public:
	// command line configuration
	bool		m_appAutomatic;

	// derived, shared video configuration
	CVideoCapabilities* m_videoCapabilities;
	bool		m_videoEncode;
	u_int32_t	m_videoPreviewWindowId;
	u_int16_t	m_videoWidth;
	u_int16_t	m_videoHeight;
	u_int16_t	m_videoMaxWidth;
	u_int16_t	m_videoMaxHeight;
	u_int32_t	m_ySize;
	u_int32_t	m_uvSize;
	bool		m_videoNeedRgbToYuv;
	u_int16_t	m_videoMpeg4ConfigLength;
	u_int8_t*	m_videoMpeg4Config;
	u_int32_t	m_videoMaxVopSize;

	// derived, shared audio configuration
	bool		m_audioEncode;
	u_int32_t	m_audioMp3SampleRate;
	u_int16_t	m_audioMp3SamplesPerFrame;

	// derived, shared file configuration
	u_int64_t	m_recordEstFileSize;
};

#endif /* __LIVE_CONFIG_H__ */

