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

#include "mp4live.h"
#include "media_flow.h"

// Generic Flow

bool CMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
{
	switch (valueName) {
	case FLOW_STATUS_STARTED:
		*(u_int32_t*)pValue = m_started;
		break;
	default:
		return false;
	}
	return true;
}

// Live Flow

void CAVLiveMediaFlow::Start(void)
{
	if (m_started || m_pConfig == NULL) {
		return;
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		SetAudioInput();
		m_audioSource = new CAudioSource();
		m_audioSource->SetConfig(m_pConfig);
		m_audioSource->StartThread();
		m_audioSource->StartCapture();
	}

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) && m_videoSource == NULL) {
		m_videoSource = new CVideoSource();
		m_videoSource->SetConfig(m_pConfig);
		m_videoSource->StartThread();
		m_videoSource->StartCapture();
	}

#ifndef NOGUI
	if (m_videoPreview == NULL) {
		m_videoPreview = new CVideoPreview();
		m_videoPreview->SetConfig(m_pConfig);
		m_videoPreview->StartThread();
		if (m_videoSource) {
			m_videoSource->AddSink(m_videoPreview);
		}
	}
	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
		m_videoPreview->StartPreview();
	}
#endif

	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
#ifdef DVLP
		if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW)) {
			m_rawRecorder = new CRawRecorder();
			m_rawRecorder->SetConfig(m_pConfig);	
			m_rawRecorder->StartThread();
			AddSink(m_rawRecorder);
		}
#endif /* DVLP */

		m_mp4Recorder = new CMp4Recorder();
		m_mp4Recorder->SetConfig(m_pConfig);	
		m_mp4Recorder->StartThread();
		AddSink(m_mp4Recorder);
	}

	if (m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
		m_rtpTransmitter = new CRtpTransmitter();
		m_rtpTransmitter->SetConfig(m_pConfig);	
		m_rtpTransmitter->StartThread();	
		AddSink(m_rtpTransmitter);
	}

#ifdef DVLP
	if (m_rawRecorder) {
		m_rawRecorder->StartRecord();
	}
#endif
	if (m_mp4Recorder) {
		m_mp4Recorder->StartRecord();
	}
	if (m_rtpTransmitter) {
		m_rtpTransmitter->StartTransmit();
	}
	
	if (m_videoSource) {
		// force video source to generate a key frame
		// so that sinks can quickly sync up
		m_videoSource->GenerateKeyFrame();
	}

	m_started = true;
}

void CAVLiveMediaFlow::Stop(void)
{
	if (!m_started) {
		return;
	}

#ifdef DVLP
	if (m_rawRecorder) {
		RemoveSink(m_rawRecorder);
		m_rawRecorder->StopThread();
		delete m_rawRecorder;
		m_rawRecorder = NULL;
	}
#endif
	if (m_mp4Recorder) {
		RemoveSink(m_mp4Recorder);
		m_mp4Recorder->StopThread();
		delete m_mp4Recorder;
		m_mp4Recorder = NULL;
	}
	if (m_rtpTransmitter) {
		RemoveSink(m_rtpTransmitter);
		m_rtpTransmitter->StopThread();
		delete m_rtpTransmitter;
		m_rtpTransmitter = NULL;
	}

	if (m_audioSource) {
		m_audioSource->StopThread();
		delete m_audioSource;
		m_audioSource = NULL;
	}
	if (!m_pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
		if (m_videoSource) {
			m_videoSource->StopThread();
			delete m_videoSource;
			m_videoSource = NULL;
		}
		if (m_videoPreview) {
			m_videoPreview->StopThread();
			delete m_videoPreview;
			m_videoPreview = NULL;
		}
	}

	m_started = false;
}

void CAVLiveMediaFlow::AddSink(CMediaSink* pSink)
{
	if (m_audioSource) {
		m_audioSource->AddSink(pSink);
	}
	if (m_videoSource) {
		m_videoSource->AddSink(pSink);
	}
}

void CAVLiveMediaFlow::RemoveSink(CMediaSink* pSink)
{
	if (m_audioSource) {
		m_audioSource->RemoveSink(pSink);
	}
	if (m_videoSource) {
		m_videoSource->RemoveSink(pSink);
	}
}

void CAVLiveMediaFlow::StartVideoPreview(void)
{
	if (m_pConfig == NULL) {
		return;
	}
	if (m_videoSource == NULL) {
		m_videoSource = new CVideoSource();
		m_videoSource->SetConfig(m_pConfig);
		m_videoSource->StartThread();
	}

	if (m_videoPreview == NULL) {
		m_videoPreview = new CVideoPreview();
		m_videoPreview->SetConfig(m_pConfig);
		m_videoPreview->StartThread();

		m_videoSource->AddSink(m_videoPreview);
	}

	m_videoSource->StartCapture();
	m_videoPreview->StartPreview();
}

void CAVLiveMediaFlow::StopVideoPreview(void)
{
	if (m_videoSource) {
		if (!m_started) {
			m_videoSource->StopThread();
			delete m_videoSource;
			m_videoSource = NULL;
		} else {
			m_videoSource->StopCapture();
		}
	}

	if (m_videoPreview) {
		if (!m_started) {
			m_videoPreview->StopThread();
			delete m_videoPreview;
			m_videoPreview = NULL;
		} else {
			m_videoPreview->StopPreview();
		}
	}
}

void CAVLiveMediaFlow::SetAudioInput(void)
{
	char* mixerName = 
		m_pConfig->GetStringValue(CONFIG_AUDIO_MIXER_NAME);

	int mixer = open(mixerName, O_RDONLY);

	if (mixer < 0) {
		error_message("Couldn't open mixer %s", mixerName);
		return;
	}

	int recmask = SOUND_MASK_LINE;
	ioctl(mixer, SOUND_MIXER_WRITE_RECSRC, &recmask);

	close(mixer);
}

void CAVLiveMediaFlow::SetAudioOutput(bool mute)
{
	static int muted = 0;
	static int lastVolume;

	char* mixerName = 
		m_pConfig->GetStringValue(CONFIG_AUDIO_MIXER_NAME);

	int mixer = open(mixerName, O_RDONLY);

	if (mixer < 0) {
		error_message("Couldn't open mixer %s", mixerName);
		return;
	}

	if (mute) {
		ioctl(mixer, SOUND_MIXER_READ_LINE, &lastVolume);
		ioctl(mixer, SOUND_MIXER_WRITE_LINE, &muted);
	} else {
		int newVolume;
		ioctl(mixer, SOUND_MIXER_READ_LINE, &newVolume);
		if (newVolume == 0) {
			ioctl(mixer, SOUND_MIXER_WRITE_LINE, &lastVolume);
		}
	}

	close(mixer);
}

bool CAVLiveMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
{
	switch (valueName) {
	case FLOW_STATUS_VIDEO_ENCODED_FRAMES:
		if (m_videoSource) {
			*(u_int32_t*)pValue = m_videoSource->GetNumEncodedFrames();
		} else {
			*(u_int32_t*)pValue = 0;
		}
		break;
	default:
		return CMediaFlow::GetStatus(valueName, pValue);
	}
	return true;
}


// Transcode Flow

void CAVTranscodeMediaFlow::Start(void)
{
	if (m_started || m_pConfig == NULL) {
		return;
	}

	if (m_transcoder == NULL) {
		m_transcoder = new CTranscoder();
		m_transcoder->SetConfig(m_pConfig);
		m_transcoder->StartThread();
	}

#ifndef NOGUI
	if (m_videoPreview == NULL) {
		m_videoPreview = new CVideoPreview();
		m_videoPreview->SetConfig(m_pConfig);
		m_videoPreview->StartThread();
		if (m_transcoder) {
			m_transcoder->AddSink(m_videoPreview);
		}
	}
	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
		m_videoPreview->StartPreview();
	}
#endif

	m_transcoder->StartTranscode();

	m_started = true;
}

void CAVTranscodeMediaFlow::Stop(void)
{
	if (!m_started) {
		return;
	}

	if (m_transcoder) {
		m_transcoder->StopThread();
		delete m_transcoder;
		m_transcoder = NULL;
	}

#ifndef NOGUI
	if (m_videoPreview) {
		m_videoPreview->StopThread();
		delete m_videoPreview;
		m_videoPreview = NULL;
	}
#endif

	m_started = false;
}

bool CAVTranscodeMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
{
	switch (valueName) {
	case FLOW_STATUS_STARTED:
		if (m_transcoder) {
			*(u_int32_t*)pValue = m_transcoder->GetRunning();
		} else {
			*(u_int32_t*)pValue = false;
		}
		break;
	case FLOW_STATUS_VIDEO_ENCODED_FRAMES:
		if (m_transcoder) {
			*(u_int32_t*)pValue = m_transcoder->GetNumEncodedVideoFrames();
		} else {
			*(u_int32_t*)pValue = 0;
		}
		break;
	case FLOW_STATUS_PROGRESS:
		if (m_transcoder) {
			*(float*)pValue = m_transcoder->GetProgress();
		} else {
			*(float*)pValue = 0;
		}
		break;
	case FLOW_STATUS_EST_SIZE:
		if (m_transcoder) {
			*(u_int64_t*)pValue = m_transcoder->GetEstSize();
		} else {
			*(u_int64_t*)pValue = 0;
		}
		break;
	default:
		return CMediaFlow::GetStatus(valueName, pValue);
	}
	return true;
}

