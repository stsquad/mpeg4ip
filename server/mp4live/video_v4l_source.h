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

#ifndef __VIDEO_V4L_SOURCE_H__
#define __VIDEO_V4L_SOURCE_H__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>

#include "media_source.h"
#include "video_encoder.h"

void CalculateVideoFrameSize(CLiveConfig* pConfig);

class CV4LVideoSource : public CMediaSource {
public:
	CV4LVideoSource() : CMediaSource() {
		m_videoDevice = -1;
		m_videoMap = NULL;
		m_videoFrameMap = NULL;
	}

	static bool InitialVideoProbe(CLiveConfig* pConfig);

	bool IsDone() {
		return false;	// live capture is inexhaustible
	}

	float GetProgress() {
		return 0.0;		// live capture device is inexhaustible
	}

protected:
	int ThreadMain(void);

	void DoStartCapture(void);
	void DoStopCapture(void);

	bool Init(void);

	bool InitDevice(void);
	void ReleaseDevice(void);

	void ProcessVideo(void);

	int8_t AcquireFrame(void);

	inline bool ReleaseFrame(int8_t frameNumber) {
		return (ioctl(m_videoDevice, VIDIOCMCAPTURE, 
			&m_videoFrameMap[frameNumber]) == 0);
	}

	void SetVideoAudioMute(bool mute);

protected:
	u_int8_t			m_maxPasses;

	int					m_videoDevice;
	struct video_mbuf	m_videoMbuf;
	void*				m_videoMap;
	struct video_mmap*	m_videoFrameMap;
	int8_t				m_captureHead;
	int8_t				m_encodeHead;

	float				m_videoSrcFrameRate;
	Duration			m_videoSrcFrameDuration;
};

class CVideoCapabilities {
public:
	CVideoCapabilities(char* deviceName) {
		m_deviceName = stralloc(deviceName);
		m_canOpen = false;
		m_canCapture = false;
		m_driverName = NULL;
		m_numInputs = 0;
		m_inputNames = NULL;
		m_inputSignalTypes = NULL;
		m_inputHasTuners = NULL;
		m_inputTunerSignalTypes = NULL;
		m_hasAudio = false;

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

	bool		m_hasAudio;

protected:
	bool ProbeDevice(void);
};

#endif /* __VIDEO_V4L_SOURCE_H__ */
