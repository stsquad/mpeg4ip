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

	m_audioStartTimestamp = GetTimestamp();

	m_sourceAudio = true;
	m_source = true;
}

void CMpeg2FileSource::DoStopSource(void)
{
	if (!m_source) {
		return;
	}

	ShutdownVideo();

	free(m_videoYUVImage);
	m_videoYUVImage = NULL;
	m_videoYImage = NULL;
	m_videoUImage = NULL;
	m_videoVImage = NULL;

	if (m_audioEncoder) {
		// flush remaining output from audio encoder
		// and forward it to sinks

		m_audioEncoder->EncodeSamples(NULL, 0);

		u_int32_t forwardedSamples;
		u_int32_t forwardedFrames;

		ForwardEncodedAudioFrames(
			m_audioEncoder, 
			m_audioStartTimestamp
				+ SamplesToTicks(m_audioEncodedForwardedSamples),
			&forwardedSamples,
			&forwardedFrames);

		m_audioEncodedForwardedSamples += forwardedSamples;
		m_audioEncodedForwardedFrames += forwardedFrames;

		m_audioEncoder->Stop();
		delete m_audioEncoder;
		m_audioEncoder = NULL;
	}

	mpeg3_close(m_mpeg2File);
	m_mpeg2File = NULL;

	m_source = false;
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

	bool realTime =
		m_pConfig->GetBoolValue(CONFIG_RTP_ENABLE);

	CMediaSource::InitVideo(
		mpeg3_frame_rate(m_mpeg2File, m_videoStream),
		mpeg3_video_width(m_mpeg2File, m_videoStream),
		mpeg3_video_height(m_mpeg2File, m_videoStream),
		false, 
		realTime);

	m_videoYUVImage = (u_int8_t*)Malloc(m_videoSrcYUVSize);

	m_videoYImage = m_videoYUVImage;
	m_videoUImage = m_videoYUVImage + m_videoSrcYSize;
	m_videoVImage = m_videoYUVImage + m_videoSrcYSize + m_videoSrcUVSize;

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

	char* afmt = mpeg3_audio_format(m_mpeg2File, m_audioStream);

	if (!strcmp(afmt, "MPEG")) {
		m_audioSrcEncoding = AUDIO_ENCODING_MP3;
	} else if (!strcmp(afmt, "AAC")) {
		m_audioSrcEncoding = AUDIO_ENCODING_AAC;
	} else if (!strcmp(afmt, "AC3")) {
		m_audioSrcEncoding = AUDIO_ENCODING_AC3;
	} else {
		debug_message("mpeg2 unsupported audio format %s", afmt);
		return false;
	}

	m_audioSrcChannels = mpeg3_audio_channels(m_mpeg2File, m_audioStream);
	m_audioSrcSampleRate = mpeg3_sample_rate(m_mpeg2File, m_audioStream);

	// LATER allow audio resampling
	m_pConfig->SetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE,
		m_audioSrcSampleRate);

	char* dstEncoding =
		m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING);

	if (!strcasecmp(dstEncoding, AUDIO_ENCODING_MP3)) {
		m_audioDstFrameType = CMediaFrame::Mp3AudioFrame;
		m_audioSrcSamplesPerFrame = 1152;
	} else if (!strcasecmp(dstEncoding, AUDIO_ENCODING_AAC)) {
		m_audioDstFrameType = CMediaFrame::AacAudioFrame;
		m_audioSrcSamplesPerFrame = 1024;
	} else {
		m_audioDstFrameType = CMediaFrame::UndefinedFrame;
		m_audioSrcSamplesPerFrame = 1024;
	}

	if (!strcasecmp(dstEncoding, m_audioSrcEncoding)) {
		m_audioEncode = false;
	} else {
		m_audioEncode = true;
	}

	if (m_audioEncode) {
		m_audioEncoder = AudioEncoderCreate(
			m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODER));

		if (!m_audioEncoder) {
			return false;
		}
		if (!m_audioEncoder->Init(m_pConfig)) {
			return false;
		}
	}

	m_audioMaxFrameLength = 4 * 1024;

	m_audioSrcSamples = 0;
	m_audioSrcFrames = 0;
	m_audioRawForwardedSamples = 0;
	m_audioRawForwardedFrames = 0;
	m_audioEncodedForwardedSamples = 0;
	m_audioEncodedForwardedFrames = 0;
	m_audioElapsedDuration = 0;

	return true;
}

void CMpeg2FileSource::ProcessMedia(void)
{
	Duration start = GetElapsedDuration();

	// process ~1 second before returning to check for commands
	while (GetElapsedDuration() - start < (Duration)TimestampTicks) {

		if (m_sourceVideo && m_sourceAudio) {
			bool endOfVideo =
				mpeg3_end_of_video(m_mpeg2File, m_videoStream);
			bool endOfAudio =
				mpeg3_end_of_audio(m_mpeg2File, m_audioStream);

			if (endOfVideo && endOfAudio) {
				DoStopSource();
				break;
			}
			if (!endOfVideo && 
			  (m_videoElapsedDuration <= m_audioElapsedDuration 
				|| endOfAudio)) {
				ProcessVideo();
			} else {
				ProcessAudio();
			}

		} else if (m_sourceVideo) {
			if (mpeg3_end_of_video(m_mpeg2File, m_videoStream)) {
				DoStopSource();
				break;
			}
			ProcessVideo();

		} else if (m_sourceAudio) {
			if (mpeg3_end_of_audio(m_mpeg2File, m_audioStream)) {
				DoStopSource();
				break;
			}
			ProcessAudio();
		}
	}
}

void CMpeg2FileSource::ProcessVideo(void)
{
	// check if we'll encode this frame
	if (!WillUseVideoFrame()) {
		// if not, just skip frame, don't bother decoding it
		m_videoSrcFrameNumber++;
		mpeg3_set_frame(m_mpeg2File, m_videoSrcFrameNumber, m_videoStream);
		return;
	}

	if (m_realTime) {
		// TBD PaceFileSource();
		Duration realDuration =
			GetTimestamp() - m_videoStartTimestamp;
		Duration aheadDuration =
			m_videoElapsedDuration - realDuration; 

		if (aheadDuration >= m_maxAheadDuration) {
			SDL_Delay((aheadDuration - (m_maxAheadDuration / 2)) / 1000);
		}
	}

	int rc = 0;

	// read mpeg2 frame
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

	if (rc != 0) {
		debug_message("error reading mpeg2 video");
		return;
	}

	ProcessVideoFrame(
		m_videoYUVImage,
		m_videoStartTimestamp 
			+ (m_videoSrcFrameNumber * m_videoSrcFrameDuration));

	return;
}

void CMpeg2FileSource::ProcessAudio(void)
{
	int rc = 0;
	u_int32_t startSample =
		mpeg3_get_sample(m_mpeg2File, m_audioStream);
	u_int16_t* pcmLeftSamples = NULL;
	u_int16_t* pcmRightSamples = NULL;

	if (m_realTime) {
		// TBD PaceFileSource();
		Duration realDuration =
			GetTimestamp() - m_audioStartTimestamp;
		Duration aheadDuration =
			m_audioElapsedDuration - realDuration; 

		if (aheadDuration >= m_maxAheadDuration) {
			SDL_Delay((aheadDuration - (m_maxAheadDuration / 2)) / 1000);
		}
	}

	if (m_audioEncode
	  || m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {

		pcmLeftSamples = (u_int16_t*)
			Malloc(m_audioSrcSamplesPerFrame * sizeof(u_int16_t));

		rc = mpeg3_read_audio(
			m_mpeg2File,
			NULL,
			(int16_t*)pcmLeftSamples,
			0,
			m_audioSrcSamplesPerFrame,
			m_audioStream);

		if (rc != 0) {
			debug_message("error reading mpeg2 audio");
			goto cleanup;
		}

		if (m_audioSrcChannels > 1) {
			pcmRightSamples = (u_int16_t*)
				Malloc(m_audioSrcSamplesPerFrame * sizeof(u_int16_t));

			rc = mpeg3_reread_audio(
				m_mpeg2File,
				NULL,
				(int16_t*)pcmRightSamples,
				1,
				m_audioSrcSamplesPerFrame,
				m_audioStream);

			if (rc != 0) {
				debug_message("error reading mpeg2 audio");
				goto cleanup;
			}
		}

		// if we're going to want the encoded frame from the file
		// then we need to backup so we can reread the frame
		if (!m_audioEncode) {
			mpeg3_set_sample(m_mpeg2File, startSample, m_audioStream);
		}
	}
			
	// encode audio frame
	if (m_audioEncode) {
		rc = m_audioEncoder->EncodeSamples(
			pcmLeftSamples, 
			pcmRightSamples,
			m_audioSrcSamplesPerFrame * sizeof(u_int16_t));

		if (!rc) {
			debug_message("error encoding mpeg2 audio");
			goto cleanup;
		}

		u_int32_t forwardedSamples;
		u_int32_t forwardedFrames;

		ForwardEncodedAudioFrames(
			m_audioEncoder, 
			m_audioStartTimestamp 
				+ SamplesToTicks(m_audioEncodedForwardedSamples),
			&forwardedSamples,
			&forwardedFrames);

		m_audioEncodedForwardedSamples += forwardedSamples;
		m_audioEncodedForwardedFrames += forwardedFrames;

	} else {
		// else it's already encoded MP3 or AAC

		u_int8_t* frameData =
			(u_int8_t*)Malloc(m_audioMaxFrameLength);
		u_int32_t frameLength;

		// just read the encoded data
		rc = mpeg3_read_audio_chunk(
			m_mpeg2File,
			frameData,
			(long int*)&frameLength,
			m_audioMaxFrameLength,
			m_audioStream);

		if (rc != 0) {
			debug_message("error reading mpeg2 audio");
			goto cleanup;
		}

		mpeg3_set_sample(m_mpeg2File, 
			startSample + m_audioSrcSamplesPerFrame, m_audioStream);

		// and forward it to sinks
		CMediaFrame* pFrame =
			new CMediaFrame(
				m_audioDstFrameType,
				frameData, 
				frameLength,
				m_audioStartTimestamp 
					+ SamplesToTicks(m_audioEncodedForwardedSamples),
				m_audioSrcSamplesPerFrame,
				m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));
		ForwardFrame(pFrame);
		delete pFrame;
	}

	// if desired, forward raw audio to sinks
	if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {
		u_int16_t* pcmSamples = NULL;

		// if necessary, interleave left and right channels
		if (pcmRightSamples) {
			CAudioEncoder::InterleaveStereoSamples(
				pcmLeftSamples,
				pcmRightSamples,
				m_audioSrcSamplesPerFrame,
				&pcmSamples);
		} else {
			pcmSamples = pcmLeftSamples;
			pcmLeftSamples = NULL;	// avoid free during cleanup
		}

		CMediaFrame* pFrame =
			new CMediaFrame(
				CMediaFrame::PcmAudioFrame, 
				pcmSamples, 
				m_audioSrcSamplesPerFrame * sizeof(u_int16_t)
					* m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS),
				m_audioStartTimestamp + SamplesToTicks(m_audioSrcSamples),
				m_audioSrcSamplesPerFrame,
				m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));
		ForwardFrame(pFrame);
		delete pFrame;
	}

	m_audioSrcSamples += m_audioSrcSamplesPerFrame;
	m_audioSrcFrames++;

cleanup:
	free(pcmLeftSamples);
	free(pcmRightSamples);
}

Duration CMpeg2FileSource::GetElapsedDuration()
{
	m_videoElapsedDuration =
		m_videoSrcFrameNumber * m_videoSrcFrameDuration;

	m_audioElapsedDuration =
		SamplesToTicks(m_audioSrcSamples);

	if (m_sourceVideo && m_sourceAudio) {
		return MIN(m_videoElapsedDuration, m_audioElapsedDuration);
	} else if (m_sourceVideo) {
		return m_videoElapsedDuration;
	} else if (m_sourceAudio) {
		return m_audioElapsedDuration;
	}
	return 0;
}

