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

#include "mp4live.h"
#include "video_sdl_preview.h"

int CSDLVideoPreview::ThreadMain(void) 
{
	while (SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
		CMsg* pMsg = m_myMsgQueue.get_message();
		
		if (pMsg != NULL) {
			switch (pMsg->get_value()) {
			case MSG_NODE_STOP_THREAD:
				DoStopPreview();
				delete pMsg;
				return 0;
			case MSG_NODE_START:
				DoStartPreview();
				break;
			case MSG_NODE_STOP:
				DoStopPreview();
				break;
			case MSG_SINK_FRAME:
				size_t dontcare;
				DoPreviewFrame((CMediaFrame*)pMsg->get_message(dontcare));
				break;
			}

			delete pMsg;
		}
	}

	return -1;
}

void CSDLVideoPreview::DoStartPreview()
{
	if (m_preview) {
		return;
	}

	if (!m_pConfig->m_videoPreviewWindowId) {
		return;
	}

	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%u", 
		m_pConfig->m_videoPreviewWindowId);
	setenv("SDL_WINDOWID", buffer, 1);
	setenv("SDL_VIDEO_CENTERED", "1", 1);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		error_message("Could not init SDL video: %s", SDL_GetError());
	}
	char driverName[32];
	if (!SDL_VideoDriverName(driverName, 1)) {
		error_message("Could not init SDL video: %s", SDL_GetError());
	}

	m_sdlScreen = SDL_SetVideoMode(m_pConfig->m_videoWidth, 
		m_pConfig->m_videoHeight, 32, SDL_HWSURFACE | SDL_NOFRAME);

	m_sdlScreenRect.x = 0;
	m_sdlScreenRect.y = 0;
	m_sdlScreenRect.w = m_pConfig->m_videoWidth;
	m_sdlScreenRect.h = m_pConfig->m_videoHeight;

	m_sdlImage = SDL_CreateYUVOverlay(m_pConfig->m_videoWidth, 
		m_pConfig->m_videoHeight, SDL_YV12_OVERLAY, m_sdlScreen);

	// currently we can only do one type of preview
	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)
	  && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {
		// so resolve any misconfiguration
		m_pConfig->SetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW, false);
	}

	m_preview = true;
}

void CSDLVideoPreview::DoStopPreview()
{
	if (!m_preview) {
		return;
	}

	SDL_FreeYUVOverlay(m_sdlImage);
	m_sdlImage = NULL;

	SDL_FreeSurface(m_sdlScreen);
	m_sdlScreen = NULL;

	SDL_Quit();

	m_preview = false;
}

void CSDLVideoPreview::DoPreviewFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return;
	}

	if (m_preview) {
		if ((pFrame->GetType() == CMediaFrame::RawYuvVideoFrame 
		    && m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW))
		  || (pFrame->GetType() == CMediaFrame::ReconstructYuvVideoFrame 
		    && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW))) {

			SDL_LockYUVOverlay(m_sdlImage);

			u_int8_t* pImage = (u_int8_t*)pFrame->GetData();

			memcpy(m_sdlImage->pixels[0], 
				pImage, 
				m_pConfig->m_ySize);

			memcpy(m_sdlImage->pixels[2], 
				pImage + m_pConfig->m_ySize, 
				m_pConfig->m_uvSize);

			memcpy(m_sdlImage->pixels[1], 
				pImage + m_pConfig->m_ySize + m_pConfig->m_uvSize, 
				m_pConfig->m_uvSize);

			SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);

			SDL_UnlockYUVOverlay(m_sdlImage);
		}
	}
	
	delete pFrame;
}

