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

#include "mp4live.h"
#include "audio_oss_source.h"


int COSSAudioSource::ThreadMain(void) 
{
	while (true) {
		int rc;

		if (m_source) {
			rc = SDL_SemTryWait(m_myMsgQueueSemaphore);
		} else {
			rc = SDL_SemWait(m_myMsgQueueSemaphore);
		}

		// semaphore error
		if (rc == -1) {
			break;
		} 

		// message pending
		if (rc == 0) {
			CMsg* pMsg = m_myMsgQueue.get_message();
		
			if (pMsg != NULL) {
				switch (pMsg->get_value()) {
				case MSG_NODE_STOP_THREAD:
					DoStopCapture();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_NODE_START:
				case MSG_SOURCE_START_AUDIO:
					DoStartCapture();
					break;
				case MSG_NODE_STOP:
					DoStopCapture();
					break;
				}

				delete pMsg;
			}
		}

		if (m_source) {
			try {
				ProcessAudio();
			}
			catch (...) {
				DoStopCapture();	
				break;
			}
		}
	}

	return -1;
}

void COSSAudioSource::DoStartCapture()
{
	if (m_source) {
		return;
	}

	if (!Init()) {
		return;
	}

	m_source = true;
}

void COSSAudioSource::DoStopCapture()
{
	if (!m_source) {
		return;
	}

	CMediaSource::DoStopAudio();

	close(m_audioDevice);
	m_audioDevice = -1;

	free(m_pcmFrameBuffer);
	m_pcmFrameBuffer = NULL;

	m_source = false;
}

bool COSSAudioSource::Init(void)
{
	bool rc = InitAudio(
		true);

	if (!rc) {
		return false;
	}

	rc = SetAudioSrc(
		CMediaFrame::PcmAudioFrame,
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS),
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

	if (!rc) {
		return false;
	}

	if (!InitDevice()) {
		return false;
	}

	// for live capture we can match the source to the destination
	m_audioSrcSamplesPerFrame = m_audioDstSamplesPerFrame;

	m_pcmFrameSize = 
		m_audioSrcSamplesPerFrame * m_audioSrcChannels * sizeof(u_int16_t);

	m_pcmFrameBuffer = (u_int8_t*)malloc(m_pcmFrameSize);

	if (!m_pcmFrameBuffer) {
		goto init_failure;
	}

	// maximum number of passes in ProcessAudio, approx 1 sec.
	m_maxPasses = 
		m_audioSrcSampleRate / m_audioSrcSamplesPerFrame;

	return true;

init_failure:
	debug_message("audio initialization failed");

	free(m_pcmFrameBuffer);
	m_pcmFrameBuffer = NULL;

	close(m_audioDevice);
	m_audioDevice = -1;
	return false;
}

bool COSSAudioSource::InitDevice(void)
{
	int rc;
	char* deviceName = m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME);

	// open the audio device
	m_audioDevice = open(deviceName, O_RDONLY);
	if (m_audioDevice < 0) {
		error_message("Failed to open %s", deviceName);
		return false;
	}

	int format = AFMT_S16_LE;
	rc = ioctl(m_audioDevice, SNDCTL_DSP_SETFMT, &format);
	if (rc < 0 || format != AFMT_S16_LE) {
		error_message("Couldn't set format for %s", deviceName);
		return false;
	}

	u_int32_t channels = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);
	rc = ioctl(m_audioDevice, SNDCTL_DSP_CHANNELS, &channels);
	if (rc < 0 
	  || channels != m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS)) {
		error_message("Couldn't set audio channels for %s", deviceName);
		return false;
	}

	u_int32_t samplingRate = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
	u_int32_t targetSamplingRate = samplingRate;

	rc = ioctl(m_audioDevice, SNDCTL_DSP_SPEED, &samplingRate);

	if (rc < 0 || abs(samplingRate - targetSamplingRate) > 1) {
		error_message("Couldn't set sampling rate for %s", deviceName);
		return false;
	}

	return true;
}

void COSSAudioSource::ProcessAudio(void)
{
	// for efficiency, process 1 second before returning to check for commands
	for (int pass = 0; pass < m_maxPasses; pass++) {

		// read a frame's worth of raw PCM data
		u_int32_t bytesRead = 
			read(m_audioDevice, m_pcmFrameBuffer, m_pcmFrameSize); 

		if (bytesRead < m_pcmFrameSize) {
			debug_message("bad audio read");
			continue;
		}

		Timestamp frameTimestamp;

		if (m_audioSrcFrameNumber == 0) {
			// timestamp needs to reflect the first pcm sample
			frameTimestamp = m_audioStartTimestamp =
				GetTimestamp() - SrcSamplesToTicks(m_audioSrcSamplesPerFrame);
		} else {
			frameTimestamp = m_audioStartTimestamp +
				SrcSamplesToTicks(m_audioSrcSampleNumber);
		}

		ProcessAudioFrame(
			m_pcmFrameBuffer,
			m_pcmFrameSize,
			frameTimestamp,
			false);
	}
}

bool CAudioCapabilities::ProbeDevice()
{
	int rc;

	// open the audio device
	int audioDevice = open(m_deviceName, O_RDONLY);
	if (audioDevice < 0) {
		return false;
	}
	m_canOpen = true;

	// union of valid sampling rates for MP3 and AAC
	static const u_int32_t allSamplingRates[] = {
		7350, 8000, 11025, 12000, 16000, 22050, 
		24000, 32000, 44100, 48000, 64000, 88200, 96000
	};
	static const u_int8_t numAllSamplingRates =
		sizeof(allSamplingRates) / sizeof(u_int32_t);

	// for all possible sampling rates
	u_int8_t i;
	for (i = 0; i < numAllSamplingRates; i++) {
		u_int32_t targetRate = allSamplingRates[i];
		u_int32_t samplingRate = targetRate;

		// attempt to set sound card to this sampling rate
		rc = ioctl(audioDevice, SNDCTL_DSP_SPEED, &samplingRate);

		// invalid sampling rate, allow deviation of 1 sample/sec
		if (rc < 0 || abs(samplingRate - targetRate) > 1) {
			debug_message("audio device %s doesn't support sampling rate %u",
				m_deviceName, targetRate);
			continue;
		}

		// valid sampling rate
		m_samplingRates[m_numSamplingRates++] = targetRate;
	}

	// zero out remaining sampling rate entries
	for (i = m_numSamplingRates; i < numAllSamplingRates; i++) {
		m_samplingRates[i] = 0;
	}

	close(audioDevice);

	return true;
}
