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

#ifndef __MEDIA_NODE_H__
#define __MEDIA_NODE_H__

#include <SDL.h>
#include <SDL_thread.h>
#include <msg_queue.h>

#include "live_config.h"
#include "media_frame.h"

class CMediaNode {
public:
	CMediaNode() {
		m_myThread = NULL;
		m_myMsgQueueSemaphore = NULL;
		m_pConfig = NULL;
	}

	static int StartThreadCallback(void* data) {
		return ((CMediaNode*)data)->ThreadMain();
	}

	void StartThread() {
		if (m_myThread == NULL) {
			m_myThread = SDL_CreateThread(StartThreadCallback, this);
			m_myMsgQueueSemaphore = SDL_CreateSemaphore(0);
		}
	}

	void StopThread() {
		if (m_myThread) {
			m_myMsgQueue.send_message(MSG_NODE_STOP_THREAD,
				NULL, 0, m_myMsgQueueSemaphore);
			SDL_WaitThread(m_myThread, NULL);
			m_myThread = NULL;
		}
	}

	virtual ~CMediaNode() {
		StopThread();
	}

	void SetConfig(CLiveConfig* pConfig) {
		m_pConfig = pConfig;
	}

protected:
	static const int MSG_NODE = 1024;
	static const int MSG_NODE_STOP_THREAD = MSG_NODE + 1;;

	virtual int ThreadMain(void) = NULL;

protected:
	SDL_Thread*			m_myThread;
	CMsgQueue			m_myMsgQueue;
	SDL_sem*			m_myMsgQueueSemaphore;

	CLiveConfig*		m_pConfig;
};

class CMediaSink : public CMediaNode {
public:
	CMediaSink() {
	}

	void EnqueueFrame(CMediaFrame* pFrame) {
		pFrame->AddReference();
		m_myMsgQueue.send_message(MSG_SINK_FRAME, 
			(unsigned char*)pFrame, sizeof(CMediaFrame),
			m_myMsgQueueSemaphore);
	}

protected:
	static const int MSG_SINK = 2048;
	static const int MSG_SINK_FRAME = MSG_SINK + 1;
};

class CMediaSource : public CMediaNode {
public:
	CMediaSource() {
	}
	
	// TBD for safety add mutex around sink list

	bool AddSink(CMediaSink* pSink) {
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == NULL) {
				m_sinks[i] = pSink;
				return true;
			}
		}
		return false;
	}
	void RemoveSink(CMediaSink* pSink) {
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == pSink) {
				int j;
				for (j = i; j < MAX_SINKS - 1; j++) {
					m_sinks[j] = m_sinks[j+1];
				}
				m_sinks[j] = NULL;
				return;
			}
		}
	}
	void RemoveAllSinks() {
		for (int i = 0; i < MAX_SINKS; i++) {
			m_sinks[i] = NULL;
		}
	}

protected:
	void ForwardFrame(CMediaFrame* pFrame) {
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == NULL) {
				return;
			}
			m_sinks[i]->EnqueueFrame(pFrame);
		}
	}

protected:
	static const u_int16_t MAX_SINKS = 8;
	CMediaSink* m_sinks[MAX_SINKS];
};

#endif /* __MEDIA_NODE_H__ */

