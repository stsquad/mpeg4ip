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
#include "file_mp4_recorder.h"
#include "video_v4l_source.h"


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
			case MSG_NODE_START:
				DoStartRecord();
				break;
			case MSG_NODE_STOP:
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
	if (m_sink) {
		return;
	}

	// enable huge file mode in mp4 if estimated size goes over 1 GB
	bool hugeFile = 
		m_pConfig->m_recordEstFileSize > 1000000000;
	u_int32_t verbosity =
		MP4_DETAILS_ERROR /* | MP4_DETAILS_WRITE_ALL */;

	if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_OVERWRITE)) {
		m_mp4File = MP4Create(
			m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
			verbosity, hugeFile);
	} else {
		m_mp4File = MP4Modify(
			m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
			verbosity);
	}

	if (!m_mp4File) {
		return;
	}

	m_rawVideoTrackId = MP4_INVALID_TRACK_ID;
	m_encodedVideoTrackId = MP4_INVALID_TRACK_ID;
	m_rawAudioTrackId = MP4_INVALID_TRACK_ID;
	m_encodedAudioTrackId = MP4_INVALID_TRACK_ID;

	m_canRecordAudio = true;

	m_rawAudioTimeScale = m_encodedAudioTimeScale = 
		m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);

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

			if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
			  AUDIO_ENCODING_AAC)) {
				audioType = MP4_MPEG4_AUDIO_TYPE;
				MP4SetAudioProfileLevel(m_mp4File, 0x0F);
			} else {
				audioType = MP4_MP3_AUDIO_TYPE;
				MP4SetAudioProfileLevel(m_mp4File, 0xFE);
			}

			m_encodedAudioTrackId = MP4AddAudioTrack(
				m_mp4File, 
				m_encodedAudioTimeScale, 
				MP4_INVALID_DURATION,
				audioType);

			if (m_encodedAudioTrackId == MP4_INVALID_TRACK_ID) {
				error_message("can't create encoded audio track");
				goto start_failure;
			}

			if (!strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
			  AUDIO_ENCODING_AAC)) {
				u_int8_t* pConfig = NULL;
				u_int32_t configLength = 0;

				MP4AV_AacGetConfiguration(
					&pConfig,
					&configLength,
					MP4AV_AAC_LC_PROFILE,
					m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE),
					m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS));

				MP4SetTrackESConfiguration(
					m_mp4File, 
					m_encodedAudioTrackId,
					pConfig, 
					configLength);
			}
		}
	}

	m_sink = true;
	return;

start_failure:
	MP4Close(m_mp4File);
	m_mp4File = NULL;
	return;
}

void CMp4Recorder::DoWriteFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}
	if (!m_sink) {
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
			MP4WriteSample(
				m_mp4File,
				m_encodedAudioTrackId,
				(u_int8_t*)pFrame->GetData(), 
				pFrame->GetDataLength(),
				pFrame->ConvertDuration(m_encodedAudioTimeScale));

			m_encodedAudioFrameNum++;
		}

	} else if (pFrame->GetType() == CMediaFrame::YuvVideoFrame
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
			pFrame->ConvertDuration(m_videoTimeScale));

		m_rawVideoFrameNum++;

	} else if (pFrame->GetType() == CMediaFrame::Mpeg4VideoFrame
	  && m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)) {

		u_int8_t* pSample = NULL;
		u_int32_t sampleLength = 0;
		Duration sampleDuration = 
			pFrame->ConvertDuration(m_videoTimeScale);
		bool isIFrame = (MP4AV_Mpeg4GetVopType(
			(u_int8_t*)pFrame->GetData(), pFrame->GetDataLength()) == 'I');

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
		
			m_encodedVideoFrameNum++;
		}
	}

	delete pFrame;
}

void CMp4Recorder::DoStopRecord()
{
	if (!m_sink) {
		return;
	}

	bool optimize = false;

	// create hint tracks
	if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {

		if (m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_OPTIMIZE)) {
			optimize = true;
		}

		if (MP4_IS_VALID_TRACK_ID(m_encodedVideoTrackId)) {
			MP4AV_Rfc3016Hinter(
				m_mp4File, 
				m_encodedVideoTrackId,
				m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

			// LATER H.26L hinter when we have a real-time H.26L encoder
		}

		if (MP4_IS_VALID_TRACK_ID(m_encodedAudioTrackId)) {
			const char *encoding = 
				m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING);

			if (!strcasecmp(encoding, AUDIO_ENCODING_MP3)) {
				MP4AV_Rfc2250Hinter(
					m_mp4File, 
					m_encodedAudioTrackId, 
					false, 
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));

			} else if (!strcasecmp(encoding, AUDIO_ENCODING_AAC)) {
				MP4AV_RfcIsmaHinter(
					m_mp4File, 
					m_encodedAudioTrackId, 
					false, 
					m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
			}
		}
	}

	// close the mp4 file
	MP4Close(m_mp4File);
	m_mp4File = NULL;

	// add ISMA style OD and Scene tracks
	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO)
	  || m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)) {

		bool useIsmaTag = false;

		// if AAC track is present, can tag this as ISMA compliant content
	  	if (m_pConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO)
		  && !strcasecmp(m_pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
		    AUDIO_ENCODING_AAC)) {

			useIsmaTag = true;
		}

		MP4MakeIsmaCompliant(
			m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME),
			0,
			useIsmaTag);
	}

	if (optimize) {
		MP4Optimize(m_pConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME));
	}

	m_sink = false;
}

