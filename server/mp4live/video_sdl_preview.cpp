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
#include "video_util_resize.h"

int CSDLVideoPreview::ThreadMain(void) 
{
  CMsg *pMsg;
	while (SDL_SemWait(m_myMsgQueueSemaphore) == 0) {
		pMsg = m_myMsgQueue.get_message();
		
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
				uint32_t dontcare;
				CMediaFrame *pFrame = 
				  (CMediaFrame*)pMsg->get_message(dontcare);
				if (pFrame != NULL) {
				  DoPreviewFrame(pFrame);
				  if (pFrame->RemoveReference())
				    delete pFrame;
				}
				break;
			}

			delete pMsg;
		}
	}

	return -1;
}

void CSDLVideoPreview::DoStartPreview()
{
	if (m_sink) {
		return;
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		error_message("Could not init SDL video: %s", SDL_GetError());
	}
	char driverName[32];
	if (!SDL_VideoDriverName(driverName, 1)) {
		error_message("Could not init SDL video: %s", SDL_GetError());
	}

	m_sink = true;
}

void CSDLVideoPreview::DoStopPreview()
{
	if (!m_sink) {
		return;
	}
	
	if (m_sdlImage != NULL) {
	  SDL_FreeYUVOverlay(m_sdlImage);
	  m_sdlImage = NULL;
	}

	if (SDL_FreeSurface != NULL) {
	  SDL_FreeSurface(m_sdlScreen);
	  m_sdlScreen = NULL;
	}

	SDL_Quit();

	m_sink = false;
}

void CSDLVideoPreview::DoPreviewFrame(CMediaFrame* pFrame)
{
	if (pFrame == NULL || pFrame->GetType() != YUVVIDEOFRAME) {
		return;
	}
	yuv_media_frame_t *pYUV = (yuv_media_frame_t *)pFrame->GetData();
	if (m_sdlImage == NULL ||
	    (m_w != pYUV->w || m_h != pYUV->h)) {
	  if (m_sdlImage != NULL) {
	    SDL_FreeYUVOverlay(m_sdlImage);
	    m_sdlImage = NULL;
	  }
	  if (m_sdlScreen != NULL) {
	    SDL_FreeSurface(m_sdlScreen);
	    m_sdlScreen = NULL;
	  }
	  m_w = pYUV->w;
	  m_h = pYUV->h;
	  debug_message("STarting preview %ux%u", m_w, m_h);
	  m_sdlScreen = SDL_SetVideoMode(m_w,
					 m_h, 32, 
					 SDL_HWSURFACE);

	  m_sdlScreenRect.x = 0;
	  m_sdlScreenRect.y = 0;
	  m_sdlScreenRect.w = m_w;
	  m_sdlScreenRect.h = m_h;
	  
	  m_sdlImage = SDL_CreateYUVOverlay(m_w, m_h,
					    SDL_YV12_OVERLAY, m_sdlScreen);
	  
	}
	if (m_sink) {
	  SDL_LockYUVOverlay(m_sdlImage);

	  CopyYuv(pYUV->y, pYUV->u, pYUV->v, 
		  pYUV->y_stride, pYUV->uv_stride, pYUV->uv_stride,
		  m_sdlImage->pixels[0], m_sdlImage->pixels[2], m_sdlImage->pixels[1],
		  m_sdlImage->pitches[0], m_sdlImage->pitches[2], m_sdlImage->pitches[1],
		  m_w, m_h);

	  SDL_UnlockYUVOverlay(m_sdlImage);
	  SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);
	  

	}
}

