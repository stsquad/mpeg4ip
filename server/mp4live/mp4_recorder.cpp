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

//#define MP4V2 1

#include "mp4live.h"
#include "mp4_recorder.h"
#include "video_source.h"
#include "mp3.h"


int CMp4Recorder::ThreadMain(void) 
{
	while (SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
		CMsg* pMsg = m_myMsgQueue.get_message();
		
		if (pMsg != NULL) {
			switch (pMsg->get_value()) {
			case MSG_NODE_STOP_THREAD:
				DoStopRecord();
				delete pMsg;
				return 0;
			case MSG_START_RECORD:
				DoStartRecord();
				break;
			case MSG_STOP_RECORD:
				DoStopRecord();
				break;
			case MSG_SINK_FRAME:
				size_t dontcare;
				DoWriteFrame((CMediaFrame*)pMsg->get_message(dontcare));
				break;
			}

			delete pMsg;
		}
	}

	return -1;
}

void CMp4Recorder::DoStartRecord()
{
	if (m_record) {
		return;
	}

#ifdef MP4V2
	m_mp4File = MP4Create(
		m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME));
#else
	m_mp4File = quicktime_open(
		m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME), 0, 1, 0);
#endif

	if (!m_mp4File) {
		return;
	}

	m_audioTimeScale = m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
	m_audioFrameDuration = MP3_SAMPLES_PER_FRAME;

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		m_movieTimeScale = m_videoTimeScale;
	} else {
		m_movieTimeScale = m_audioTimeScale;
	}
#ifdef MP4V2
	MP4SetTimeScale(m_mp4File, m_movieTimeScale);
#else
	quicktime_set_time_scale(m_mp4File, m_movieTimeScale);
#endif

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		m_videoFrameNum = 1;

#ifdef MP4V2
		m_videoTrack = MP4AddVideoTrack(m_mp4File,
			m_videoTimeScale,
			0,
			m_pConfig->m_videoWidth, 
			m_pConfig->m_videoHeight);

		if (!m_videoTrack) {
			// TBD error
		}

		MP4SetVideoProfileLevel(m_mp4File, 
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID));

		MP4SetTrackESConfiguration(m_mp4File, m_videoTrack,
			m_pConfig->m_videoMpeg4Config, 
			m_pConfig->m_videoMpeg4ConfigLength); 
#else
		quicktime_set_video(m_mp4File, 1, 
			m_pConfig->m_videoWidth, 
			m_pConfig->m_videoHeight,
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE), 
			m_videoTimeScale,
			"mp4v");

		quicktime_set_iod_video_profile_level(m_mp4File,
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID));

		/* create the appropriate MP4 decoder config */
		quicktime_set_mp4_video_decoder_config(m_mp4File, m_videoTrack, 
			m_pConfig->m_videoMpeg4Config, 
			m_pConfig->m_videoMpeg4ConfigLength); 
#endif

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
			m_videoPayloadNumber = 0;	// dynamically assigned

#ifdef MP4V2
			m_videoHintTrack = MP4AddHintTrack(m_mp4File, m_videoTrack);

			// TBD MP4SetHintTrackRtpPayload(m_mp4File, m_videoHintTrack,
			// 	"MP4V-ES", &m_videoPayloadNumber,  
			//	m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
#else
			m_videoHintTrack = quicktime_set_video_hint(
				m_mp4File, m_videoTrack, 
				"MP4V-ES", &m_videoPayloadNumber, 
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
#endif

			m_videoRtpPktNum = 1;
			m_videoBytesThisSec = 0;
			m_videoFirstRtpPktThisSec = m_videoRtpPktNum;
			m_videoMaxRtpBytesPerSec = 0;

			/* convert the mpeg4 configuration to ASCII form */
			char* sConfig = BinaryToAscii(m_pConfig->m_videoMpeg4Config, 
				m_pConfig->m_videoMpeg4ConfigLength); 

			/* create the appropriate SDP attribute */
			if (sConfig) {
				char sdpBuf[1024];

				sprintf(sdpBuf,
					"a=fmtp:%u profile-level-id=%u; config=%s;\n",
						m_videoPayloadNumber,
						m_pConfig->GetIntegerValue(
							CONFIG_VIDEO_PROFILE_LEVEL_ID),
						sConfig); 

				/* add this to the MP4 file's sdp atom */
#ifdef MP4V2
				// TBD MP4SetHintTrackSdp(m_mp4File, m_videoHintTrack, sdpBuf);
#else
				quicktime_add_video_sdp(m_mp4File, sdpBuf, 
					m_videoTrack, m_videoHintTrack);
#endif

				/* cleanup, don't want to leak memory */
				free(sConfig);
			}
		}
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
#ifdef MP4V2
		m_audioTrack = MP4AddAudioTrack(m_mp4File, 
			m_audioTimeScale, m_audioFrameDuration);

		if (!m_audioTrack) {
			// TBD error
		}

		MP4SetAudioProfileLevel(m_mp4File, 0xFE);
#else
		quicktime_set_audio(m_mp4File, 2, 
			m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE), 
			16, 0, m_audioTimeScale, MP3_SAMPLES_PER_FRAME, "mp3 ");

		quicktime_set_iod_audio_profile_level(m_mp4File, 0xFE);
#endif

		m_audioFrameNum = 1;

		m_audioFrameRate = 
			m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE)
			/ MP3_SAMPLES_PER_FRAME;

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
			m_audioPayloadNumber = 14;	// static payload for MPEG Audio

#ifdef MP4V2
			m_audioHintTrack = MP4AddHintTrack(m_mp4File, m_audioTrack);

			// TBD MP4SetHintTrackRtpPayload(m_mp4File, m_audioHintTrack,
			// 	"MPA", &m_audioPayloadNumber,  
			//	m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
#else
			m_audioHintTrack = quicktime_set_audio_hint(m_mp4File, m_audioTrack,
				"MPA", &m_audioPayloadNumber, 
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
#endif

			m_audioRtpPktNum = 1;
			m_audioHintBufLength = 0;
			m_audioFramesThisHint = 0;
			m_audioBytesThisHint = 0;
			m_audioBytesThisSec = 0;
			m_audioFirstRtpPktThisSec = m_audioRtpPktNum;
			m_audioMaxRtpBytesPerSec = 0;

#ifdef MP4V2
			// MP4AddRtpHint(m_mp4File, m_audioHintTrack);
			// MP4AddRtpPacket(m_mp4File, m_audioHintTrack, true);
#else
			quicktime_init_hint_sample(m_audioHintBuf, &m_audioHintBufLength);
			quicktime_add_hint_packet(m_audioHintBuf, &m_audioHintBufLength, 
				m_audioPayloadNumber, m_audioRtpPktNum);

			// RFC 2250 uses RTP M-bit for start of frame
			quicktime_set_hint_Mbit(m_audioHintBuf);
#endif
		}
	}

	m_record = true;
}

void CMp4Recorder::DoStopRecord()
{
	if (!m_record) {
		return;
	}

	// write out any remaining audio hint samples
	if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
		if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
		  && m_audioFramesThisHint > 0) {
#ifdef MP4V2
#else
			quicktime_write_audio_hint(m_mp4File, 
				m_audioHintBuf, m_audioHintBufLength, 
				m_audioTrack, m_audioHintTrack, 
				m_audioFramesThisHint * m_audioFrameDuration);

			// DEBUG
			// quicktime_dump_hint_sample(m_audioHintBuf);
#endif
		}

#ifdef MP4V2
#else
		/* record maximum bits per second in Quicktime file */
		quicktime_set_audio_hint_max_rate(m_mp4File, 
			1000, m_audioMaxRtpBytesPerSec * 8,
			m_audioTrack, m_audioHintTrack);
		quicktime_set_video_hint_max_rate(m_mp4File, 
			1000, m_videoMaxRtpBytesPerSec * 8,
			m_videoTrack, m_videoHintTrack);
#endif
	}

#ifdef MP4V2
	MP4Close(m_mp4File);
#else
	quicktime_close(m_mp4File);
#endif
	m_mp4File = NULL;

	debug_message("MP4 recorder wrote %u audio frames", m_audioFrameNum);
	debug_message("MP4 recorder wrote %u video frames", m_videoFrameNum);

	m_record = false;
}

void CMp4Recorder::DoWriteFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}
	if (!m_record) {
		delete pFrame;
		return;
	}

	if (pFrame->GetType() == CMediaFrame::Mp3AudioFrame) {
		// at start of recording wait for a video I frame
		if (!m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)
		  || m_videoFrameNum > 1) {

#ifdef MP4V2
			MP4WriteSample(
				m_mp4File,
				m_audioTrack,
				(u_int8_t*)pFrame->GetData(), 
				pFrame->GetDataLength());
#else
			quicktime_write_audio_frame(
				m_mp4File,
				(unsigned char*)pFrame->GetData(), 
				pFrame->GetDataLength(),
				m_audioTrack);
#endif

			if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
				Write2250Hints(pFrame);
			}

			m_audioFrameNum++;
		}
	} else if (pFrame->GetType() == CMediaFrame::Mpeg4VideoFrame) {
		u_int8_t* pSample = NULL;
		u_int32_t sampleLength = 0;
		Duration sampleDuration = ConvertVideoDuration(pFrame->GetDuration());
		bool isIFrame = (CVideoSource::GetMpeg4VideoFrameType(pFrame) == 'I');

		// at start of recording wait for an I frame
		if (m_videoFrameNum == 1) {
			if (isIFrame) {
				// for first frame we need to prepend 
				// the MPEG-4 video configuration
				// to meet the requirements of RFC 3016

				sampleLength = 
					m_pConfig->m_videoMpeg4ConfigLength
					+ pFrame->GetDataLength();

				pSample = (u_int8_t*)malloc(sampleLength);
				if (pSample) {
					memcpy(pSample,
						m_pConfig->m_videoMpeg4Config, 
						m_pConfig->m_videoMpeg4ConfigLength);
					memcpy(pSample + m_pConfig->m_videoMpeg4ConfigLength,
						pFrame->GetData(), 
						pFrame->GetDataLength());
				}
			}
			// else ignore the frame
		} else { // m_videoFrameNum > 1
			pSample = (u_int8_t*)pFrame->GetData();
			sampleLength = pFrame->GetDataLength();
		}

		if (pSample != NULL) {
#ifdef MP4V2
			MP4WriteSample(
				m_mp4File,
				m_videoTrack,
				pSample, 
				sampleLength,
				sampleDuration,
				0,
				isIFrame);
#else
			quicktime_write_video_frame(
				m_mp4File, 
				pSample, 
				sampleLength,
				m_videoTrack,
				isIFrame,
				sampleDuration,
				0);
#endif
		
			if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
				Write3016Hints(sampleLength, isIFrame, sampleDuration);
			}

			m_videoFrameNum++;
		}

	} else {
		debug_message("MP4 recorder received unknown frame type %u",
			pFrame->GetType());
	}

	delete pFrame;
}

void CMp4Recorder::Write2250Hints(CMediaFrame* pFrame)
{
	u_int32_t frameLength = pFrame->GetDataLength();

	// if frame is larger than remaining payload size
	if (frameLength + 4 > m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE) 
	  - m_audioBytesThisHint) {

#ifdef MP4V2
#else
		// write out hint sample
		quicktime_write_audio_hint(m_mp4File, 
			m_audioHintBuf, m_audioHintBufLength, 
			m_audioTrack, m_audioHintTrack, 
			m_audioFramesThisHint * m_audioFrameDuration);

		// DEBUG
		// quicktime_dump_hint_sample(m_audioHintBuf);
#endif

		// start a new hint 
		m_audioHintBufLength = 0;
		m_audioFramesThisHint = 0;
		m_audioBytesThisHint = 0;

#ifdef MP4V2
		// MP4AddRtpHint(m_mp4File, m_audioHintTrack);
		// MP4AddRtpPacket(m_mp4File, m_audioHintTrack, true);
#else
		quicktime_init_hint_sample(m_audioHintBuf, &m_audioHintBufLength);
		quicktime_add_hint_packet(m_audioHintBuf, &m_audioHintBufLength, 
			m_audioPayloadNumber, m_audioRtpPktNum);

		// RFC 2250 uses RTP M-bit for start of frame
		quicktime_set_hint_Mbit(m_audioHintBuf);
#endif

		m_audioRtpPktNum++;
	}

	// if frame is less than remaining payload size
	if (frameLength + 4 <= m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)
	  - m_audioBytesThisHint) {
		static u_int32_t zero32 = 0;

#ifdef MP4V2
		//MP4AddRtpImmediateData(m_mp4File, m_audioHintTrack,
		//	(u_int8_t*)&zero32, sizeof(zero32));
		//MP4AddRtpSampleData(m_mp4File, m_audioHintTrack,
		//	m_audioFrameNum, 0, frameLength);
#else
		quicktime_add_hint_immed_data(m_audioHintBuf, &m_audioHintBufLength,
			(u_char*)&zero32, sizeof(zero32));
		quicktime_add_hint_sample_data(m_audioHintBuf, &m_audioHintBufLength, 
			m_audioFrameNum, 0, frameLength);
#endif

		m_audioBytesThisHint += (frameLength + 4);

	} else { // jumbo frame, need to fragment it
		u_int16_t frameOffset = 0;

		while (frameOffset < frameLength) {
			u_int16_t fragLength = MIN(frameLength - frameOffset,
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) - 4;

			u_int8_t payloadHeader[4];
			payloadHeader[0] = payloadHeader[1] = 0;
			payloadHeader[2] = (frameOffset >> 8);
			payloadHeader[3] = frameOffset & 0xFF;

#ifdef MP4V2
#else
			quicktime_add_hint_immed_data(
				m_audioHintBuf, &m_audioHintBufLength,
				(u_char*)&payloadHeader, sizeof(payloadHeader));

			quicktime_add_hint_sample_data(
				m_audioHintBuf, &m_audioHintBufLength, 
				m_audioFrameNum, frameOffset, fragLength);
#endif

			frameOffset += fragLength;

			// if we're not at the last fragment
			if (frameOffset < frameLength) {
#ifdef MP4V2
#else
				// we'll need another RTP packet
				quicktime_add_hint_packet(
					m_audioHintBuf, &m_audioHintBufLength, 
					m_audioPayloadNumber, m_audioRtpPktNum);
				m_audioRtpPktNum++;
#endif
			}
		}

		// lie to ourselves so as to force next frame to output 
		// our hint as is, and start a new hint for itself
		m_audioBytesThisHint = 
			m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
	}

	m_audioFramesThisHint++;
	m_audioBytesThisSec += frameLength;

	if ((m_audioFrameNum % m_audioFrameRate) == 0) {
		u_int32_t rtpPktsThisSec = 
			m_audioRtpPktNum - m_audioFirstRtpPktThisSec;

		m_audioMaxRtpBytesPerSec = MAX(m_audioMaxRtpBytesPerSec,
			(rtpPktsThisSec * RTP_HEADER_STD_SIZE) + m_audioBytesThisSec);
		
		m_audioBytesThisSec = 0;
		m_audioFirstRtpPktThisSec = m_audioRtpPktNum;
	}
}

void CMp4Recorder::Write3016Hints(CMediaFrame* pFrame)
{
	char type = CVideoSource::GetMpeg4VideoFrameType(pFrame);

	Write3016Hints(pFrame->GetDataLength(), type == 'I',
		ConvertVideoDuration(pFrame->GetDuration()));
}

void CMp4Recorder::Write3016Hints(u_int32_t frameLength, 
	bool isIFrame, u_int32_t frameDuration)
{
	u_int32_t offset = 0;
	u_int32_t remaining = frameLength;

	m_videoHintBufLength = 0;
#ifdef MP4V2
	//MP4AddRtpHint(m_mp4File, m_videoHintTrack);
#else
	quicktime_init_hint_sample(m_videoHintBuf, &m_videoHintBufLength);
#endif

	while (remaining) {
		u_int32_t payloadLength;

		if (remaining <= m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {
			payloadLength = remaining;
		} else {
			payloadLength = m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
		}

#ifdef MP4V2
		//MP4AddRtpPacket(m_mp4File, m_videoHintTrack, isLastPacket);
		m_videoRtpPktNum++;
		//MP4AddRtpSampleData(m_mp4File, m_videoHintTrack,
		//	m_videoFrameNum, offset, payloadLength);
#else
		quicktime_add_hint_packet(m_videoHintBuf, &m_videoHintBufLength, 
			m_videoPayloadNumber, m_videoRtpPktNum);
		m_videoRtpPktNum++;

		quicktime_add_hint_sample_data(m_videoHintBuf, &m_videoHintBufLength, 
			m_videoFrameNum, offset, payloadLength);
#endif

		offset += payloadLength;
		remaining -= payloadLength;
	}

#ifdef MP4V2
#else
	quicktime_set_hint_Mbit(m_videoHintBuf);
	quicktime_write_video_hint(m_mp4File, m_videoHintBuf, m_videoHintBufLength, 
		m_videoTrack, m_videoHintTrack, frameDuration, isIFrame);

	// DEBUG quicktime_dump_hint_sample(m_videoHintBuf);
#endif

	m_videoBytesThisSec += frameLength;

	if ((m_videoFrameNum % m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE))
	  == 0) {
		u_int32_t rtpPktsThisSec = 
			m_videoRtpPktNum - m_videoFirstRtpPktThisSec;

		m_videoMaxRtpBytesPerSec = MAX(m_videoMaxRtpBytesPerSec,
			(rtpPktsThisSec * RTP_HEADER_STD_SIZE) + m_videoBytesThisSec);
		
		m_videoBytesThisSec = 0;
		m_videoFirstRtpPktThisSec = m_videoRtpPktNum;
	}
}

