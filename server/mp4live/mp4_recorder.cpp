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
#include "mp4_recorder.h"
#include "video_source.h"
#include "mp3.h"
#include "transcoder.h"


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

	// enable huge file mode in mp4 if estimated size goes over 1 GB
	bool hugeFile = m_pConfig->m_recordEstFileSize > 1000000000;

	m_mp4File = MP4Create(
		m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
		MP4_DETAILS_ERROR /* DEBUG | MP4_DETAILS_WRITE_ALL */,
		hugeFile);

	if (!m_mp4File) {
		return;
	}

	m_canRecordAudio = true;

	m_rawAudioTimeScale = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);

	m_encodedAudioTimeScale = 
		m_pConfig->m_audioEncodedSampleRate;
	m_encodedAudioFrameDuration = MP4_INVALID_DURATION;

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)
	  && (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)
	    || m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO))) {
		m_movieTimeScale = m_videoTimeScale;

	} else if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {
		m_movieTimeScale = m_encodedAudioTimeScale;

	} else {
		m_movieTimeScale = m_rawAudioTimeScale;
	}
	MP4SetTimeScale(m_mp4File, m_movieTimeScale);

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {
			m_rawVideoFrameNum = 1;
			m_canRecordAudio = false;

			m_rawVideoTrackId = MP4AddVideoTrack(
				m_mp4File,
				m_videoTimeScale,
				MP4_INVALID_DURATION,
				m_pConfig->m_videoWidth, 
				m_pConfig->m_videoHeight,
				MP4_YUV12_VIDEO_TYPE);

			if (m_rawVideoTrackId == MP4_INVALID_TRACK_ID) {
				error_message("can't create raw video track");
				goto start_failure;
			}

			MP4SetVideoProfileLevel(m_mp4File, 0xFF);
		}

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {
			m_encodedVideoFrameNum = 1;
			m_canRecordAudio = false;

			m_encodedVideoTrackId = MP4AddVideoTrack(
				m_mp4File,
				m_videoTimeScale,
				MP4_INVALID_DURATION,
				m_pConfig->m_videoWidth, 
				m_pConfig->m_videoHeight,
				MP4_MPEG4_VIDEO_TYPE);

			if (m_encodedVideoTrackId == MP4_INVALID_TRACK_ID) {
				error_message("can't create encoded video track");
				goto start_failure;
			}

			MP4SetVideoProfileLevel(m_mp4File, 
				m_pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID));

			MP4SetTrackESConfiguration(m_mp4File, m_encodedVideoTrackId,
				m_pConfig->m_videoMpeg4Config, 
				m_pConfig->m_videoMpeg4ConfigLength); 

			if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
				m_videoPayloadNumber = 0;	// dynamically assigned

				m_videoHintTrackId = 
					MP4AddHintTrack(m_mp4File, m_encodedVideoTrackId);

				if (m_videoHintTrackId == MP4_INVALID_TRACK_ID) {
					error_message("can't create video hint track");
					goto start_failure;
				}

				MP4SetHintTrackRtpPayload(
					m_mp4File, 
					m_videoHintTrackId,
					"MP4V-ES", 
					&m_videoPayloadNumber,  
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

				/* convert the mpeg4 configuration to ASCII form */
				char* sConfig = BinaryToAscii(
					m_pConfig->m_videoMpeg4Config, 
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
					MP4AppendHintTrackSdp(m_mp4File, 
						m_videoHintTrackId, 
						sdpBuf);

					/* cleanup, don't want to leak memory */
					free(sConfig);
				}
			}
		}
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {
			m_rawAudioFrameNum = 1;

			m_rawAudioTrackId = MP4AddAudioTrack(
				m_mp4File, 
				m_rawAudioTimeScale, 
				0,
				MP4_PCM16_AUDIO_TYPE);

			if (m_rawAudioTrackId == MP4_INVALID_TRACK_ID) {
				error_message("can't create raw audio track");
				goto start_failure;
			}

			MP4SetAudioProfileLevel(m_mp4File, 0xFF);
		}

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {
			m_encodedAudioFrameNum = 1;

			u_int8_t audioType;
			char* audioPayloadName;

			if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
			  AUDIO_ENCODING_AAC)) {
				audioType = MP4_MPEG4_AUDIO_TYPE;
				MP4SetAudioProfileLevel(m_mp4File, 0x0F);
				audioPayloadName = "mpeg4-generic"; 
			} else {
				audioType = MP4_MP3_AUDIO_TYPE;
				MP4SetAudioProfileLevel(m_mp4File, 0xFE);
				audioPayloadName = "MPA"; 
			}

			m_encodedAudioTrackId = MP4AddAudioTrack(
				m_mp4File, 
				m_encodedAudioTimeScale, 
				m_encodedAudioFrameDuration,
				audioType);

			if (m_encodedAudioTrackId == MP4_INVALID_TRACK_ID) {
				error_message("can't create encoded audio track");
				goto start_failure;
			}

			if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
				m_audioPayloadNumber = 0;	// dynamic payload

				m_audioHintTrackId = 
					MP4AddHintTrack(m_mp4File, m_encodedAudioTrackId);

				if (m_audioHintTrackId == MP4_INVALID_TRACK_ID) {
					error_message("can't create audio hint track");
					goto start_failure;
				}

				MP4SetHintTrackRtpPayload(
					m_mp4File, 
					m_audioHintTrackId,
					audioPayloadName,
					&m_audioPayloadNumber,  
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

				m_audioHintBufLength = 0;
				m_audioFramesThisHint = 0;
				m_audioBytesThisHint = 0;

				MP4AddRtpHint(m_mp4File, m_audioHintTrackId);
				MP4AddRtpPacket(m_mp4File, m_audioHintTrackId, true);
			}
		}
	}

	m_record = true;
	return;

start_failure:
	MP4Close(m_mp4File);
	m_mp4File = NULL;
	return;
}

void CMp4Recorder::DoStopRecord()
{
	if (!m_record) {
		return;
	}

	// write out any remaining audio hint samples
	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)
	  && m_audioFramesThisHint > 0) {

		MP4WriteRtpHint(m_mp4File, m_audioHintTrackId, 
			m_audioFramesThisHint * m_encodedAudioFrameDuration);
	}

	MP4Close(m_mp4File);
	m_mp4File = NULL;

	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)
	  || m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {

		bool useIsmaTag = true;

	  	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)
		  && !strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
		    AUDIO_ENCODING_MP3)) {

			useIsmaTag = false;
		}

		MP4MakeIsmaCompliant(
			m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
			0, useIsmaTag);
	}

	// TEMP solution to AAC hint tracks
	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)
	  && !strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
		 AUDIO_ENCODING_AAC)) {

			CTranscoder::HintTrack(
				m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
				m_encodedAudioTrackId);
	}

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

	if (pFrame->GetType() == CMediaFrame::PcmAudioFrame
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO)) {

		if (m_canRecordAudio) {
			MP4WriteSample(
				m_mp4File,
				m_rawAudioTrackId,
				(u_int8_t*)pFrame->GetData(), 
				pFrame->GetDataLength(),
				pFrame->GetDataLength() / 4);

			m_rawAudioFrameNum++;
		}

	} else if ((pFrame->GetType() == CMediaFrame::Mp3AudioFrame
	    || pFrame->GetType() == CMediaFrame::AacAudioFrame)
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {

		if (m_canRecordAudio) {

			if (m_encodedAudioFrameDuration == MP4_INVALID_DURATION) {
				m_encodedAudioFrameDuration = 
					m_pConfig->m_audioEncodedSamplesPerFrame;
			}

			MP4WriteSample(
				m_mp4File,
				m_encodedAudioTrackId,
				(u_int8_t*)pFrame->GetData(), 
				pFrame->GetDataLength(),
				m_encodedAudioFrameDuration);

			if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
				if (!strcasecmp(m_pConfig->GetStringValue(
				  CONFIG_AUDIO_ENCODING), AUDIO_ENCODING_AAC)) {
					// TBD AAC
				} else {
					Write2250Hints(pFrame);
				}
			}

			m_encodedAudioFrameNum++;
		}

	} else if (pFrame->GetType() == CMediaFrame::RawYuvVideoFrame
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO)) {

		// let audio record if raw is the only video being recorded
		if (!m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {
			m_canRecordAudio = true;
		}

		MP4WriteSample(
			m_mp4File,
			m_rawVideoTrackId,
			(u_int8_t*)pFrame->GetData(), 
			pFrame->GetDataLength(),
			ConvertVideoDuration(pFrame->GetDuration()));

		m_rawVideoFrameNum++;

	} else if (pFrame->GetType() == CMediaFrame::Mpeg4VideoFrame
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {

		u_int8_t* pSample = NULL;
		u_int32_t sampleLength = 0;
		Duration sampleDuration = 
			ConvertVideoDuration(pFrame->GetDuration());
		bool isIFrame = 
			(CVideoSource::GetMpeg4VideoFrameType(pFrame) == 'I');

		if (m_encodedVideoFrameNum > 1 || isIFrame) {
			pSample = (u_int8_t*)pFrame->GetData();
			sampleLength = pFrame->GetDataLength();

			m_canRecordAudio = true;

		} // else waiting for I frame at start of recording

		if (pSample != NULL) {
			MP4WriteSample(
				m_mp4File,
				m_encodedVideoTrackId,
				pSample, 
				sampleLength,
				sampleDuration,
				0,
				isIFrame);
		
			if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
				Write3016Hints(sampleLength, isIFrame, sampleDuration);
			}

			m_encodedVideoFrameNum++;
		}
	}

	delete pFrame;
}

void CMp4Recorder::Write2250Hints(CMediaFrame* pFrame)
{
	u_int32_t frameLength = pFrame->GetDataLength();

	if (m_audioFramesThisHint > 0) {
		if (m_audioBytesThisHint + frameLength <= 
		  m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {

			// just add the mp3 frame to the current hint
			MP4AddRtpSampleData(m_mp4File, m_audioHintTrackId,
				m_encodedAudioFrameNum, 0, frameLength);

			m_audioFramesThisHint++;
			m_audioBytesThisHint += frameLength;
			return;

		} else {
			// write out current hint
			MP4WriteRtpHint(m_mp4File, m_audioHintTrackId, 
				m_audioFramesThisHint * m_encodedAudioFrameDuration);

			// start a new hint 
			m_audioHintBufLength = 0;
			m_audioFramesThisHint = 0;
			m_audioBytesThisHint = 0;

			MP4AddRtpHint(m_mp4File, m_audioHintTrackId);
			MP4AddRtpPacket(m_mp4File, m_audioHintTrackId, true);

			// fall thru
		}
	}

	// by here m_audioFramesThisHint == 0

	if (frameLength + 4 <= 
	  m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {
		// add the rfc2250 payload header
		static u_int32_t zero32 = 0;

		MP4AddRtpImmediateData(m_mp4File, m_audioHintTrackId,
			(u_int8_t*)&zero32, sizeof(zero32));

		MP4AddRtpSampleData(m_mp4File, m_audioHintTrackId,
			m_encodedAudioFrameNum, 0, frameLength);

		m_audioBytesThisHint += (frameLength + 4);
	} else {
		// jumbo frame, need to fragment it
		u_int16_t frameOffset = 0;

		while (frameOffset < frameLength) {
			u_int16_t fragLength = MIN(frameLength - frameOffset,
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) - 4;

			u_int8_t payloadHeader[4];
			payloadHeader[0] = payloadHeader[1] = 0;
			payloadHeader[2] = (frameOffset >> 8);
			payloadHeader[3] = frameOffset & 0xFF;

			MP4AddRtpImmediateData(m_mp4File, m_audioHintTrackId,
				(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

			MP4AddRtpSampleData(m_mp4File, m_audioHintTrackId,
				m_encodedAudioFrameNum, frameOffset, fragLength);

			frameOffset += fragLength;

			// if we're not at the last fragment
			if (frameOffset < frameLength) {
				MP4AddRtpPacket(m_mp4File, m_audioHintTrackId, false);
			}
		}

		// lie to ourselves so as to force next frame to output 
		// our hint as is, and start a new hint for itself
		m_audioBytesThisHint = 
			m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
	}

	m_audioFramesThisHint = 1;
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
	MP4AddRtpHint(m_mp4File, m_videoHintTrackId);

	if (m_encodedVideoFrameNum == 1) {
		MP4AddRtpESConfigurationPacket(m_mp4File, m_videoHintTrackId);
	}

	while (remaining) {
		bool isLastPacket = false;
		u_int32_t payloadLength;

		if (remaining <= m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {
			payloadLength = remaining;
			isLastPacket = true;
		} else {
			payloadLength = m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
		}

		MP4AddRtpPacket(m_mp4File, m_videoHintTrackId, isLastPacket);

		MP4AddRtpSampleData(m_mp4File, m_videoHintTrackId,
			m_encodedVideoFrameNum, offset, payloadLength);

		offset += payloadLength;
		remaining -= payloadLength;
	}

	MP4WriteRtpHint(m_mp4File, m_videoHintTrackId, frameDuration, isIFrame);
}

