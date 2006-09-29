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
 *              Peter Maersk-Moller peter@maersk-moller.net(interlace, sources)
 */

#ifndef __LIVE_CONFIG_H__
#define __LIVE_CONFIG_H__

#include <sys/types.h>
#include <mp4.h>

#include "mpeg4ip_config_set.h"

#include "media_time.h"
#include "video_util_tv.h"

#define NUM_FILE_HISTORY		8

#define FILE_SOURCE				"FILE"
#define URL_SOURCE				"URL"
#ifdef ADD_RTP_SOURCE
#define RTP_SOURCE "RTP"
#endif
#ifdef ADD_DVB_SOURCE
#define DVB_SOURCE "DVB"
#endif

#define AUDIO_SOURCE_OSS		"OSS"
#define AUDIO_SOURCE_ALSA		"ALSA"


#define VIDEO_SOURCE_V4L		"V4L"
#ifdef ADD_RTP_SOURCE
#define AUDIO_SOURCE_RTP "RTP"
#define VIDEO_SOURCE_RTP "RTP"
#define MPEG2_TS 0x1
#define MPEG2_PS 0x2
#define MPEG2_ES 0x3
#endif
#ifdef ADD_DVB_SOURCE
#define VIDEO_SOURCE_DVB "DVB"
#endif


#define VIDEO_ENCODER_FFMPEG	"ffmpeg"
#define VIDEO_ENCODER_DIVX		"divx"
#define VIDEO_ENCODER_H26L		"h26l"
#define VIDEO_ENCODER_XVID		"xvid"
#define VIDEO_ENCODER_H261              "h261"
#define VIDEO_ENCODER_X264              "x264"

#define VIDEO_ENCODING_NONE		"None"
#define VIDEO_ENCODING_YUV12	"YUV12"
#define VIDEO_ENCODING_MPEG2	"MPEG2"
#define VIDEO_ENCODING_MPEG4	"MPEG4"
#define VIDEO_ENCODING_H261     "H261"
#define VIDEO_ENCODING_MPEG2    "MPEG2"
#define VIDEO_ENCODING_H263     "H263"
#define VIDEO_ENCODING_H264     "H264"

#define VIDEO_NTSC_FRAME_RATE	((float)29.97)
#define VIDEO_PAL_FRAME_RATE	((float)25.00)

#define MP3_MPEG1_SAMPLES_PER_FRAME	1152	// for MPEG-1 bitrates
#define MP3_MPEG2_SAMPLES_PER_FRAME	576		// for MPEG-2 bitrates

#define VIDEO_SIGNAL_PAL 0
#define VIDEO_SIGNAL_NTSC 1
#define VIDEO_SIGNAL_SECAM 2
#define VIDEO_SIGNAL_MAX 3

#define VIDEO_FILTER_NONE      "none"
#define VIDEO_FILTER_DEINTERLACE "deinterlace - blend"
#define VIDEO_FILTER_DECIMATE  "deinterlace - decimate"
#define VIDEO_FILTER_FFMPEG_DEINTERLACE_INPLACE "deinterlace - ffmpeg inplace"

// to decide if we overwrite or not
#define FILE_MP4_OVERWRITE 1
#define FILE_MP4_APPEND 0
#define FILE_MP4_CREATE_NEW 2

#define TEXT_SOURCE_TIMED_FILE "timed-file"
#define TEXT_SOURCE_DIALOG "dialog"
#define TEXT_SOURCE_FILE_WITH_DIALOG "file-dialog"

// forward declarations
class CCapabilities;
class CLiveConfig;

// some configuration utility routines

DECLARE_CONFIG(CONFIG_APP_REAL_TIME);
DECLARE_CONFIG(CONFIG_APP_REAL_TIME_SCHEDULER);
DECLARE_CONFIG(CONFIG_APP_DURATION);
DECLARE_CONFIG(CONFIG_APP_DURATION_UNITS);
DECLARE_CONFIG(CONFIG_APP_FILE_0);
DECLARE_CONFIG(CONFIG_APP_FILE_1);
DECLARE_CONFIG(CONFIG_APP_FILE_2);
DECLARE_CONFIG(CONFIG_APP_FILE_3);
DECLARE_CONFIG(CONFIG_APP_FILE_4);
DECLARE_CONFIG(CONFIG_APP_FILE_5);
DECLARE_CONFIG(CONFIG_APP_FILE_6);
DECLARE_CONFIG(CONFIG_APP_FILE_7);
DECLARE_CONFIG(CONFIG_APP_DEBUG);
DECLARE_CONFIG(CONFIG_APP_LOGLEVEL);
DECLARE_CONFIG(CONFIG_APP_SIGNAL_HALT);
DECLARE_CONFIG(CONFIG_APP_SIGNAL_REC_RESTART);
DECLARE_CONFIG(CONFIG_APP_STREAM_DIRECTORY);
DECLARE_CONFIG(CONFIG_APP_PROFILE_DIRECTORY);

DECLARE_CONFIG(CONFIG_VIDEO_SOURCE_TYPE);
DECLARE_CONFIG(CONFIG_VIDEO_SOURCE_NAME);
DECLARE_CONFIG(CONFIG_VIDEO_INPUT);
DECLARE_CONFIG(CONFIG_VIDEO_SIGNAL);
DECLARE_CONFIG(CONFIG_VIDEO_TUNER);
DECLARE_CONFIG(CONFIG_VIDEO_CHANNEL_LIST_INDEX);
DECLARE_CONFIG(CONFIG_VIDEO_CHANNEL_INDEX);
DECLARE_CONFIG(CONFIG_VIDEO_SOURCE_TRACK);

DECLARE_CONFIG(CONFIG_AUDIO_SOURCE_TYPE);
DECLARE_CONFIG(CONFIG_AUDIO_SOURCE_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_MIXER_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_INPUT_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_SOURCE_TRACK);
DECLARE_CONFIG(CONFIG_AUDIO_CHANNELS);
DECLARE_CONFIG(CONFIG_AUDIO_SAMPLE_RATE);
DECLARE_CONFIG(CONFIG_AUDIO_TAKE_STREAM_VALUES);
DECLARE_CONFIG(CONFIG_VIDEO_RAW_WIDTH);
DECLARE_CONFIG(CONFIG_VIDEO_RAW_HEIGHT);


DECLARE_CONFIG(CONFIG_AUDIO_ENABLE);
DECLARE_CONFIG(CONFIG_AUDIO_OSS_USE_SMALL_FRAGS);
DECLARE_CONFIG(CONFIG_AUDIO_OSS_FRAGMENTS);
DECLARE_CONFIG(CONFIG_AUDIO_OSS_FRAG_SIZE);

DECLARE_CONFIG(CONFIG_VIDEO_ENABLE);
DECLARE_CONFIG(CONFIG_VIDEO_PREVIEW);
DECLARE_CONFIG(CONFIG_VIDEO_PREVIEW_STREAM);
DECLARE_CONFIG(CONFIG_VIDEO_BRIGHTNESS);
DECLARE_CONFIG(CONFIG_VIDEO_HUE);
DECLARE_CONFIG(CONFIG_VIDEO_COLOR);
DECLARE_CONFIG(CONFIG_VIDEO_CONTRAST);
DECLARE_CONFIG(CONFIG_V4L_CACHE_TIMESTAMP);
DECLARE_CONFIG(CONFIG_VIDEO_CAP_BUFF_COUNT);
DECLARE_CONFIG(CONFIG_VIDEO_FILTER);

DECLARE_CONFIG(CONFIG_TEXT_ENABLE);
DECLARE_CONFIG(CONFIG_TEXT_SOURCE_TYPE);
DECLARE_CONFIG(CONFIG_TEXT_SOURCE_FILE_NAME);
DECLARE_CONFIG(CONFIG_RECORD_ENABLE);
DECLARE_CONFIG(CONFIG_RECORD_RAW_IN_MP4);
DECLARE_CONFIG(CONFIG_RECORD_RAW_IN_MP4_AUDIO);
DECLARE_CONFIG(CONFIG_RECORD_RAW_IN_MP4_VIDEO);
DECLARE_CONFIG(CONFIG_RECORD_RAW_MP4_FILE_NAME);

DECLARE_CONFIG(CONFIG_RECORD_MP4_HINT_TRACKS);
DECLARE_CONFIG(CONFIG_RECORD_MP4_OVERWRITE);
DECLARE_CONFIG(CONFIG_RECORD_MP4_OPTIMIZE);
DECLARE_CONFIG(CONFIG_RECORD_MP4_FILE_STATUS);
DECLARE_CONFIG(CONFIG_RECORD_MP4_VIDEO_TIMESCALE_USES_AUDIO);
DECLARE_CONFIG(CONFIG_RECORD_MP4_ISMA_COMPLIANT);

DECLARE_CONFIG(CONFIG_RTP_PAYLOAD_SIZE);
DECLARE_CONFIG(CONFIG_RTP_MCAST_TTL);
DECLARE_CONFIG(CONFIG_RTP_DISABLE_TS_OFFSET);
DECLARE_CONFIG(CONFIG_RTP_USE_SSM);
DECLARE_CONFIG(CONFIG_RTP_NO_B_RR_0);

DECLARE_CONFIG(CONFIG_RAW_ENABLE);
DECLARE_CONFIG(CONFIG_RAW_PCM_FILE_NAME);
DECLARE_CONFIG(CONFIG_RAW_PCM_FIFO);
DECLARE_CONFIG(CONFIG_RAW_YUV_FILE_NAME);
DECLARE_CONFIG(CONFIG_RAW_YUV_FIFO);

// normally this would be in a .cpp file
// but we have it here to make it easier to keep
// the enumerator list and the variables in sync

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable MyConfigVariables[] = {
	// APP
  CONFIG_BOOL(CONFIG_APP_REAL_TIME, "isRealTime", true),
  CONFIG_BOOL(CONFIG_APP_REAL_TIME_SCHEDULER, "useRealTimeScheduler", true),
  CONFIG_INT(CONFIG_APP_DURATION, "duration", 1),
  CONFIG_INT(CONFIG_APP_DURATION_UNITS, "durationUnits", 60),

  CONFIG_STRING(CONFIG_APP_FILE_0, "file0", ""),
  CONFIG_STRING(CONFIG_APP_FILE_1, "file1", ""),
  CONFIG_STRING(CONFIG_APP_FILE_2, "file2", ""),
  CONFIG_STRING(CONFIG_APP_FILE_3, "file3", ""),
  CONFIG_STRING(CONFIG_APP_FILE_4, "file4", ""),
  CONFIG_STRING(CONFIG_APP_FILE_5, "file5", ""),
  CONFIG_STRING(CONFIG_APP_FILE_6, "file6", ""),
  CONFIG_STRING(CONFIG_APP_FILE_7, "file7", ""),
  CONFIG_STRING_HELP(CONFIG_APP_STREAM_DIRECTORY, "streamDir", NULL,
		     "Location of Stream Profiles"),
  CONFIG_STRING_HELP(CONFIG_APP_PROFILE_DIRECTORY, "profileDir", NULL,
		     "Location of Audio/Video Profiles"),
  CONFIG_BOOL(CONFIG_APP_DEBUG, "debug", false),
  CONFIG_INT(CONFIG_APP_LOGLEVEL, "logLevel", 0),
  CONFIG_STRING(CONFIG_APP_SIGNAL_HALT, "signalHalt", "sighup"),
  CONFIG_STRING(CONFIG_APP_SIGNAL_REC_RESTART, "signalRestartRecording", "usr1"),
  // AUDIO

  CONFIG_BOOL(CONFIG_AUDIO_ENABLE, "audioEnable", true),
  CONFIG_STRING(CONFIG_AUDIO_SOURCE_TYPE, "audioSourceType", AUDIO_SOURCE_OSS),
  CONFIG_STRING(CONFIG_AUDIO_SOURCE_NAME, "audioDevice", "/dev/dsp"),
  CONFIG_STRING(CONFIG_AUDIO_MIXER_NAME, "audioMixer", "/dev/mixer"),
  CONFIG_STRING(CONFIG_AUDIO_INPUT_NAME, "audioInput", "mix"),

  CONFIG_INT(CONFIG_AUDIO_SOURCE_TRACK, "audioSourceTrack", 0),
  CONFIG_INT(CONFIG_AUDIO_CHANNELS, "audioChannels", 2),
  CONFIG_INT(CONFIG_AUDIO_SAMPLE_RATE, "audioSampleRate", 44100),
  CONFIG_BOOL(CONFIG_AUDIO_TAKE_STREAM_VALUES, "audioTakeStreamValues", true),

  CONFIG_BOOL(CONFIG_AUDIO_OSS_USE_SMALL_FRAGS, "audioOssUseSmallFrags", true),
  CONFIG_INT(CONFIG_AUDIO_OSS_FRAGMENTS, "audioOssFragments", 128),
  CONFIG_INT(CONFIG_AUDIO_OSS_FRAG_SIZE, "audioOssFragSize", 8),


  // VIDEO

  CONFIG_BOOL_HELP(CONFIG_VIDEO_ENABLE, "videoEnable", true, "Enable Video"),
  CONFIG_STRING(CONFIG_VIDEO_SOURCE_TYPE, "videoSourceType", VIDEO_SOURCE_V4L),
  CONFIG_STRING(CONFIG_VIDEO_SOURCE_NAME, "videoDevice", "/dev/video0"),
  CONFIG_INT(CONFIG_VIDEO_INPUT, "videoInput", 1),
  CONFIG_INT(CONFIG_VIDEO_SIGNAL, "videoSignal", VIDEO_SIGNAL_NTSC),

  CONFIG_INT(CONFIG_VIDEO_TUNER, "videoTuner", -1),
  CONFIG_INT(CONFIG_VIDEO_CHANNEL_LIST_INDEX, "videoChannelListIndex", 0),
  CONFIG_INT(CONFIG_VIDEO_CHANNEL_INDEX, "videoChannelIndex", 1),

  CONFIG_INT(CONFIG_VIDEO_SOURCE_TRACK, "videoSourceTrack", 0),

  CONFIG_BOOL(CONFIG_VIDEO_PREVIEW, "videoPreview", false),

  CONFIG_STRING(CONFIG_VIDEO_PREVIEW_STREAM, "videoPreviewStream", NULL),
  CONFIG_INT(CONFIG_VIDEO_RAW_WIDTH, "videoRawWidth", 320),
  CONFIG_INT(CONFIG_VIDEO_RAW_HEIGHT, "videoRawHeight", 240),

  CONFIG_INT(CONFIG_VIDEO_BRIGHTNESS, "videoBrightness", 50),
  CONFIG_INT(CONFIG_VIDEO_HUE, "videoHue", 50),
  CONFIG_INT(CONFIG_VIDEO_COLOR, "videoColor", 50),
  CONFIG_INT(CONFIG_VIDEO_CONTRAST, "videoContrast", 50),

  CONFIG_BOOL(CONFIG_V4L_CACHE_TIMESTAMP, "videoTimestampCache", true),

  CONFIG_INT(CONFIG_VIDEO_CAP_BUFF_COUNT, "videoCaptureBuffersCount", 16),
  CONFIG_STRING(CONFIG_VIDEO_FILTER, "videoFilter", "none"),
  // text
  CONFIG_BOOL(CONFIG_TEXT_ENABLE, "textEnable", false),
  CONFIG_STRING(CONFIG_TEXT_SOURCE_TYPE, "textSource", TEXT_SOURCE_DIALOG),
  CONFIG_STRING(CONFIG_TEXT_SOURCE_FILE_NAME, "textSourceFile", NULL),
  // RECORD
  CONFIG_BOOL(CONFIG_RECORD_ENABLE, "recordEnable", true),
  CONFIG_BOOL(CONFIG_RECORD_RAW_IN_MP4, "recordRawInMp4", false),
  CONFIG_BOOL(CONFIG_RECORD_RAW_IN_MP4_AUDIO, "recordRawMp4Audio", true),
  CONFIG_BOOL(CONFIG_RECORD_RAW_IN_MP4_VIDEO, "recordRawMp4Video", true),

  CONFIG_STRING(CONFIG_RECORD_RAW_MP4_FILE_NAME, "recordRawMp4File", "raw.mp4"),
  CONFIG_BOOL(CONFIG_RECORD_MP4_HINT_TRACKS, "recordMp4HintTracks", true),
  CONFIG_BOOL(CONFIG_RECORD_MP4_OVERWRITE, "recordMp4Overwrite", true),
  CONFIG_BOOL(CONFIG_RECORD_MP4_OPTIMIZE, "recordMp4Optimize", false),
  CONFIG_INT(CONFIG_RECORD_MP4_FILE_STATUS, "recordMp4FileStatus", FILE_MP4_OVERWRITE),
  CONFIG_BOOL_HELP(CONFIG_RECORD_MP4_VIDEO_TIMESCALE_USES_AUDIO,
		   "recordMp4VideoTimescaleUsesAudio", true, 
		   "Set video timescale to same as audio for local playback using quicktime"),
  CONFIG_BOOL_HELP(CONFIG_RECORD_MP4_ISMA_COMPLIANT,
		   "recordMp4IsmaCompliant", false, 
		   "Make ISMA compliant mp4 files - default is false"),

  // RTP
  CONFIG_INT(CONFIG_RTP_PAYLOAD_SIZE, "rtpPayloadSize", 1460),
  CONFIG_INT(CONFIG_RTP_MCAST_TTL, "rtpMulticastTtl", 15),

  CONFIG_BOOL(CONFIG_RTP_DISABLE_TS_OFFSET, "rtpDisableTimestampOffset", false),
  CONFIG_BOOL(CONFIG_RTP_USE_SSM, "rtpUseSingleSourceMulticast", false),

  CONFIG_BOOL(CONFIG_RTP_NO_B_RR_0, "rtpNoBRR0", false),

  // RAW sink

  CONFIG_BOOL(CONFIG_RAW_ENABLE, "rawEnable", false),
  CONFIG_STRING(CONFIG_RAW_PCM_FILE_NAME, "rawAudioFile", "capture.pcm"),
  CONFIG_BOOL(CONFIG_RAW_PCM_FIFO, "rawAudioUseFifo", false),
  
  CONFIG_STRING(CONFIG_RAW_YUV_FILE_NAME, "rawVideoFile", "capture.yuv"),
  CONFIG_BOOL(CONFIG_RAW_YUV_FIFO, "rawVideoUseFifo", false),

};
#endif /* DECLARE_CONFIG_VARIABLES */


class CVideoCapabilities;

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

	CLiveConfig* GetParentConfig()
	{
		return m_parentConfig;
	}
	
	void SetParentConfig(CLiveConfig* parentConfig)
	{
		m_parentConfig=parentConfig;
	}
	
public:
	// command line configuration
	bool		m_appAutomatic;

	// derived, shared video configuration
	CVideoCapabilities* m_videoCapabilities;
#ifdef ADD_RTP_SOURCE
	CCapabilities *m_RTPvideoCapabilities;
#endif
#ifdef ADD_DVB_SOURCE
	CCapabilities *m_DVBvideoCapabilities;
#endif
	u_int32_t	m_videoPreviewWindowId;
	u_int16_t	m_videoWidth;
	u_int16_t	m_videoHeight;
	u_int32_t	m_ySize;
	u_int32_t	m_uvSize;
	u_int32_t	m_yuvSize;
	u_int32_t	m_videoMpeg4ConfigLength;
	u_int8_t*	m_videoMpeg4Config;
	u_int32_t	m_videoMaxVopSize;
	u_int8_t	m_videoTimeIncrBits;

	u_int8_t        m_videoMpeg4ProfileId;
	// derived, shared audio configuration
	CCapabilities* m_audioCapabilities;

	// derived, shared file configuration
	u_int64_t	m_recordEstFileSize;
	
	//Parent Config Pointer
	CLiveConfig* m_parentConfig;
};

class CCapabilities
{
 public:
  CCapabilities (const char *deviceName) {
    m_deviceName = strdup(deviceName);
    m_canOpen = false;
    m_next = NULL;
  };

  virtual ~CCapabilities() {
    CHECK_AND_FREE(m_deviceName);
  };

  virtual bool IsValid() {
    return m_canOpen;
  };
  
  virtual void Display(CLiveConfig *pConfig,
		       char *msg, 
		       uint32_t max_len) = 0;
  virtual u_int32_t CheckSampleRate(u_int32_t rate) {
    return rate;
  };

  const char *m_deviceName;
  bool m_canOpen;
  bool Match(const char *deviceName) {
    return strcmp(deviceName, m_deviceName) == 0;
  }
  void SetNext(CCapabilities *next) {
    m_next = next;
  };
  CCapabilities *GetNext(void) { return m_next; };
 protected:
  virtual bool ProbeDevice(void) = 0;
  CCapabilities *m_next;
};

CCapabilities *FindAudioCapabilitiesByDevice(const char *device);
CVideoCapabilities *FindVideoCapabilitiesByDevice(const char *device);
extern CCapabilities *AudioCapabilities;
extern CVideoCapabilities *VideoCapabilities;

#endif /* __LIVE_CONFG_H__ */

