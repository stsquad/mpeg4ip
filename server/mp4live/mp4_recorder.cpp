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

	m_mp4File = MP4Create(
		m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
		0 /* DEBUG MP4_DETAILS_ERROR | MP4_DETAILS_WRITE_ALL */);

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
	MP4SetTimeScale(m_mp4File, m_movieTimeScale);

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		m_videoFrameNum = 1;

		m_videoTrack = MP4AddVideoTrack(m_mp4File,
			m_videoTimeScale,
			MP4_INVALID_DURATION,
			m_pConfig->m_videoWidth, 
			m_pConfig->m_videoHeight,
			MP4_MPEG4_VIDEO_TYPE);

		if (m_videoTrack == MP4_INVALID_TRACK_ID) {
			// TBD error
		}

		MP4SetVideoProfileLevel(m_mp4File, 
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID));

		MP4SetTrackESConfiguration(m_mp4File, m_videoTrack,
			m_pConfig->m_videoMpeg4Config, 
			m_pConfig->m_videoMpeg4ConfigLength); 

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
			m_videoPayloadNumber = 0;	// dynamically assigned

			m_videoHintTrack = MP4AddHintTrack(m_mp4File, m_videoTrack);

			if (m_videoHintTrack == MP4_INVALID_TRACK_ID) {
				// TBD error
			}

			MP4SetHintTrackRtpPayload(m_mp4File, m_videoHintTrack,
			 	"MP4V-ES", &m_videoPayloadNumber,  
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

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
				MP4AppendHintTrackSdp(m_mp4File, m_videoHintTrack, sdpBuf);

				/* cleanup, don't want to leak memory */
				free(sConfig);
			}
		}
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		m_audioTrack = MP4AddAudioTrack(m_mp4File, 
			m_audioTimeScale, 
			m_audioFrameDuration,
			MP4_MP3_AUDIO_TYPE);

		if (m_audioTrack == MP4_INVALID_TRACK_ID) {
			// TBD error
		}

		MP4SetAudioProfileLevel(m_mp4File, 0xFE);

		m_audioFrameNum = 1;

		m_audioFrameRate = 
			m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE)
			/ MP3_SAMPLES_PER_FRAME;

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
			m_audioPayloadNumber = 14;	// static payload for MPEG Audio

			m_audioHintTrack = MP4AddHintTrack(m_mp4File, m_audioTrack);

			if (m_audioHintTrack == MP4_INVALID_TRACK_ID) {
				// TBD error
			}

			MP4SetHintTrackRtpPayload(m_mp4File, m_audioHintTrack,
			 	"MPA", &m_audioPayloadNumber,  
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

			m_audioHintBufLength = 0;
			m_audioFramesThisHint = 0;
			m_audioBytesThisHint = 0;

			MP4AddRtpHint(m_mp4File, m_audioHintTrack);
			MP4AddRtpPacket(m_mp4File, m_audioHintTrack, true);
		}
	}

	// LATER? MP4MakeIsmaCompliant(m_mp4File);

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
			MP4WriteRtpHint(m_mp4File, m_audioHintTrack, 
				m_audioFramesThisHint * m_audioFrameDuration);
		}
	}

	MP4Close(m_mp4File);
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

			MP4WriteSample(
				m_mp4File,
				m_audioTrack,
				(u_int8_t*)pFrame->GetData(), 
				pFrame->GetDataLength());

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

		if (m_videoFrameNum > 1 || isIFrame) {
			pSample = (u_int8_t*)pFrame->GetData();
			sampleLength = pFrame->GetDataLength();
		} // else waiting for I frame at start of recording

		if (pSample != NULL) {
			MP4WriteSample(
				m_mp4File,
				m_videoTrack,
				pSample, 
				sampleLength,
				sampleDuration,
				0,
				isIFrame);
		
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

	if (m_audioFramesThisHint > 0) {
		if (m_audioBytesThisHint + frameLength <= 
		  m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {

			// just add the mp3 frame to the current hint
			MP4AddRtpSampleData(m_mp4File, m_audioHintTrack,
				m_audioFrameNum, 0, frameLength);

			m_audioFramesThisHint++;
			m_audioBytesThisHint += frameLength;
			return;

		} else {
			// write out current hint
			MP4WriteRtpHint(m_mp4File, m_audioHintTrack, 
				m_audioFramesThisHint * m_audioFrameDuration);

			// start a new hint 
			m_audioHintBufLength = 0;
			m_audioFramesThisHint = 0;
			m_audioBytesThisHint = 0;

			MP4AddRtpHint(m_mp4File, m_audioHintTrack);
			MP4AddRtpPacket(m_mp4File, m_audioHintTrack, true);

			// fall thru
		}
	}

	// by here m_audioFramesThisHint == 0

	if (frameLength + 4 <= 
	  m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE)) {
		// add the rfc2250 payload header
		static u_int32_t zero32 = 0;

		MP4AddRtpImmediateData(m_mp4File, m_audioHintTrack,
			(u_int8_t*)&zero32, sizeof(zero32));

		MP4AddRtpSampleData(m_mp4File, m_audioHintTrack,
			m_audioFrameNum, 0, frameLength);

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

			MP4AddRtpImmediateData(m_mp4File, m_audioHintTrack,
				(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

			MP4AddRtpSampleData(m_mp4File, m_audioHintTrack,
				m_audioFrameNum, frameOffset, fragLength);

			frameOffset += fragLength;

			// if we're not at the last fragment
			if (frameOffset < frameLength) {
				MP4AddRtpPacket(m_mp4File, m_audioHintTrack, false);
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
	MP4AddRtpHint(m_mp4File, m_videoHintTrack);

	if (m_videoFrameNum == 1) {
		MP4AddRtpESConfigurationPacket(m_mp4File, m_videoHintTrack);
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

		MP4AddRtpPacket(m_mp4File, m_videoHintTrack, isLastPacket);

		MP4AddRtpSampleData(m_mp4File, m_videoHintTrack,
			m_videoFrameNum, offset, payloadLength);

		offset += payloadLength;
		remaining -= payloadLength;
	}

	MP4WriteRtpHint(m_mp4File, m_videoHintTrack, frameDuration, isIFrame);
}

