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
 *		Bill May 		wmay@cisco.com
 */

#ifndef __MEDIA_FLOW_H__
#define __MEDIA_FLOW_H__

#include "mp4live_config.h"
#include "media_source.h"
#include "media_sink.h"

class CRtpTransmitter;
class CMp4Recorder;

// abstract parent class
class CMediaFlow {
public:
	CMediaFlow(CLiveConfig* pConfig = NULL) {
		m_pConfig = pConfig;
		m_started = false;
	}

	virtual void Start() = NULL;

	virtual void Stop() = NULL; 

	void SetConfig(CLiveConfig* pConfig) {
		m_pConfig = pConfig;
	}

	virtual bool GetStatus(u_int32_t valueName, void* pValue);

protected:
	CLiveConfig*		m_pConfig;
	bool				m_started;
};

enum {
	FLOW_STATUS_DONE,
	FLOW_STATUS_DURATION,
	FLOW_STATUS_PROGRESS,
	FLOW_STATUS_VIDEO_ENCODED_FRAMES,
	FLOW_STATUS_FILENAME, 
	FLOW_STATUS_MAX
};

class CAVMediaFlow : public CMediaFlow {
public:
	CAVMediaFlow(CLiveConfig* pConfig = NULL)
		: CMediaFlow(pConfig) {
		m_videoSource = NULL;
		m_audioSource = NULL;
		m_mp4Recorder = NULL;
		m_rtpTransmitter = NULL;
		m_rawSink = NULL;
	}

	virtual ~CAVMediaFlow() {
		Stop();
		if (m_videoSource != NULL) {
		  m_videoSource->StopThread();
		  delete m_videoSource;
		  m_videoSource = NULL;
		}
	}

	void Start(void);
	void Stop(void);

	void SetPictureControls(void);

	void SetAudioOutput(bool mute);

	bool GetStatus(u_int32_t valueName, void* pValue);

protected:
	void AddSink(CMediaSink* pSink);
	void RemoveSink(CMediaSink* pSink);

protected:
	void CreateRtpTransmitter(void);
	CMediaSource* 	m_videoSource;
	CMediaSource*	m_audioSource;

	CMp4Recorder*		m_mp4Recorder;
	CRtpTransmitter*	m_rtpTransmitter;
	CMediaSink*		m_rawSink;
};

#endif /* __MEDIA_FLOW_H__ */

