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
	if (m_pConfig->m_audioEnable) {
		m_pcmFile = open(m_pConfig->m_recordPcmFileName, 
			O_CREAT | O_WRONLY,	S_IRUSR | S_IWUSR);
		if (m_pcmFile == -1) {
			error_message("Failed to open %s", m_pConfig->m_recordPcmFileName);
		}
	}

	if (m_pConfig->m_videoEnable) {
		m_yuvFile = open(m_pConfig->m_recordYuvFileName,
			O_CREAT | O_WRONLY,	S_IRUSR | S_IWUSR);
		if (m_yuvFile == -1) {
			error_message("Failed to open %s", m_pConfig->m_recordYuvFileName);
		}
	}
	m_record = true;
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
	if (pFrame->GetType() == CMediaFrame::PcmAudioFrame) {
		write(m_pcmFile, pFrame->GetData(), pFrame->GetDataLength());
	}

	if (pFrame->GetType() == CMediaFrame::YuvVideoFrame) {
		write(m_yuvFile, pFrame->GetData(), pFrame->GetDataLength());
	}

	delete pFrame;
}

