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

#ifndef __MEDIA_FLOW_H__
#define __MEDIA_FLOW_H__

#include "mp4live_config.h"
#include "media_source.h"
#include "media_sink.h"
#include "media_stream.h"

class CRtpTransmitter;
class CMp4Recorder;
class CVideoProfileList;
class CAudioProfileList;
class CVideoEncoder;
class CAudioEncoder;
class CTextSource;

enum {
	FLOW_STATUS_DONE,
	FLOW_STATUS_DURATION,
	FLOW_STATUS_PROGRESS,
	FLOW_STATUS_VIDEO_ENCODED_FRAMES,
	FLOW_STATUS_FILENAME, 
	FLOW_STATUS_MAX
};

class CAVMediaFlow {
public:
        CAVMediaFlow(CLiveConfig* pConfig = NULL);
        
	CLiveConfig *GetConfig(void) { return m_pConfig; };
	virtual ~CAVMediaFlow();

	virtual void Start(bool startvideo = true, bool startaudio = true,
		   bool starttext = true);
	virtual void Stop(void);

	void SetPictureControls(void);

	void SetAudioOutput(bool mute);

	bool GetStatus(u_int32_t valueName, void* pValue);

	CMediaSource* GetAudioSource()
	{
		return m_audioSource;
	}
	
	CMediaSource* GetVideoSource()
	{
		return m_videoSource;
	}
	CTextSource* GetTextSource() {
	  return m_textSource;
	}
	  
	bool ReadStreams(void);
	void ValidateAndUpdateStreams(void);
	bool AddStream(const char *name);
	bool DeleteStream(const char *name, bool remove_file = true);
	void RestartFileRecording(void);
protected:
	//void AddSink(CMediaSink* pSink);
	//void RemoveSink(CMediaSink* pSink);
	void StartStreams(void);
protected:
	CLiveConfig*		m_pConfig;
	bool				m_started;

	CMediaSource* 	m_videoSource;
	CMediaSource*	m_audioSource;
	CTextSource*   m_textSource;
	uint32_t m_maxAudioSamplesPerFrame;
 public:
	CVideoProfileList *m_video_profile_list;
	CAudioProfileList *m_audio_profile_list;
	CTextProfileList  *m_text_profile_list;
	CMediaStreamList *m_stream_list;

	CVideoEncoder *m_video_encoder_list;
	CAudioEncoder *m_audio_encoder_list;
	CTextEncoder *m_text_encoder_list;
 protected:
	CVideoEncoder *FindOrCreateVideoEncoder(CVideoProfile *vp, 
						bool create = true);
	CAudioEncoder *FindOrCreateAudioEncoder(CAudioProfile *ap);
	CTextEncoder  *FindOrCreateTextEncoder(CTextProfile *tp);
	CMp4Recorder*		m_mp4RawRecorder;
	CMediaSink*		m_rawSink;
};

#endif /* __MEDIA_FLOW_H__ */

