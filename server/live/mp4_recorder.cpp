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

	m_mp4File = quicktime_open(m_pConfig->m_recordMp4FileName, 0, 1, 0);

	if (!m_mp4File) {
		return;
	}

	quicktime_set_time_scale(m_mp4File, m_videoTimeScale);

	if (m_pConfig->m_audioEnable) {
		m_audioTimeScale = m_pConfig->m_audioSamplingRate;

		quicktime_set_audio(m_mp4File, 2, m_pConfig->m_audioSamplingRate, 16, 
			0, m_audioTimeScale, m_audioTimeScale / 1152, "mp3 ");

		m_audioPayloadNumber = 14;

		m_audioHintTrack = quicktime_set_audio_hint(m_mp4File, m_audioTrack,
			"MPA", &m_audioPayloadNumber, m_pConfig->m_rtpPayloadSize);
	}

	if (m_pConfig->m_videoEnable) {
		quicktime_set_video(m_mp4File, 1, 
			m_pConfig->m_videoWidth, m_pConfig->m_videoHeight,
			m_pConfig->m_videoTargetFrameRate, m_videoTimeScale,
			"mp4v");

		// always simple profile for now
		quicktime_set_iod_video_profile_level(m_mp4File, 3);

		m_videoPayloadNumber = 0;	// dynamically assigned

		m_videoHintTrack = quicktime_set_video_hint(m_mp4File, m_videoTrack, 
			"MP4V-ES", &m_videoPayloadNumber, m_pConfig->m_rtpPayloadSize);
	}

	m_record = true;
}

void CMp4Recorder::DoStopRecord()
{
	if (!m_record) {
		return;
	}

	// TBD write out any remaining hint samples

	quicktime_close(m_mp4File);
	m_mp4File = NULL;

	m_record = false;
}

void CMp4Recorder::DoWriteFrame(CMediaFrame* pFrame)
{
	if (!m_record) {
		delete pFrame;
		return;
	}

	if (pFrame->GetType() == CMediaFrame::Mp3AudioFrame) {
		quicktime_write_audio_frame(m_mp4File,
			(unsigned char*)pFrame->GetData(), pFrame->GetDataLength(), 
			m_audioTrack);

		// TBD audio hint sample
	}

	if (pFrame->GetType() == CMediaFrame::Mpeg4VideoFrame) {
		quicktime_write_video_frame(m_mp4File, 
			(unsigned char*)pFrame->GetData(), pFrame->GetDataLength(), 
			m_videoTrack, 0, 0);

		// TBD video hint sample
	}

	delete pFrame;
}

