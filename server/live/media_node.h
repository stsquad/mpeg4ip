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
			SDL_DestroySemaphore(m_myMsgQueueSemaphore);
			m_myMsgQueueSemaphore = NULL;
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
		m_pEnqueueMutex = SDL_CreateMutex();
		if (m_pEnqueueMutex == NULL) {
			debug_message("EnqueueFrame CreateMutex error");
		}
	}

	~CMediaSink() {
		SDL_DestroyMutex(m_pEnqueueMutex);
	}

	void EnqueueFrame(CMediaFrame* pFrame) {
		if (pFrame == NULL) {
			debug_message("EnqueueFrame: got NULL frame!?");
			return;
		}
		if (SDL_LockMutex(m_pEnqueueMutex) == -1) {
			debug_message("EnqueueFrame LockMutex error");
			return;
		}

		pFrame->AddReference();
		m_myMsgQueue.send_message(MSG_SINK_FRAME, 
			(unsigned char*)pFrame, sizeof(CMediaFrame),
			m_myMsgQueueSemaphore);

		if (SDL_UnlockMutex(m_pEnqueueMutex) == -1) {
			debug_message("EnqueueFrame UnlockMutex error");
		}
	}

protected:
	static const int MSG_SINK = 2048;
	static const int MSG_SINK_FRAME = MSG_SINK + 1;

	SDL_mutex*	m_pEnqueueMutex;
};

class CMediaSource : public CMediaNode {
public:
	CMediaSource() {
		m_pSinksMutex = SDL_CreateMutex();
		if (m_pSinksMutex == NULL) {
			debug_message("CreateMutex error");
		}
	}

	~CMediaSource() {
		SDL_DestroyMutex(m_pSinksMutex);
		m_pSinksMutex = NULL;
	}
	
	bool AddSink(CMediaSink* pSink) {
		bool rc = false;

		if (SDL_LockMutex(m_pSinksMutex) == -1) {
			debug_message("AddSink LockMutex error");
			return rc;
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == NULL) {
				m_sinks[i] = pSink;
				rc = true;
				break;
			}
		}
		if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
			debug_message("UnlockMutex error");
		}
		return rc;
	}

	void RemoveSink(CMediaSink* pSink) {
		if (SDL_LockMutex(m_pSinksMutex) == -1) {
			debug_message("RemoveSink LockMutex error");
			return;
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == pSink) {
				int j;
				for (j = i; j < MAX_SINKS - 1; j++) {
					m_sinks[j] = m_sinks[j+1];
				}
				m_sinks[j] = NULL;
				break;
			}
		}
		if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
			debug_message("UnlockMutex error");
		}
	}

	void RemoveAllSinks(void) {
		if (SDL_LockMutex(m_pSinksMutex) == -1) {
			debug_message("RemoveAllSinks LockMutex error");
			return;
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == NULL) {
				break;
			}
			m_sinks[i] = NULL;
		}
		if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
			debug_message("UnlockMutex error");
		}
	}


protected:
	void ForwardFrame(CMediaFrame* pFrame) {
		if (SDL_LockMutex(m_pSinksMutex) == -1) {
			debug_message("ForwardFrame LockMutex error");
			return;
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			if (m_sinks[i] == NULL) {
				break;
			}
			m_sinks[i]->EnqueueFrame(pFrame);
		}
		if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
			debug_message("UnlockMutex error");
		}
	}

protected:
	static const u_int16_t MAX_SINKS = 8;
	CMediaSink* m_sinks[MAX_SINKS];
	SDL_mutex*	m_pSinksMutex;
};

#endif /* __MEDIA_NODE_H__ */

