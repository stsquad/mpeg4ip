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
#include "audio_oss_source.h"
#include "video_v4l_source.h"
#include "file_mp4_recorder.h"
#include "rtp_transmitter.h"
#include "file_raw_sink.h"
#include "mp4live_common.h"

// Generic Flow

bool CMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
{
	switch (valueName) {
	default:
		return false;
	}
	return true;
}

void CAVMediaFlow::CreateRtpTransmitter (void)
{
  debug_message("Creating Rtp transmitter");
  m_rtpTransmitter = new CRtpTransmitter(m_pConfig);
  m_rtpTransmitter->StartThread();	
}

void CAVMediaFlow::Start(void)
{
	if (m_started || m_pConfig == NULL) {
		return;
	}

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) 
	  && m_videoSource == NULL) {
	  m_videoSource = CreateVideoSource(m_pConfig);
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
	  m_audioSource = CreateAudioSource(m_pConfig, m_videoSource);
	}

	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
		m_mp4Recorder = new CMp4Recorder();
		m_mp4Recorder->SetConfig(m_pConfig);	
		m_mp4Recorder->StartThread();
		AddSink(m_mp4Recorder);
	}

	if (m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
	  CreateRtpTransmitter();
	  if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
	    m_rtpTransmitter->CreateAudioRtpDestination(0,
							m_pConfig->GetStringValue(CONFIG_RTP_AUDIO_DEST_ADDRESS),
							m_pConfig->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT),
							0);
	  }
	  if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
	    m_rtpTransmitter->CreateVideoRtpDestination(0,
							m_pConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS),
							m_pConfig->GetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT),
							0);
	  }
	}

	if (m_pConfig->GetBoolValue(CONFIG_RAW_ENABLE)) {
		m_rawSink = new CRawFileSink();
		m_rawSink->SetConfig(m_pConfig);	
		m_rawSink->StartThread();	
		AddSink(m_rawSink);
	}

	if (m_videoSource && m_videoSource == m_audioSource) {
		m_videoSource->Start();
	} else {
		if (m_videoSource) {
			m_videoSource->StartVideo();
		}
		if (m_audioSource) {
			m_audioSource->StartAudio();
		}
	}
	if (m_mp4Recorder) {
		m_mp4Recorder->Start();
	}
	if (m_rtpTransmitter) {
	  AddSink(m_rtpTransmitter);
	  m_rtpTransmitter->Start();
	}
	if (m_rawSink) {
		m_rawSink->Start();
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
	if (m_rawSink) {
		RemoveSink(m_rawSink);
		m_rawSink->StopThread();
		delete m_rawSink;
		m_rawSink = NULL;
	}

	bool oneSource = (m_audioSource == m_videoSource);

	if (m_audioSource) {
		m_audioSource->StopThread();
		delete m_audioSource;
		m_audioSource = NULL;
	}

	if (!m_pConfig->IsCaptureVideoSource()) {
		if (m_videoSource && !oneSource) {
			m_videoSource->StopThread();
			delete m_videoSource;
		}
		m_videoSource = NULL;

	}

	m_started = false;
}

void CAVMediaFlow::AddSink(CMediaSink* pSink)
{
	if (m_videoSource) {
		m_videoSource->AddSink(pSink);
	}
	if (m_audioSource && m_audioSource != m_videoSource) {
		m_audioSource->AddSink(pSink);
	}
}

void CAVMediaFlow::RemoveSink(CMediaSink* pSink)
{
	if (m_videoSource) {
		m_videoSource->RemoveSink(pSink);
	}
	if (m_audioSource && m_audioSource != m_videoSource) {
		m_audioSource->RemoveSink(pSink);
	}
}


void CAVMediaFlow::SetPictureControls(void)
{
	if (m_videoSource) {
		m_videoSource->SetPictureControls();
	}
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
	CMediaSource* source = NULL;
	if (m_videoSource) {
		source = m_videoSource;
	} else if (m_audioSource) {
		source = m_audioSource;
	}

	switch (valueName) {
	case FLOW_STATUS_DONE: 
		{
		bool done = true;
		if (m_videoSource) {
			done = m_videoSource->IsDone();
		}
		if (m_audioSource) {
			done = (done && m_audioSource->IsDone());
		}
		*(bool*)pValue = done;
		}
		break;
	case FLOW_STATUS_DURATION:
		if (source) {
			*(Duration*)pValue = source->GetElapsedDuration();
		} else {
			*(Duration*)pValue = 0;
		}
		break;
	case FLOW_STATUS_PROGRESS:
		if (source) {
			*(float*)pValue = source->GetProgress();
		} else {
			*(float*)pValue = 0.0;
		}
		break;
	case FLOW_STATUS_VIDEO_ENCODED_FRAMES:
		if (m_videoSource) {
			*(u_int32_t*)pValue = m_videoSource->GetNumEncodedVideoFrames();
		} else {
			*(u_int32_t*)pValue = 0;
		}
		break;
	default:
		return CMediaFlow::GetStatus(valueName, pValue);
	}
	return true;
}

