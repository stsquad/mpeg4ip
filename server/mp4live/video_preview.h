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
 */

#ifndef __VIDEO_PREVIEW_H__ 
#define __VIDEO_PREVIEW_H__ 

#include "media_node.h"

class CVideoPreview : public CMediaSink {
public:
	CVideoPreview() : CMediaSink() {
		m_preview = false;
	}

	void StartPreview(void) {
		m_myMsgQueue.send_message(MSG_START_PREVIEW,
			 NULL, 0, m_myMsgQueueSemaphore);
	}

	void StopPreview(void) {
		m_myMsgQueue.send_message(MSG_STOP_PREVIEW,
			NULL, 0, m_myMsgQueueSemaphore);
	}

protected:
	static const int MSG_START_PREVIEW	= 1;
	static const int MSG_STOP_PREVIEW	= 2;

	int ThreadMain(void);

	void DoStartPreview(void);
	void DoStopPreview(void);
	void DoPreviewFrame(CMediaFrame* pFrame);

protected:
	bool			m_preview;

	SDL_Surface*	m_sdlScreen;
	SDL_Rect		m_sdlScreenRect;
	SDL_Overlay*	m_sdlImage;
};

#endif /* __VIDEO_PREVIEW_H__ */
