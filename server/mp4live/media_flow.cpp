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
#include "file_mpeg2_source.h"
#include "file_mp4_source.h"
#include "video_sdl_preview.h"
#include "file_mp4_recorder.h"
#include "rtp_transmitter.h"

// Generic Flow

bool CMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
{
	switch (valueName) {
	default:
		return false;
	}
	return true;
}


void CAVMediaFlow::Start(void)
{
	if (m_started || m_pConfig == NULL) {
		return;
	}

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) 
	  && m_videoSource == NULL) {
		const char* sourceType = 
			m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_TYPE);

		if (!strcasecmp(sourceType, VIDEO_SOURCE_V4L)) {
			m_videoSource = new CV4LVideoSource();

		} else if (!strcasecmp(sourceType, FILE_SOURCE_MP4)) {
			m_videoSource = new CMp4FileSource();

		} else if (!strcasecmp(sourceType, FILE_SOURCE_MPEG2)) {
			m_videoSource = new CMpeg2FileSource();
		}
		m_videoSource->SetConfig(m_pConfig);
		m_videoSource->StartThread();
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		if (m_pConfig->IsOneSource() && m_videoSource) {
			m_audioSource = m_videoSource;
		} else {
			const char* sourceType = 
				m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);

			if (!strcasecmp(sourceType, AUDIO_SOURCE_OSS)) {
				SetAudioInput();
				m_audioSource = new COSSAudioSource();

			} else if (!strcasecmp(sourceType, FILE_SOURCE_MP4)) {
				m_audioSource = new CMp4FileSource();

			} else if (!strcasecmp(sourceType, FILE_SOURCE_MPEG2)) {
				m_audioSource = new CMpeg2FileSource();
			}
			m_audioSource->SetConfig(m_pConfig);
			m_audioSource->StartThread();
		}

		m_audioSource->SetVideoSource(m_videoSource);
	}

	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
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

#ifndef NOGUI
	if (m_videoPreview == NULL) {
		m_videoPreview = new CSDLVideoPreview();
		m_videoPreview->SetConfig(m_pConfig);
		m_videoPreview->StartThread();
		m_videoPreview->Start();
		if (m_videoSource) {
			m_videoSource->AddSink(m_videoPreview);
		}
	}
#endif

	if (m_videoSource) {
		m_videoSource->StartVideo();
	}
	if (m_audioSource) {
		m_audioSource->StartAudio();
	}
	if (m_mp4Recorder) {
		m_mp4Recorder->Start();
	}
	if (m_rtpTransmitter) {
		m_rtpTransmitter->Start();
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

	bool oneSource = (m_audioSource == m_videoSource);

	if (m_audioSource) {
		m_audioSource->StopThread();
		delete m_audioSource;
		m_audioSource = NULL;
	}

	if (m_pConfig->IsFileVideoSource()) {
		if (m_videoSource && !oneSource) {
			m_videoSource->StopThread();
			delete m_videoSource;
		}
		m_videoSource = NULL;

#ifndef NOGUI
		if (m_videoPreview) {
			m_videoPreview->StopThread();
			delete m_videoPreview;
			m_videoPreview = NULL;
		}
#endif
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

void CAVMediaFlow::StartVideoPreview(void)
{
	if (m_pConfig == NULL) {
		return;
	}

	if (m_pConfig->IsFileVideoSource()) {
		return;
	}

	if (m_videoSource == NULL) {
		m_videoSource = new CV4LVideoSource();
		m_videoSource->SetConfig(m_pConfig);
		m_videoSource->StartThread();
	}

	if (m_videoPreview == NULL) {
		m_videoPreview = new CSDLVideoPreview();
		m_videoPreview->SetConfig(m_pConfig);
		m_videoPreview->StartThread();

		m_videoSource->AddSink(m_videoPreview);
	}

	m_videoSource->StartVideo();
	m_videoPreview->Start();
}

void CAVMediaFlow::StopVideoPreview(void)
{
	if (m_pConfig->IsFileVideoSource()) {
		return;
	}

	if (m_videoSource) {
		if (!m_started) {
			m_videoSource->StopThread();
			delete m_videoSource;
			m_videoSource = NULL;
		} else {
			m_videoSource->Stop();
		}
	}

	if (m_videoPreview) {
		if (!m_started) {
			m_videoPreview->StopThread();
			delete m_videoPreview;
			m_videoPreview = NULL;
		} else {
			m_videoPreview->Stop();
		}
	}
}

void CAVMediaFlow::SetAudioInput(void)
{
	// if mixer is specified, then user takes responsibility for
	// configuring mixer to set the appropriate input sources
	// this allows multiple inputs to be used, for example

	if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
	  "mix")) {
		return;
	}

	// else set the mixer input source to the one specified

	static char* inputNames[] = SOUND_DEVICE_NAMES;

	char* mixerName = 
		m_pConfig->GetStringValue(CONFIG_AUDIO_MIXER_NAME);

	int mixer = open(mixerName, O_RDONLY);

	if (mixer < 0) {
		error_message("Couldn't open mixer %s", mixerName);
		return;
	}

	u_int8_t i;
	int recmask = 0;

	for (i = 0; i < sizeof(inputNames) / sizeof(char*); i++) {
		if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
		  inputNames[i])) {
			recmask |= (1 << i);
			ioctl(mixer, SOUND_MIXER_WRITE_RECSRC, &recmask);
			break;
		}
	}

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

