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

		if (m_capture) {
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

		if (m_capture) {
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
	if (m_capture) {
		return;
	}

	if (!Init()) {
		return;
	}

	m_capture = true;
}

void COSSAudioSource::DoStopCapture()
{
	if (!m_capture) {
		return;
	}

	if (m_encoder) {
		// flush remaining output from encoders
		// and forward it to sinks

		m_encoder->EncodeSamples(NULL, 0);

		u_int32_t forwardedSamples;
		u_int32_t forwardedFrames;

		ForwardEncodedAudioFrames(
			m_encoder, 
			m_startTimestamp 
				+ SamplesToTicks(m_encodedForwardedSamples),
			&forwardedSamples,
			&forwardedFrames);

		m_encodedForwardedSamples += forwardedSamples;
		m_encodedForwardedFrames += forwardedFrames;

		m_encoder->Stop();
		delete m_encoder;
		m_encoder = NULL;
	}

	close(m_audioDevice);
	m_audioDevice = -1;

	free(m_rawFrameBuffer);
	m_rawFrameBuffer = NULL;

	m_capture = false;
}

bool COSSAudioSource::Init(void)
{
	m_startTimestamp = 0;

	if (m_pConfig->m_audioEncode) {
		if (!InitEncoder()) {
			goto init_failure;
		}

		m_rawSamplesPerFrame = 
			m_encoder->GetSamplesPerFrame();
	} else {
		m_rawSamplesPerFrame = 
			m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
	}

	if (!InitDevice()) {
		return false;
	}

	m_rawFrameSize = m_rawSamplesPerFrame
		* m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) 
		* sizeof(u_int16_t);

	m_rawFrameBuffer = (u_int16_t*)malloc(m_rawFrameSize);

	if (!m_rawFrameBuffer) {
		goto init_failure;
	}

	m_rawForwardedSamples = 0;
	m_rawForwardedFrames = 0;
	m_encodedForwardedSamples = 0;
	m_encodedForwardedFrames = 0;

	// maximum number of passes in ProcessAudio, approx 1 sec.
	m_maxPasses = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE) 
		/ m_rawSamplesPerFrame;

	return true;

init_failure:
	debug_message("audio initialization failed");

	free(m_rawFrameBuffer);
	m_rawFrameBuffer = NULL;

	delete m_encoder;
	m_encoder = NULL;

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

bool COSSAudioSource::InitEncoder()
{
	char* encoderName = 
		m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODER);

	m_encoder = AudioEncoderCreate(encoderName);

	if (m_encoder == NULL) {
		return false;
	}

	return m_encoder->Init(m_pConfig);
}

void COSSAudioSource::ProcessAudio(void)
{
	bool rc;

	// for efficiency, process 1 second before returning to check for commands
	for (int pass = 0; pass < m_maxPasses; pass++) {

		// get a new buffer, if we've handed off the old one
		// currently only happens when we're doing a raw record
		if (m_rawFrameBuffer == NULL) {
			m_rawFrameBuffer = (u_int16_t*)Malloc(m_rawFrameSize);
		}

		// read a frame's worth of raw PCM data
		u_int32_t bytesRead = 
			read(m_audioDevice, m_rawFrameBuffer, m_rawFrameSize); 

		if (bytesRead < m_rawFrameSize) {
			continue;
		}

		Timestamp now = GetTimestamp();

		if (m_startTimestamp == 0) {
			m_startTimestamp = now - SamplesToTicks(m_rawSamplesPerFrame);
		}

		// encode audio frame
		if (m_pConfig->m_audioEncode) {

			rc = m_encoder->EncodeSamples(m_rawFrameBuffer, bytesRead);

			if (!rc) {
				debug_message("oss audio failed to encode");
			}

			u_int32_t forwardedSamples;
			u_int32_t forwardedFrames;

			ForwardEncodedAudioFrames(
				m_encoder, 
				m_startTimestamp 
					+ SamplesToTicks(m_encodedForwardedSamples),
				&forwardedSamples,
				&forwardedFrames);

			m_encodedForwardedSamples += forwardedSamples;
			m_encodedForwardedFrames += forwardedFrames;
		}

		// if desired, forward raw audio to sinks
		if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {

			CMediaFrame* pFrame =
				new CMediaFrame(
					CMediaFrame::PcmAudioFrame, 
					m_rawFrameBuffer, 
					m_rawFrameSize,
					m_startTimestamp + SamplesToTicks(m_rawForwardedSamples),
					m_rawSamplesPerFrame,
					m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));
			ForwardFrame(pFrame);
			delete pFrame;

			m_rawForwardedSamples += m_rawSamplesPerFrame;
			m_rawForwardedFrames++;

			// we'll get a new buffer on the next pass
			m_rawFrameBuffer = NULL;
		}
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

		// invalid sampling rate, allow deviation of up to 1 sample/sec
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
