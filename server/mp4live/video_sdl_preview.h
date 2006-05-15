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

#ifndef __VIDEO_SDL_PREVIEW_H__ 
#define __VIDEO_SDL_PREVIEW_H__ 

#include "media_sink.h"

class CSDLVideoPreview : public CMediaSink {
public:
	CSDLVideoPreview() : CMediaSink() {
		m_sdlScreen = NULL;
		m_sdlImage = NULL;
		m_w = m_h = 0;
	}

	virtual const char* name() {
	  return "CSDLVideoPreview";
	}
protected:
	int ThreadMain(void);

	void DoStartPreview(void);
	void DoStopPreview(void);
	void DoPreviewFrame(CMediaFrame* pFrame);

protected:
	SDL_Surface*	m_sdlScreen;
	SDL_Rect		m_sdlScreenRect;
	SDL_Overlay*	m_sdlImage;
	uint32_t m_w, m_h;
};

#endif /* __VIDEO_SDL_PREVIEW_H__ */
