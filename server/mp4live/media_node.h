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

#include "mp4live_config.h"
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
			m_myMsgQueueSemaphore = SDL_CreateSemaphore(0);
			m_myThread = SDL_CreateThread(StartThreadCallback, this);
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

	void Start(void) {
		m_myMsgQueue.send_message(MSG_NODE_START,
			 NULL, 0, m_myMsgQueueSemaphore);
	}

	void Stop(void) {
		m_myMsgQueue.send_message(MSG_NODE_STOP,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	virtual ~CMediaNode() {
		StopThread();
	}

	void SetConfig(CLiveConfig* pConfig) {
		m_pConfig = pConfig;
	}

protected:
	static const int MSG_NODE				= 1024;
	static const int MSG_NODE_START			= MSG_NODE + 1;
	static const int MSG_NODE_STOP			= MSG_NODE + 2;
	static const int MSG_NODE_STOP_THREAD	= MSG_NODE + 3;

	virtual int ThreadMain(void) = NULL;

protected:
	SDL_Thread*			m_myThread;
	CMsgQueue			m_myMsgQueue;
	SDL_sem*			m_myMsgQueueSemaphore;

	CLiveConfig*		m_pConfig;
};

class CMediaSink : public CMediaNode {
public:
	CMediaSink() : CMediaNode() {
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
			(unsigned char*)pFrame, 0,
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

class CAudioEncoder;	// forward declaration

class CMediaSource : public CMediaNode {
public:
	CMediaSource() : CMediaNode() {
		m_pSinksMutex = SDL_CreateMutex();
		if (m_pSinksMutex == NULL) {
			debug_message("CreateMutex error");
		}
		for (int i = 0; i < MAX_SINKS; i++) {
			m_sinks[i] = NULL;
		}
	}

	~CMediaSource() {
		SDL_DestroyMutex(m_pSinksMutex);
		m_pSinksMutex = NULL;
	}
	
	bool AddSink(CMediaSink* pSink);

	void RemoveSink(CMediaSink* pSink);

	void RemoveAllSinks(void);

	void StartVideo(void) {
		m_myMsgQueue.send_message(MSG_SOURCE_START_VIDEO,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	void StartAudio(void) {
		m_myMsgQueue.send_message(MSG_SOURCE_START_AUDIO,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	void GenerateKeyFrame(void) {
		m_myMsgQueue.send_message(MSG_SOURCE_KEY_FRAME,
			NULL, 0, m_myMsgQueueSemaphore);
	}

	virtual u_int32_t GetNumEncodedVideoFrames() {
		return 0;
	}

	virtual u_int32_t GetNumEncodedAudioFrames() {
		return 0;
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

	void ForwardEncodedAudioFrames(
		CAudioEncoder* encoder,
		Timestamp baseTimestamp,
		u_int32_t* pNumSamples,
		u_int32_t* pNumFrames);

	Duration SamplesToTicks(u_int32_t numSamples) {
		return (numSamples * TimestampTicks)
			/ m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);
	}

protected:
	static const int MSG_SOURCE_START_VIDEO	= 1;
	static const int MSG_SOURCE_START_AUDIO	= 2;
	static const int MSG_SOURCE_KEY_FRAME	= 3;

	static const u_int16_t MAX_SINKS = 8;
	CMediaSink* m_sinks[MAX_SINKS];
	SDL_mutex*	m_pSinksMutex;
};

#endif /* __MEDIA_NODE_H__ */

