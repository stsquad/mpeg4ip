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
#include "audio_source.h"
#include "mp3.h"

int CAudioSource::ThreadMain(void) 
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
				case MSG_START_CAPTURE:
					DoStartCapture();
					break;
				case MSG_STOP_CAPTURE:
					DoStopCapture();
					break;
				}

				delete pMsg;
			}
		}

		if (m_capture) {
			ProcessAudio();
		}
	}

	return -1;
}

void CAudioSource::DoStartCapture()
{
	if (m_capture) {
		return;
	}

	if (!Init()) {
		return;
	}

	m_capture = true;
}

void CAudioSource::DoStopCapture()
{
	if (!m_capture) {
		return;
	}

	if (m_pConfig->m_audioEncode) {
		// lame can be holding onto a few MP3 frames
		// get them and forward them to sinks

		u_int32_t mp3DataLength = lame_encode_finish(
			&m_lameParams,
			(char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
			m_mp3FrameBufferSize - m_mp3FrameBufferLength);

		if (mp3DataLength > 0) {
			m_mp3FrameBufferLength += mp3DataLength;
			ForwardCompletedFrames(GetTimestamp());
		}
	}

	close(m_audioDevice);
	m_audioDevice = -1;

	m_capture = false;
}

bool CAudioSource::Init(void)
{
	if (!InitDevice()) {
		return false;
	}

	if (m_pConfig->m_audioEncode) {
		if (!InitEncoder()) {
			close(m_audioDevice);
			m_audioDevice = -1;
			return false;
		}
	}

	m_samplesPerFrame = MP3_SAMPLES_PER_FRAME;
	m_rawFrameSize = 
		m_samplesPerFrame *  m_pConfig->m_audioChannels * sizeof(u_int16_t);
	m_frameDuration = (m_samplesPerFrame * TimestampTicks) 
		/ m_pConfig->m_audioSamplingRate;
	// maximum number of passes in ProcessAudio, approx 1 sec.
	m_maxPasses = m_pConfig->m_audioSamplingRate / m_samplesPerFrame;
	m_rawFrameNumber = 0xFFFFFFFF;
	m_mp3MaxFrameSize = (u_int)(1.25 * m_samplesPerFrame) + 7200;
	m_mp3FrameBufferSize = 2 * m_mp3MaxFrameSize;
	m_mp3FrameBufferLength = 0;

	m_rawFrameBuffer = (u_int16_t*)malloc(m_rawFrameSize);
	m_leftBuffer = (u_int16_t*)malloc(m_samplesPerFrame * sizeof(u_int16_t));
	m_rightBuffer = (u_int16_t*)malloc(m_samplesPerFrame * sizeof(u_int16_t));
	m_mp3FrameBuffer = (u_int8_t*)malloc(m_mp3FrameBufferSize);

	if (!m_rawFrameBuffer || !m_leftBuffer || !m_rightBuffer 
	  || !m_mp3FrameBuffer) {
		close(m_audioDevice);
		m_audioDevice = -1;
		return false;
	}

	return true;
}

bool CAudioSource::InitDevice(void)
{
	int rc;

	// open the audio device
	m_audioDevice = open(m_pConfig->m_audioDeviceName, O_RDONLY);
	if (m_audioDevice < 0) {
		error_message("Failed to open %s", 
			m_pConfig->m_audioDeviceName);
		return false;
	}

	int format = AFMT_S16_LE;
	rc = ioctl(m_audioDevice, SNDCTL_DSP_SETFMT, &format);
	if (rc < 0 || format != AFMT_S16_LE) {
		error_message("Couldn't set format for %s", 
			m_pConfig->m_audioDeviceName);
		return false;
	}

	int channels = m_pConfig->m_audioChannels;
	rc = ioctl(m_audioDevice, SNDCTL_DSP_CHANNELS, &channels);
	if (rc < 0 || channels != m_pConfig->m_audioChannels) {
		error_message("Couldn't set audio channels for %s", 
			m_pConfig->m_audioDeviceName);
		return false;
	}

	int samplingRate = m_pConfig->m_audioSamplingRate;
	rc = ioctl(m_audioDevice, SNDCTL_DSP_SPEED, &samplingRate);
	if (rc < 0 || (u_int)abs(samplingRate - m_pConfig->m_audioSamplingRate) >
	  m_pConfig->m_audioSamplingRate / 10) {
		error_message("Couldn't set sampling rate for %s", 
			m_pConfig->m_audioDeviceName);
		return false;
	}

	return true;
}

bool CAudioSource::InitEncoder()
{
	lame_init(&m_lameParams);

	m_lameParams.num_channels = m_pConfig->m_audioChannels;
	m_lameParams.in_samplerate = m_pConfig->m_audioSamplingRate;
	m_lameParams.brate = m_pConfig->m_audioTargetBitRate;
	m_lameParams.mode = 0;
	m_lameParams.quality = 2;
	m_lameParams.silent = 1;
	m_lameParams.gtkflag = 0;

	lame_init_params(&m_lameParams);

	return true;
}

void CAudioSource::ProcessAudio(void)
{
	// for efficiency, process 1 second before returning to check for commands
	for (int pass = 0; pass < m_maxPasses; pass++) {

		// get a new buffer, if we've handed off the old one
		// currently only happens when we're doing a raw record
		if (m_rawFrameBuffer == NULL) {
			m_rawFrameBuffer = (u_int16_t*)malloc(m_rawFrameSize);
			if (m_rawFrameBuffer == NULL) {
				debug_message("malloc error");
				break;
			}
		}

		// read the a frame's worth of raw PCM data
		int bytesRead = read(m_audioDevice, m_rawFrameBuffer, m_rawFrameSize); 

		if (bytesRead <= 0) {
			continue;
		}

		// OPTION timing might be better if we used
		// frameTimestamp = m_startTimestamp 
		// + m_rawFrameNumber * m_frameDuration;

		Timestamp frameTimestamp = GetTimestamp() - m_frameDuration;
		m_rawFrameNumber++;
		if (m_rawFrameNumber == 0) {
			m_startTimestamp = frameTimestamp;
		}
		if (m_mp3FrameBufferLength == 0) {
			m_mp3FrameTimestamp = frameTimestamp;
		}

		// encode audio frame to MP3
		if (m_pConfig->m_audioEncode) {
			// de-interleave raw frame buffer
			u_int16_t* s = m_rawFrameBuffer;
			for (int i = 0; i < m_samplesPerFrame; i++) {
				m_leftBuffer[i] = *s++;
				m_rightBuffer[i] = *s++;
			}

			// call lame encoder
			u_int32_t mp3DataLength = lame_encode_buffer(
				&m_lameParams,
				(short*)m_leftBuffer, (short*)m_rightBuffer, 
				m_samplesPerFrame,
				(char*)&m_mp3FrameBuffer[m_mp3FrameBufferLength], 
				m_mp3FrameBufferSize - m_mp3FrameBufferLength);

			// forward the completed frames
			if (mp3DataLength > 0) {
				m_mp3FrameBufferLength += mp3DataLength;
				ForwardCompletedFrames(frameTimestamp);
			}
		}

		// if desired, forward raw audio to sinks
		if (m_pConfig->m_recordRaw) {
			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::PcmAudioFrame, 
					m_rawFrameBuffer, m_rawFrameSize,
					frameTimestamp, m_frameDuration);
			ForwardFrame(pFrame);
			delete pFrame;

			// we'll get a new buffer on the next pass
			m_rawFrameBuffer = NULL;
		}
	}
}

void CAudioSource::ForwardCompletedFrames(Timestamp newFrameTimestamp)
{
	u_int8_t* mp3Frame;
	u_int32_t mp3FrameLength;

	while (Mp3FindNextFrame(m_mp3FrameBuffer,
	  m_mp3FrameBufferLength, &mp3Frame, &mp3FrameLength, false)) {

		// check if we have all the bytes for the MP3 frame
		if (mp3FrameLength > m_mp3FrameBufferLength) {
			break;
		}

		// need a buffer for this MP3 frame
		u_int8_t* newMp3Frame = (u_int8_t*)malloc(mp3FrameLength);
		if (newMp3Frame == NULL) {
			debug_message("malloc error");
			break;
		}

		// copy the MP3 frame
		memcpy(newMp3Frame, mp3Frame, mp3FrameLength);

		// forward the copy of the MP3 frame to sinks
		CMediaFrame* pFrame =
			new CMediaFrame(CMediaFrame::Mp3AudioFrame, 
				newMp3Frame, mp3FrameLength,
				m_mp3FrameTimestamp, m_frameDuration);
		ForwardFrame(pFrame);
		delete pFrame;

		// shift what remains in the buffer down
		memcpy(m_mp3FrameBuffer, 
			mp3Frame + mp3FrameLength, 
			m_mp3FrameBufferLength - mp3FrameLength);
		m_mp3FrameBufferLength -= mp3FrameLength;

		m_mp3FrameTimestamp = newFrameTimestamp;
	}
}

