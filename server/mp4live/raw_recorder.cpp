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
#include "raw_recorder.h"

int CRawRecorder::ThreadMain(void) 
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

void CRawRecorder::DoStartRecord()
{
	if (m_record) {
		return;
	}
	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		m_pcmFile = open(
			m_pConfig->GetStringValue(CONFIG_RECORD_PCM_FILE_NAME), 
			O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
		if (m_pcmFile == -1) {
			error_message("Failed to open %s", 
				m_pConfig->GetStringValue(CONFIG_RECORD_PCM_FILE_NAME)); 
		}
	}

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		m_yuvFile = open(
			m_pConfig->GetStringValue(CONFIG_RECORD_YUV_FILE_NAME), 
			O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
		if (m_yuvFile == -1) {
			error_message("Failed to open %s", 
				m_pConfig->GetStringValue(CONFIG_RECORD_YUV_FILE_NAME)); 
		}
	}
	if (m_pcmFile != -1 || m_yuvFile != -1) {
		m_record = true;
	}
}

void CRawRecorder::DoStopRecord()
{
	if (!m_record) {
		return;
	}

	if (m_pcmFile != -1) {
		close(m_pcmFile);
		m_pcmFile = -1;
	}
	if (m_yuvFile != -1) {
		close(m_yuvFile);
		m_yuvFile = -1;
	}

	m_record = false;
}

void CRawRecorder::DoWriteFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}

	if (m_record) {
		if (pFrame->GetType() == CMediaFrame::PcmAudioFrame 
		  && m_pcmFile != -1) {
			write(m_pcmFile, pFrame->GetData(), pFrame->GetDataLength());

		} else if (pFrame->GetType() == CMediaFrame::RawYuvVideoFrame 
		  && m_yuvFile != -1) {
			write(m_yuvFile, pFrame->GetData(), pFrame->GetDataLength());
		} else {
			// debug_message("Raw recorder received unknown frame type %u",
			//	pFrame->GetType());
		}
	}
	
	delete pFrame;
}

