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
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "file_raw_sink.h"

int CRawFileSink::ThreadMain(void) 
{
	while (SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
		CMsg* pMsg = m_myMsgQueue.get_message();
		
		if (pMsg != NULL) {
			switch (pMsg->get_value()) {
			case MSG_NODE_STOP_THREAD:
				DoStopSink();
				delete pMsg;
				return 0;
			case MSG_NODE_START:
				DoStartSink();
				break;
			case MSG_NODE_STOP:
				DoStopSink();
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

void CRawFileSink::DoStartSink()
{
	if (m_sink) {
		return;
	}

	if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		m_pcmFile = OpenFile(
			m_pConfig->GetStringValue(CONFIG_RAW_PCM_FILE_NAME), 
			m_pConfig->GetBoolValue(CONFIG_RAW_PCM_FIFO));
	}

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		m_yuvFile = OpenFile(
			m_pConfig->GetStringValue(CONFIG_RAW_YUV_FILE_NAME), 
			m_pConfig->GetBoolValue(CONFIG_RAW_YUV_FIFO));
	}

	if (m_pcmFile != -1 || m_yuvFile != -1) {
		m_sink = true;
	}
}

int CRawFileSink::OpenFile(char* fileName, bool useFifo)
{
	int fd = -1;
	int openFlags = O_CREAT | O_TRUNC | O_WRONLY;
	int mode = S_IRUSR | S_IWUSR | S_IRGRP;

	if (useFifo) {
		int rc;
		struct stat stats;

		rc = stat(fileName, &stats);

		if (rc == 0) {
			if (S_ISFIFO(stats.st_mode)) {
				openFlags = O_RDWR | O_NONBLOCK;
			} else {
				error_message(
					"Warning: %s exists but is not a fifo (named pipe)\n",
					fileName);
			}
		} else {
			rc = mkfifo(fileName, mode);

			if (rc != 0) {
				error_message(
					"Can't create fifo (named pipe) %s: %s\n",
					fileName, strerror(errno));
				return -1;
			}
			openFlags = O_RDWR | O_NONBLOCK;
		}
	}

	fd = open(fileName, openFlags, mode);

	if (fd == -1) {
		error_message("Failed to open %s: %s", 
			fileName, strerror(errno));
	}

	return fd;
}

void CRawFileSink::DoStopSink()
{
	if (!m_sink) {
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

	m_sink = false;
}

void CRawFileSink::DoWriteFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}

	if (m_sink) {
		if (pFrame->GetType() == PCMAUDIOFRAME 
		  && m_pcmFile != -1) {
			write(m_pcmFile, pFrame->GetData(), pFrame->GetDataLength());

		} else if (pFrame->GetType() == YUVVIDEOFRAME 
		  && m_yuvFile != -1) {
			write(m_yuvFile, pFrame->GetData(), pFrame->GetDataLength());
		}
	}
	
	if (pFrame->RemoveReference())
	  delete pFrame;
}

