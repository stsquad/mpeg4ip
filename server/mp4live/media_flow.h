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

#ifndef __MEDIA_FLOW_H__
#define __MEDIA_FLOW_H__

#include "live_config.h"
#include "video_source.h"
#include "audio_source.h"
#include "raw_recorder.h"
#include "mp4_recorder.h"
#include "rtp_transmitter.h"

// abstract parent class
class CMediaFlow {
public:
	CMediaFlow(CLiveConfig* pConfig = NULL) {
		m_pConfig = pConfig;
	}

	virtual void Start() = NULL;

	virtual void Stop() = NULL; 

	void SetConfig(CLiveConfig* pConfig) {
		m_pConfig = pConfig;
	}

protected:
	CLiveConfig*		m_pConfig;
};

enum {
	FLOW_STATUS_VIDEO_ENCODED_FRAMES
};

class CAVMediaFlow : public CMediaFlow {
public:
	CAVMediaFlow(CLiveConfig* pConfig = NULL)
		: CMediaFlow(pConfig) {
		m_started = false;
		m_videoSource = NULL;
		m_audioSource = NULL;
		m_rawRecorder = NULL;
		m_mp4Recorder = NULL;
		m_rtpTransmitter = NULL;
	}

	virtual ~CAVMediaFlow() {
		Stop();
	}

	void Start(void);
	void Stop(void);

	void StartVideoPreview(void);
	void StopVideoPreview(void);

	void SetAudioInput(void);
	void SetAudioOutput(bool mute);

	bool GetStatus(u_int32_t valueName, void* pValue);

protected:
	bool				m_started;
	CVideoSource* 		m_videoSource;
	CAudioSource*		m_audioSource;
	CRawRecorder*		m_rawRecorder;
	CMp4Recorder*		m_mp4Recorder;
	CRtpTransmitter*	m_rtpTransmitter;
};

#endif /* __MEDIA_FLOW_H__ */

