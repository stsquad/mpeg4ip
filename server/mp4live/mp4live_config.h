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

#include "media_time.h"
#include "video_util_tv.h"

#define NUM_FILE_HISTORY		8

#define FILE_SOURCE				"FILE"
#define URL_SOURCE				"URL"

#define AUDIO_SOURCE_OSS		"OSS"

#define AUDIO_ENCODER_FAAC		"faac"
#define AUDIO_ENCODER_LAME		"lame"

#define AUDIO_ENCODING_NONE		"None"
#define AUDIO_ENCODING_PCM16	"PCM16"
#define AUDIO_ENCODING_MP3		"MP3"
#define AUDIO_ENCODING_AAC		"AAC"
#define AUDIO_ENCODING_AC3		"AC3"
#define AUDIO_ENCODING_VORBIS	"VORBIS"

#define VIDEO_SOURCE_V4L		"V4L"

#define VIDEO_ENCODER_FFMPEG	"ffmpeg"
#define VIDEO_ENCODER_DIVX		"divx"
#define VIDEO_ENCODER_H26L		"h26l"
#define VIDEO_ENCODER_XVID		"xvid"

#define VIDEO_ENCODING_NONE		"None"
#define VIDEO_ENCODING_YUV12	"YUV12"
#define VIDEO_ENCODING_MPEG2	"MPEG2"
#define VIDEO_ENCODING_MPEG4	"MPEG4"
#define VIDEO_ENCODING_H26L		"H26L"

#define VIDEO_NTSC_FRAME_RATE	((float)29.97)
#define VIDEO_PAL_FRAME_RATE	((float)25.00)

#define VIDEO_STD_ASPECT_RATIO 	((float)1.33)	// standard 4:3
#define VIDEO_LB1_ASPECT_RATIO 	((float)2.35)	// typical "widescreen" format
#define VIDEO_LB2_ASPECT_RATIO 	((float)1.85)	// alternate widescreen format
#define VIDEO_LB3_ASPECT_RATIO 	((float)1.78)	// hdtv 16:9

#define MP3_MPEG1_SAMPLES_PER_FRAME	1152	// for MPEG-1 bitrates
#define MP3_MPEG2_SAMPLES_PER_FRAME	576		// for MPEG-2 bitrates

// forward declarations
class CVideoCapabilities;
class CAudioCapabilities;
class CLiveConfig;

// some configuration utility routines
void GenerateMpeg4VideoConfig(CLiveConfig* pConfig);
bool GenerateSdpFile(CLiveConfig* pConfig);

enum {
	CONFIG_APP_REAL_TIME,
	CONFIG_APP_REAL_TIME_SCHEDULER,
	CONFIG_APP_DURATION,
	CONFIG_APP_DURATION_UNITS,
	CONFIG_APP_FILE_0,
	CONFIG_APP_FILE_1,
	CONFIG_APP_FILE_2,
	CONFIG_APP_FILE_3,
	CONFIG_APP_FILE_4,
	CONFIG_APP_FILE_5,
	CONFIG_APP_FILE_6,
	CONFIG_APP_FILE_7,
	CONFIG_APP_DEBUG,
	CONFIG_APP_LOGLEVEL,

	CONFIG_AUDIO_ENABLE,
	CONFIG_AUDIO_SOURCE_TYPE,
	CONFIG_AUDIO_SOURCE_NAME,
	CONFIG_AUDIO_MIXER_NAME,
	CONFIG_AUDIO_INPUT_NAME,
	CONFIG_AUDIO_SOURCE_TRACK,
	CONFIG_AUDIO_CHANNELS,
	CONFIG_AUDIO_SAMPLE_RATE,
	CONFIG_AUDIO_BIT_RATE,
	CONFIG_AUDIO_ENCODING,
	CONFIG_AUDIO_ENCODER,

	CONFIG_VIDEO_ENABLE,
	CONFIG_VIDEO_SOURCE_TYPE,
	CONFIG_VIDEO_SOURCE_NAME,
	CONFIG_VIDEO_INPUT,
	CONFIG_VIDEO_SIGNAL,
	CONFIG_VIDEO_TUNER,
	CONFIG_VIDEO_CHANNEL_LIST_INDEX,
	CONFIG_VIDEO_CHANNEL_INDEX,
	CONFIG_VIDEO_SOURCE_TRACK,
	CONFIG_VIDEO_PREVIEW,
	CONFIG_VIDEO_RAW_PREVIEW,
	CONFIG_VIDEO_ENCODED_PREVIEW,
	CONFIG_VIDEO_ENCODER,
	CONFIG_VIDEO_RAW_WIDTH,
	CONFIG_VIDEO_RAW_HEIGHT,
	CONFIG_VIDEO_ASPECT_RATIO,
	CONFIG_VIDEO_FRAME_RATE,
	CONFIG_VIDEO_KEY_FRAME_INTERVAL,
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
	CONFIG_RECORD_MP4_OVERWRITE,
	CONFIG_RECORD_MP4_OPTIMIZE,

	CONFIG_RTP_ENABLE,
	CONFIG_RTP_DEST_ADDRESS,
	CONFIG_RTP_AUDIO_DEST_PORT,
	CONFIG_RTP_VIDEO_DEST_PORT,
	CONFIG_RTP_OVER_RTSP,
	CONFIG_RTP_RECV_BUFFER_TIME,
	CONFIG_RTP_PAYLOAD_SIZE,
	CONFIG_RTP_MCAST_TTL,
	CONFIG_RTP_DISABLE_TS_OFFSET,
	CONFIG_RTP_USE_SSM,
	CONFIG_SDP_FILE_NAME,

	CONFIG_RAW_ENABLE,
	CONFIG_RAW_PCM_FILE_NAME,
	CONFIG_RAW_PCM_FIFO,
	CONFIG_RAW_YUV_FILE_NAME,
	CONFIG_RAW_YUV_FIFO,
};

// normally this would be in a .cpp file
// but we have it here to make it easier to keep
// the enumerator list and the variables in sync

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable MyConfigVariables[] = {
	// APP

	{ CONFIG_APP_REAL_TIME, "isRealTime", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_APP_REAL_TIME_SCHEDULER, "useRealTimeScheduler", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_APP_DURATION, "duration", 
		CONFIG_TYPE_INTEGER, (config_integer_t)1, },

	{ CONFIG_APP_DURATION_UNITS, "durationUnits", 
		CONFIG_TYPE_INTEGER, (config_integer_t)60, },

	{ CONFIG_APP_FILE_0, "file0", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_1, "file1", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_2, "file2", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_3, "file3", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_4, "file4", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_5, "file5", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_6, "file6", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_FILE_7, "file7", 
		CONFIG_TYPE_STRING, "", },

	{ CONFIG_APP_DEBUG, "debug", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_APP_LOGLEVEL, "logLevel", 
		CONFIG_TYPE_INTEGER, (config_integer_t)0, },

	// AUDIO

	{ CONFIG_AUDIO_ENABLE, "audioEnable", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_AUDIO_SOURCE_TYPE, "audioSourceType", 
		CONFIG_TYPE_STRING, AUDIO_SOURCE_OSS, },

	{ CONFIG_AUDIO_SOURCE_NAME, "audioDevice", 
		CONFIG_TYPE_STRING, "/dev/dsp", },

	{ CONFIG_AUDIO_MIXER_NAME, "audioMixer", 
		CONFIG_TYPE_STRING, "/dev/mixer", },

	{ CONFIG_AUDIO_INPUT_NAME, "audioInput", 
		CONFIG_TYPE_STRING, "mix", },

	{ CONFIG_AUDIO_SOURCE_TRACK, "audioSourceTrack", 
		CONFIG_TYPE_INTEGER, (config_integer_t)0, },

	{ CONFIG_AUDIO_CHANNELS, "audioChannels", 
		CONFIG_TYPE_INTEGER, (config_integer_t)2, },

	{ CONFIG_AUDIO_SAMPLE_RATE, "audioSampleRate", 
		CONFIG_TYPE_INTEGER, (config_integer_t)44100, },

	{ CONFIG_AUDIO_BIT_RATE, "audioBitRate", 
		CONFIG_TYPE_INTEGER, (config_integer_t)128, },

	{ CONFIG_AUDIO_ENCODING, "audioEncoding",
		CONFIG_TYPE_STRING, AUDIO_ENCODING_MP3, },

	{ CONFIG_AUDIO_ENCODER, "audioEncoder",
		CONFIG_TYPE_STRING, AUDIO_ENCODER_LAME, },

	// VIDEO

	{ CONFIG_VIDEO_ENABLE, "videoEnable", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_VIDEO_SOURCE_TYPE, "videoSourceType", 
		CONFIG_TYPE_STRING, VIDEO_SOURCE_V4L, },

	{ CONFIG_VIDEO_SOURCE_NAME, "videoDevice", 
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

	{ CONFIG_VIDEO_SOURCE_TRACK, "videoSourceTrack", 
		CONFIG_TYPE_INTEGER, (config_integer_t)0, },

	{ CONFIG_VIDEO_PREVIEW, "videoPreview",
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_VIDEO_RAW_PREVIEW, "videoRawPreview",
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_VIDEO_ENCODED_PREVIEW, "videoEncodedPreview",
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_VIDEO_ENCODER, "videoEncoder",
		CONFIG_TYPE_STRING, VIDEO_ENCODER_XVID, },

	{ CONFIG_VIDEO_RAW_WIDTH, "videoRawWidth",
		CONFIG_TYPE_INTEGER, (config_integer_t)320, },

	{ CONFIG_VIDEO_RAW_HEIGHT, "videoRawHeight",
		CONFIG_TYPE_INTEGER, (config_integer_t)240, },

	{ CONFIG_VIDEO_ASPECT_RATIO, "videoAspectRatio",
		CONFIG_TYPE_FLOAT, VIDEO_STD_ASPECT_RATIO },

	{ CONFIG_VIDEO_FRAME_RATE, "videoFrameRate", 
		CONFIG_TYPE_FLOAT, VIDEO_NTSC_FRAME_RATE, },

	{ CONFIG_VIDEO_KEY_FRAME_INTERVAL, "videoKeyFrameInterval", 
		CONFIG_TYPE_FLOAT, (float)2.0, },

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

	{ CONFIG_RECORD_MP4_OVERWRITE, "recordMp4Overwrite", 
		CONFIG_TYPE_BOOL, true, },

	{ CONFIG_RECORD_MP4_OPTIMIZE, "recordMp4Optimize", 
		CONFIG_TYPE_BOOL, false, },

	// RTP

	{ CONFIG_RTP_ENABLE, "rtpEnable", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RTP_DEST_ADDRESS, "rtpDestAddress",
		CONFIG_TYPE_STRING, "224.1.2.3", },

	{ CONFIG_RTP_AUDIO_DEST_PORT, "rtpAudioDestPort",
		CONFIG_TYPE_INTEGER, (config_integer_t)20002, },

	{ CONFIG_RTP_VIDEO_DEST_PORT, "rtpVideoDestPort",
		CONFIG_TYPE_INTEGER, (config_integer_t)20000, },

	{ CONFIG_RTP_OVER_RTSP, "rtpOverRtsp",
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RTP_RECV_BUFFER_TIME, "rtpRecvBufferTime",
		CONFIG_TYPE_INTEGER, (config_integer_t)2, },

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


	// RAW sink

	{ CONFIG_RAW_ENABLE, "rawEnable", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RAW_PCM_FILE_NAME, "rawAudioFile", 
		CONFIG_TYPE_STRING, "capture.pcm", },

	{ CONFIG_RAW_PCM_FIFO, "rawAudioUseFifo", 
		CONFIG_TYPE_BOOL, false, },

	{ CONFIG_RAW_YUV_FILE_NAME, "rawVideoFile", 
		CONFIG_TYPE_STRING, "capture.yuv", },

	{ CONFIG_RAW_YUV_FIFO, "rawVideoUseFifo", 
		CONFIG_TYPE_BOOL, false, },

};
#endif /* DECLARE_CONFIG_VARIABLES */


class CLiveConfig : public CConfigSet {
public:
	CLiveConfig(SConfigVariable* variables, 
		config_index_t numVariables, const char* defaultFileName);

	~CLiveConfig();

	// recalculate derived values
	void Update();
	void UpdateFileHistory(const char* fileName);
	void UpdateVideo();
	void CalculateVideoFrameSize();
	void UpdateAudio();
	void UpdateRecord();

	bool IsOneSource();
	bool IsCaptureVideoSource();
	bool IsCaptureAudioSource();
	bool IsFileVideoSource();
	bool IsFileAudioSource();

	bool SourceRawVideo() {
		return (GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)
			|| (GetBoolValue(CONFIG_RECORD_ENABLE)
				&& GetBoolValue(CONFIG_RECORD_RAW_VIDEO))
			|| GetBoolValue(CONFIG_RAW_ENABLE));
	}

	bool SourceRawAudio() {
		return (GetBoolValue(CONFIG_RECORD_ENABLE)
				&& GetBoolValue(CONFIG_RECORD_RAW_AUDIO))
			|| GetBoolValue(CONFIG_RAW_ENABLE);
	}

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
	u_int32_t	m_yuvSize;
	bool		m_videoNeedRgbToYuv;
	u_int16_t	m_videoMpeg4ConfigLength;
	u_int8_t*	m_videoMpeg4Config;
	u_int32_t	m_videoMaxVopSize;
	u_int8_t	m_videoTimeIncrBits;

	// derived, shared audio configuration
	CAudioCapabilities* m_audioCapabilities;
	bool		m_audioEncode;

	// derived, shared file configuration
	u_int64_t	m_recordEstFileSize;
};

#endif /* __LIVE_CONFIG_H__ */

