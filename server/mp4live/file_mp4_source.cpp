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
 */

#include "mp4live.h"
#include "file_mp4_source.h"
#include <mp4av.h>


int CMp4FileSource::ThreadMain(void) 
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
					DoStopSource();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_NODE_START:
					DoStartVideo();
					DoStartAudio();
					break;
				case MSG_SOURCE_START_VIDEO:
					DoStartVideo();
					break;
				case MSG_SOURCE_START_AUDIO:
					DoStartAudio();
					break;
				case MSG_NODE_STOP:
					DoStopSource();
					break;
				case MSG_SOURCE_KEY_FRAME:
					DoGenerateKeyFrame();
					break;
				}

				delete pMsg;
			}
		}

		if (m_source) {
			try {
				ProcessMedia();
			}
			catch (...) {
				DoStopSource();
				break;
			}
		}
	}

	return -1;
}

void CMp4FileSource::DoStartVideo(void)
{
	if (m_sourceVideo) {
		return;
	}

	if (!m_source) {
		m_mp4File = MP4Read(
			m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME),
			MP4_DETAILS_ERROR);

		if (!m_mp4File) {
			return;
		}
	}

	if (!InitVideo()) {
		return;
	}

	m_sourceVideo = true;
	m_source = true;
}

void CMp4FileSource::DoStartAudio(void)
{
	if (m_sourceAudio) {
		return;
	}

	if (!m_source) {
		m_mp4File = MP4Read(
			m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME),
			MP4_DETAILS_ERROR);

		if (m_mp4File == MP4_INVALID_FILE_HANDLE) {
			return;
		}
	}

	if (!InitAudio()) {
		return;
	}

	m_sourceAudio = true;
	m_source = true;
}

void CMp4FileSource::DoStopSource(void)
{
	CMediaSource::DoStopSource();

	MP4Close(m_mp4File);
	m_mp4File = MP4_INVALID_FILE_HANDLE;
}

bool CMp4FileSource::InitVideo(void)
{
	m_videoTrackId = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_SOURCE_TRACK);

	const char* trackType =
		MP4GetTrackType(m_mp4File, m_videoTrackId);

	if (strcasecmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
		debug_message("mp4 bad video track number %u\n", m_videoTrackId);
		return false;
	}

	u_int8_t videoType = 
		MP4GetTrackVideoType(m_mp4File, m_videoTrackId);

	if (videoType != MP4_YUV12_VIDEO_TYPE) {
		debug_message("TBD can't source from encoded video tracks yet");
		return false;
	}

	MediaType srcType = CMediaFrame::YuvVideoFrame;

	m_videoSrcTotalFrames = 
		MP4GetTrackNumberOfSamples(m_mp4File, m_videoTrackId);

	bool realTime =
		m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE);

	return CMediaSource::InitVideo(
		srcType,
		MP4GetTrackVideoWidth(m_mp4File, m_videoTrackId),
		MP4GetTrackVideoHeight(m_mp4File, m_videoTrackId),
		false, 
		realTime);
}

bool CMp4FileSource::InitAudio(void)
{
	m_audioTrackId = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK);

	const char* trackType =
		MP4GetTrackType(m_mp4File, m_audioTrackId);

	if (strcasecmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
		debug_message("mp4 bad audio track number %u\n", m_audioTrackId);
		return false;
	}

	u_int8_t audioType = 
		MP4GetTrackAudioType(m_mp4File, m_audioTrackId);

	if (audioType != MP4_PCM16_AUDIO_TYPE) {
		debug_message("TBD can't source from encoded audio tracks yet");
		return false;
	}

	MediaType srcType = CMediaFrame::PcmAudioFrame;

	m_audioSrcTotalFrames = 
		MP4GetTrackNumberOfSamples(m_mp4File, m_audioTrackId);

	u_int8_t srcChannels =
		MP4AV_AudioGetChannels(m_mp4File, m_audioTrackId);
	u_int32_t srcSamplingRate =
		MP4AV_AudioGetSamplingRate(m_mp4File, m_audioTrackId);
		
	bool realTime =
		m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE);

	bool rc = CMediaSource::InitAudio(
		srcType,
		srcChannels,
		srcSamplingRate,
		realTime);

	if (!rc) {
		return false;
	}

	if (srcType == CMediaFrame::AacAudioFrame) {
		m_audioSrcSamplesPerFrame =
			MP4AV_AudioGetSamplingWindow(m_mp4File, m_audioTrackId);
	}

	return true;
}

void CMp4FileSource::ProcessVideo(void)
{
	PaceSource();

	u_int8_t* pBytes = NULL;
	u_int32_t bytesLength = 0;
	MP4Duration frameDuration;

	bool rc = MP4ReadSample(
		m_mp4File,
		m_videoTrackId,
		m_videoSrcFrameNumber + 1,
		&pBytes,
		&bytesLength,
		NULL,
		&frameDuration);

	if (!rc) {
		debug_message("error reading mp4 video");
		return;
	}

	ProcessVideoFrame(
		pBytes,
		bytesLength,
		(Duration)MP4ConvertFromTrackDuration(
			m_mp4File, m_videoTrackId, frameDuration, TimestampTicks));

	free(pBytes);

	return;
}

void CMp4FileSource::ProcessAudio(void)
{
	PaceSource();

	u_int8_t* pBytes = NULL;
	u_int32_t bytesLength = 0;
	MP4Duration frameDuration;

	bool rc = MP4ReadSample(
		m_mp4File,
		m_audioTrackId,
		m_audioSrcFrameNumber + 1,
		&pBytes,
		&bytesLength,
		NULL,
		&frameDuration);

	if (!rc) {
		debug_message("error reading mp4 audio");
		return;
	}

	ProcessAudioFrame(
		pBytes,
		bytesLength,
		(Duration)MP4ConvertFromTrackDuration(
			m_mp4File, m_audioTrackId, frameDuration, m_audioSrcSampleRate));

	free(pBytes);

	return;
}

float CMp4FileSource::GetProgress()
{
	if (m_sourceVideo && m_sourceAudio) {
		return MIN(
			(float)m_videoSrcFrameNumber / (float)m_videoSrcTotalFrames,
			(float)m_audioSrcFrameNumber / (float)m_audioSrcTotalFrames);
	} else if (m_sourceVideo) {
		return (float)m_videoSrcFrameNumber / (float)m_videoSrcTotalFrames;
	} else if (m_sourceAudio) {
		return (float)m_audioSrcFrameNumber / (float)m_audioSrcTotalFrames;
	}
	return 0.0;
}

