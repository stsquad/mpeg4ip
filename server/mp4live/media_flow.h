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

#include "mp4live_config.h"
#include "video_source.h"
#include "audio_source.h"
#ifdef DVLP
#include "raw_recorder.h"
#endif
#include "video_preview.h"
#include "mp4_recorder.h"
#include "rtp_transmitter.h"
#include "transcoder.h"

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
	FLOW_STATUS_STARTED,
	FLOW_STATUS_VIDEO_ENCODED_FRAMES,
	FLOW_STATUS_PROGRESS,
	FLOW_STATUS_EST_SIZE,
};

class CAVLiveMediaFlow : public CMediaFlow {
public:
	CAVLiveMediaFlow(CLiveConfig* pConfig = NULL)
		: CMediaFlow(pConfig) {
		m_videoSource = NULL;
		m_audioSource = NULL;
#ifdef DVLP
		m_rawRecorder = NULL;
#endif
		m_videoPreview = NULL;
		m_mp4Recorder = NULL;
		m_rtpTransmitter = NULL;
	}

	virtual ~CAVLiveMediaFlow() {
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
	void AddSink(CMediaSink* pSink);
	void RemoveSink(CMediaSink* pSink);

protected:
	CVideoSource* 		m_videoSource;
	CAudioSource*		m_audioSource;
#ifdef DVLP
	CRawRecorder*		m_rawRecorder;
#endif
	CVideoPreview*		m_videoPreview;
	CMp4Recorder*		m_mp4Recorder;
	CRtpTransmitter*	m_rtpTransmitter;
};

class CAVTranscodeMediaFlow : public CMediaFlow {
public:
	CAVTranscodeMediaFlow(CLiveConfig* pConfig = NULL)
		: CMediaFlow(pConfig) {
		m_transcoder = NULL;
		m_videoPreview = NULL;
	}

	virtual ~CAVTranscodeMediaFlow() {
		Stop();
	}

	void Start(void);
	void Stop(void);

	bool GetStatus(u_int32_t valueName, void* pValue);

protected:
	CTranscoder*		m_transcoder;
	CVideoPreview*		m_videoPreview;
};

#endif /* __MEDIA_FLOW_H__ */

