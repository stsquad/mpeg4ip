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

void CAVMediaFlow::Start(void)
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

	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
		if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW)) {
			m_rawRecorder = new CRawRecorder();
			m_rawRecorder->SetConfig(m_pConfig);	
			m_rawRecorder->StartThread();
			if (m_audioSource) {
				m_audioSource->AddSink(m_rawRecorder);
			}
			if (m_videoSource) {
				m_videoSource->AddSink(m_rawRecorder);
			}
		}
		if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4)) {
			m_mp4Recorder = new CMp4Recorder();
			m_mp4Recorder->SetConfig(m_pConfig);	
			m_mp4Recorder->StartThread();
			if (m_audioSource) {
				m_audioSource->AddSink(m_mp4Recorder);
			}
			if (m_videoSource) {
				m_videoSource->AddSink(m_mp4Recorder);
			}
		}
	}

	if (m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
		m_rtpTransmitter = new CRtpTransmitter();
		m_rtpTransmitter->SetConfig(m_pConfig);	
		m_rtpTransmitter->StartThread();	
		if (m_audioSource) {
			m_audioSource->AddSink(m_rtpTransmitter);
		}
		if (m_videoSource) {
			m_videoSource->AddSink(m_rtpTransmitter);
		}
	}

	if (m_rawRecorder) {
		m_rawRecorder->StartRecord();
	}
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

void CAVMediaFlow::Stop(void)
{
	if (!m_started) {
		return;
	}

	if (m_audioSource) {
		m_audioSource->StopThread();
		delete m_audioSource;
		m_audioSource = NULL;
	}
	if (m_videoSource) {
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
			m_videoSource->RemoveAllSinks();
		} else {
			m_videoSource->StopThread();
			delete m_videoSource;
			m_videoSource = NULL;
		}
	}
	if (m_rawRecorder) {
		m_rawRecorder->StopThread();
		delete m_rawRecorder;
		m_rawRecorder = NULL;
	}
	if (m_mp4Recorder) {
		m_mp4Recorder->StopThread();
		delete m_mp4Recorder;
		m_mp4Recorder = NULL;
	}
	if (m_rtpTransmitter) {
		m_rtpTransmitter->StopThread();
		delete m_rtpTransmitter;
		m_rtpTransmitter = NULL;
	}

	m_started = false;
}

void CAVMediaFlow::StartVideoPreview(void)
{
	if (m_pConfig == NULL) {
		return;
	}
	if (m_videoSource == NULL) {
		m_videoSource = new CVideoSource();
		m_videoSource->SetConfig(m_pConfig);
		m_videoSource->StartThread();
	}

	m_videoSource->StartPreview();
}

void CAVMediaFlow::StopVideoPreview(void)
{
	if (m_videoSource) {
		if (!m_started) {
			m_videoSource->StopThread();
			delete m_videoSource;
			m_videoSource = NULL;
		} else {
			m_videoSource->StopPreview();
		}
	}
}

void CAVMediaFlow::SetAudioInput(void)
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

void CAVMediaFlow::SetAudioOutput(bool mute)
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

bool CAVMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
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
		return false;
	}
	return true;
}
