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
#include "file_mpeg2_source.h"
#include "audio_encoder.h"


int CMpeg2FileSource::ThreadMain(void) 
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

void CMpeg2FileSource::DoStartVideo(void)
{
	if (m_sourceVideo) {
		return;
	}

	if (!m_source) {
		m_mpeg2File = mpeg3_open(
			m_pConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME));

		if (!m_mpeg2File) {
			return;
		}

#ifdef USE_MMX
		mpeg3_set_mmx(m_mpeg2File, true);
#endif
	}

	if (!InitVideo()) {
		return;
	}

	m_sourceVideo = true;
	m_source = true;
}

void CMpeg2FileSource::DoStartAudio(void)
{
	if (m_sourceAudio) {
		return;
	}

	if (!m_source) { 
		m_mpeg2File = mpeg3_open(
			m_pConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));

		if (!m_mpeg2File) {
			return;
		}

#ifdef USE_MMX
		mpeg3_set_mmx(m_mpeg2File, true);
#endif
	}

	if (!InitAudio()) {
		return;
	}

	m_sourceAudio = true;
	m_source = true;
}

void CMpeg2FileSource::DoStopSource(void)
{
	CMediaSource::DoStopSource();

#ifdef MPEG2_RAW_ONLY
	free(m_videoYUVImage);
	m_videoYUVImage = NULL;
	m_videoYImage = NULL;
	m_videoUImage = NULL;
	m_videoVImage = NULL;

	free(m_audioPcmLeftSamples);
	m_audioPcmLeftSamples = NULL;
	free(m_audioPcmRightSamples);
	m_audioPcmRightSamples = NULL;
	free(m_audioPcmSamples);
	m_audioPcmSamples = NULL;
#else
	free(m_videoMpeg2Frame);
	m_videoMpeg2Frame = NULL;

	free(m_audioFrameData);
	m_audioFrameData = NULL;
#endif

	mpeg3_close(m_mpeg2File);
	m_mpeg2File = NULL;
}

bool CMpeg2FileSource::InitVideo(void)
{
	if (!mpeg3_has_video(m_mpeg2File)) {
		debug_message("mpeg2 file doesn't contain video\n");
		return false;
	}

	m_videoStream = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_SOURCE_TRACK);

	if (m_videoStream >= mpeg3_total_vstreams(m_mpeg2File)) {
		debug_message("mpeg2 bad video stream number\n");
		return false;
	}

	float srcFrameRate =
		mpeg3_frame_rate(m_mpeg2File, m_videoStream);

	m_videoSrcFrameDuration =
		(Duration)(((float)TimestampTicks / srcFrameRate) + 0.5);

	bool realTime =
		m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE);

	CMediaSource::InitVideo(
		CMediaFrame::YuvVideoFrame,
		mpeg3_video_width(m_mpeg2File, m_videoStream),
		mpeg3_video_height(m_mpeg2File, m_videoStream),
		false, 
		realTime);

#ifdef MPEG2_RAW_ONLY
	m_videoYUVImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);

	m_videoYImage = m_videoYUVImage;
	m_videoUImage = m_videoYUVImage + m_videoSrcYSize;
	m_videoVImage = m_videoYUVImage + m_videoSrcYSize + m_videoSrcUVSize;
#else
	m_videoSrcMaxFrameLength = 128 * 1024;
	m_videoMpeg2Frame = (u_int8_t*)Malloc(m_videoSrcMaxFrameLength);
#endif

	return true;
}

bool CMpeg2FileSource::InitAudio(void)
{
	if (!mpeg3_has_audio(m_mpeg2File)) {
		debug_message("mpeg2 file doesn't contain audio\n");
		return false;
	}

	m_audioStream = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK);

	// verify m_audioStream
	if (m_audioStream >= mpeg3_total_astreams(m_mpeg2File)) {
		debug_message("mpeg2 bad audio stream number\n");
		return false;
	}

	char* afmt = 
		mpeg3_audio_format(m_mpeg2File, m_audioStream);
	MediaType srcType;

#ifdef MPEG2_RAW_ONLY
	srcType = CMediaFrame::PcmAudioFrame;
#else
	if (!strcmp(afmt, "MPEG")) {
		srcType = CMediaFrame::Mp3AudioFrame;
	} else if (!strcmp(afmt, "AAC")) {
		srcType = CMediaFrame::AacAudioFrame;
	} else if (!strcmp(afmt, "AC3")) {
		srcType = CMediaFrame::Ac3AudioFrame;
	} else if (!strcmp(afmt, "PCM")) {
		srcType = CMediaFrame::PcmAudioFrame;
	} else if (!strcmp(afmt, "Vorbis")) {
		srcType = CMediaFrame::VorbisAudioFrame;
	} else {
		debug_message("mpeg2 unsupported audio format %s", afmt);
		return false;
	}
#endif

	bool realTime =
		m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE);

	CMediaSource::InitAudio(
		srcType,
		mpeg3_audio_channels(m_mpeg2File, m_audioStream),
		mpeg3_sample_rate(m_mpeg2File, m_audioStream),
		realTime);

#ifdef MPEG2_RAW_ONLY
	m_audioPcmLeftSamples = (u_int16_t*)
		Malloc(m_audioSrcSamplesPerFrame * sizeof(u_int16_t));

	if (m_audioSrcChannels > 1) {
		m_audioPcmRightSamples = (u_int16_t*)
			Malloc(m_audioSrcSamplesPerFrame * sizeof(u_int16_t));
		m_audioPcmSamples = (u_int16_t*)
			Malloc(2 * m_audioSrcSamplesPerFrame * sizeof(u_int16_t));
	} else {
		m_audioPcmRightSamples = NULL;
		m_audioPcmSamples = NULL;
	}

#else
	m_audioSrcMaxFrameLength = 4 * 1024;
	m_audioFrameData = (u_int8_t*)Malloc(m_audioSrcMaxFrameLength);
#endif

	return true;
}

void CMpeg2FileSource::ProcessVideo()
{
	// check if we'll use this frame
	if (!WillUseVideoFrame(m_videoSrcFrameDuration)) {
		// if not, just skip frame, don't bother decoding it
		m_videoSrcFrameNumber++;
		m_videoSrcElapsedDuration += m_videoSrcFrameDuration;
		mpeg3_set_frame(m_mpeg2File, m_videoSrcFrameNumber, m_videoStream);
		return;
	}

	PaceSource();

	int rc;
	u_int8_t* frameData;
	u_int32_t frameDataLength;

#ifdef MPEG2_RAW_ONLY

	// read and decode video frame to YUV
	rc = mpeg3_read_yuvframe(
		m_mpeg2File, 
		(char*)m_videoYImage, 
		(char*)m_videoUImage, 
		(char*)m_videoVImage, 
		0, 
		0,
		m_videoSrcWidth, 
		m_videoSrcHeight,
		m_videoStream);

	frameData = m_videoYUVImage;
	frameDataLength = m_videoSrcYUVSize;

#else

	// read encoded MPEG frame
	rc = mpeg3_read_video_chunk(
		m_mpeg2File,
		m_videoMpeg2Frame,
		(long*)&frameDataLength,
		m_videoSrcMaxFrameLength,
		m_videoStream);

	frameData = m_videoMpeg2Frame;

	// TBD may need to set frame

#endif

	if (rc != 0) {
		debug_message("error reading mpeg2 video");
		return;
	}

	ProcessVideoFrame(
		frameData,
		frameDataLength,
		m_videoSrcFrameDuration);

	return;
}

void CMpeg2FileSource::ProcessAudio(void)
{
	PaceSource();

	int rc;
	u_int8_t* frameData;
	u_int32_t frameDataLength;

#ifdef MPEG2_RAW_ONLY
	rc = mpeg3_read_audio(
		m_mpeg2File,
		NULL,
		(int16_t*)m_audioPcmLeftSamples,
		0,
		m_audioSrcSamplesPerFrame,
		m_audioStream);

	if (rc != 0) {
		debug_message("error reading mpeg2 audio");
		return;
	}

	if (m_audioPcmRightSamples) {
		rc = mpeg3_reread_audio(
			m_mpeg2File,
			NULL,
			(int16_t*)m_audioPcmRightSamples,
			1,
			m_audioSrcSamplesPerFrame,
			m_audioStream);

		if (rc != 0) {
			debug_message("error reading mpeg2 audio");
			return;
		}

		CAudioEncoder::InterleaveStereoSamples(
			m_audioPcmLeftSamples,
			m_audioPcmRightSamples,
			m_audioSrcSamplesPerFrame,
			&m_audioPcmSamples);

		frameData = (u_int8_t*)m_audioPcmSamples;
		frameDataLength = 2 * m_audioSrcSamplesPerFrame * sizeof(u_int16_t);
	} else {
		frameData = (u_int8_t*)m_audioPcmLeftSamples;
		frameDataLength = m_audioSrcSamplesPerFrame * sizeof(u_int16_t);
	}

#else
	u_int32_t startSample =
		mpeg3_get_sample(m_mpeg2File, m_audioStream);

	// read the audio data
	rc = mpeg3_read_audio_chunk(
		m_mpeg2File,
		m_audioFrameData,
		(long*)&frameDataLength,
		m_audioSrcMaxFrameLength,
		m_audioStream);

	if (rc != 0) {
		debug_message("error reading mpeg2 audio");
		return;
	}

	mpeg3_set_sample(
		m_mpeg2File, 
		startSample + m_audioSrcSamplesPerFrame, 
		m_audioStream);

	frameData = m_audioFrameData;
#endif 

	ProcessAudioFrame(
		frameData,
		frameDataLength,
		m_audioSrcSamplesPerFrame);
}

