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

#ifndef __VIDEO_SOURCE_H__
#define __VIDEO_SOURCE_H__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <avcodec.h>			/* ffmpeg */

#include "media_node.h"

#define NTSC_INT_FPS	30
#define PAL_INT_FPS		25
#define SECAM_INT_FPS	25
#define MAX_INT_FPS		NTSC_INT_FPS

class CVideoSource : public CMediaSource {
public:
	CVideoSource() {
		m_capture = false;
		m_preview = false;
		m_videoDevice = -1;
		m_videoMap = NULL;
		m_videoFrameMap = NULL;
		m_divxHandle = 1;
		m_sdlScreen = NULL;
		m_sdlImage = NULL;
		m_maxVopSize = 32 * 1024;
	}

	void StartCapture(void) {
		m_myMsgQueue.send_message(MSG_START_CAPTURE,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	void StopCapture(void) {
		m_myMsgQueue.send_message(MSG_STOP_CAPTURE,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	void StartPreview(void) {
		m_myMsgQueue.send_message(MSG_START_PREVIEW,
			 NULL, 0, m_myMsgQueueSemaphore);
	}

	void StopPreview(void) {
		m_myMsgQueue.send_message(MSG_STOP_PREVIEW,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	static char GetMpeg4VideoFrameType(CMediaFrame* pFrame);

protected:
	static const int MSG_START_CAPTURE	= 1;
	static const int MSG_STOP_CAPTURE	= 2;
	static const int MSG_START_PREVIEW	= 3;
	static const int MSG_STOP_PREVIEW	= 4;

	int ThreadMain(void);

	void DoStartCapture(void);
	void DoStopCapture(void);

	void DoStartPreview(void);
	void DoStopPreview(void);

	bool Init(void);
	bool InitDevice(void);
	void InitSampleFrames(u_int16_t targetFps, u_int16_t rawFps);
	void InitSizes(void);
	bool InitEncoder(void);

	void ProcessVideo(void);

	int8_t AcquireFrame(void);

	inline bool ReleaseFrame(int8_t frameNumber) {
		return (ioctl(m_videoDevice, VIDIOCMCAPTURE, 
			&m_videoFrameMap[frameNumber]) == 0);
	}

	static void memcpy2to1(u_int8_t* dst, u_int16_t* src, u_int32_t dstsize) {
		while (dstsize--) {
			*dst++ = *src++;
		}
	}

protected:
	bool				m_capture;
	bool				m_preview;

	int					m_videoDevice;
	struct video_mbuf	m_videoMbuf;
	void*				m_videoMap;
	struct video_mmap*	m_videoFrameMap;
	int8_t				m_captureHead;
	int8_t				m_encodeHead;

	bool				m_sampleFrames[MAX_INT_FPS];
	u_int32_t			m_rawFrameNumber;
	u_int32_t			m_skippedFrames;
	u_int16_t			m_rawFrameRate;
	Timestamp			m_startTimestamp;
	Duration			m_rawFrameDuration;
	Duration			m_targetFrameDuration;
	u_int16_t			m_frameRateRatio;

	u_int32_t			m_yRawSize;
	u_int32_t			m_uvRawSize;
	u_int32_t			m_yuvSize;
	u_int32_t			m_ySize;
	u_int32_t			m_uvSize;
	u_int32_t			m_yOffset;
	u_int32_t			m_uvOffset;

	u_int32_t			m_maxVopSize;
	u_int8_t*			m_prevVopBuf;
	u_int32_t			m_prevVopBufLength;
	Timestamp			m_prevVopTimestamp;

	u_int32_t			m_divxHandle;

	AVEncodeContext		m_avctx;

	SDL_Surface*		m_sdlScreen;
	SDL_Rect			m_sdlScreenRect;
	SDL_Overlay*		m_sdlImage;
};

class CVideoCapabilities {
public:
	CVideoCapabilities(char* deviceName) {
		m_deviceName = stralloc(deviceName);
		m_driverName = NULL;
		m_numInputs = 0;
		m_inputNames = NULL;
		m_inputSignalTypes = NULL;
		m_inputHasTuners = NULL;
		m_inputTunerSignalTypes = NULL;

		ProbeDevice();
	}

	~CVideoCapabilities() {
		free(m_deviceName);
		free(m_driverName);
		for (int i = 0; i < m_numInputs; i++) {
			free(m_inputNames[i]);
		}
		free(m_inputNames);
		free(m_inputSignalTypes);
		free(m_inputHasTuners);
		free(m_inputTunerSignalTypes);
	}

	inline bool IsValid() {
		return m_canOpen && m_canCapture;
	}

public:
	char*		m_deviceName; 
	bool		m_canOpen;
	bool		m_canCapture;

	// N.B. the rest of the fields are only valid 
	// if m_canOpen and m_canCapture are both true

	char*		m_driverName; 
	u_int16_t	m_numInputs;

	u_int16_t	m_minWidth;
	u_int16_t	m_minHeight;
	u_int16_t	m_maxWidth;
	u_int16_t	m_maxHeight;

	// each of these points at m_numInputs values
	char**		m_inputNames;
	u_int8_t*	m_inputSignalTypes;			// current signal type of input
	bool*		m_inputHasTuners;
	u_int8_t*	m_inputTunerSignalTypes;	// possible signal types from tuner

protected:
	bool ProbeDevice(void);
};

#endif /* __VIDEO_SOURCE_H__ */
