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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *			Cesar Gonzalez		cesar@eureka-sistemas.com
 */

#ifndef __MEDIA_OUTPUT_H__
#define __MEDIA_OUTPUT_H__

#include <sys/types.h>

#include "media_source.h"
#include "media_frame.h"

class CLoopSource : public CMediaSource {
public:
	CLoopSource(bool isAudioSource) : CMediaSource() {
		m_sink = false;
		m_isAudioSource=isAudioSource;
		m_pEnqueueMutex = SDL_CreateMutex();
		if (m_pEnqueueMutex == NULL) {
			debug_message("EnqueueFrame CreateMutex error");
		}
	}

	~CLoopSource() {
		SDL_DestroyMutex(m_pEnqueueMutex);
	}

	bool Init(void);
	
	bool IsDone() {
		return false;	// live capture is inexhaustible
	}

	float GetProgress() {
		return 0.0;		// live capture device is inexhaustible
	}

	void EnqueueFrame(CMediaFrame* pFrame) {
		if (pFrame == NULL) {
			debug_message("EnqueueRawFrame: got NULL frame!?");
			return;
		}

		if (SDL_LockMutex(m_pEnqueueMutex) == -1) {
			debug_message("EnqueueRawFrame LockMutex error");
			return;
		}

		m_myMsgQueue.send_message(MSG_INPUT_FRAME,
			(CMediaFrame*)pFrame, 0,
			m_myMsgQueueSemaphore);

		if (SDL_UnlockMutex(m_pEnqueueMutex) == -1) {
			debug_message("EnqueueFrame UnlockMutex error");
		}
	}
	
protected:
	int ThreadMain(void);

	void DoStartCapture(void);
	void DoStopCapture(void);
	
	void ProcessFrame(CMediaFrame* pFrame);
	void ProcessVideo(CMediaFrame* pFrame);
	void ProcessAudio(CMediaFrame* pFrame);

protected:
	bool 		m_sink;
	static const uint32_t MSG_INPUT_FRAME = 3072;
	SDL_mutex*	m_pEnqueueMutex;
	CMediaFrame*	m_pFrame;
	bool m_isAudioSource;
};

#endif /* __VIDEO_FIFO_SOURCE_H__ */
