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

#include "mp4live.h"
#include <unistd.h>

#include "loop_source.h"

int CLoopSource::ThreadMain(void)
{
	while (true) {
		// message pending
		if (SDL_SemWait(m_myMsgQueueSemaphore)==0) {
			CMsg* pMsg = m_myMsgQueue.get_message();

			if (pMsg != NULL) {
				switch (pMsg->get_value()) {
				case MSG_NODE_STOP_THREAD:
					DoStopCapture();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_NODE_START:				
				case MSG_SOURCE_START_VIDEO:
					DoStartCapture();
					break;		
				case MSG_SOURCE_START_AUDIO:
					DoStartCapture();
					break;		
				case MSG_NODE_STOP:
					DoStopCapture();
					break;
				case MSG_SOURCE_KEY_FRAME:
					DoGenerateKeyFrame();
					break;
				case MSG_INPUT_FRAME:
					uint32_t dontcare;
        				ProcessFrame((CMediaFrame*)pMsg->get_message(dontcare));
        				break;
				}

				delete pMsg;
			}
		}
	}
	return -1;
}

void CLoopSource::ProcessFrame(CMediaFrame* pFrame)
{
	if(pFrame->GetType() == RECONSTRUCTYUVVIDEOFRAME && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) && m_source==true)
	{
		ProcessVideo(pFrame);

	} else if(pFrame->GetType() == RAWPCMAUDIOFRAME && m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE) && m_source==true)
	{
		ProcessAudio(pFrame);
	} else {
		debug_message("Frame Type %i dismished- A enable %i - m_source %i", pFrame->GetType(), m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE), m_source);
		if (pFrame->RemoveReference())
			delete pFrame;
	}
}

void CLoopSource::DoStartCapture(void)
{
	if (m_source) {
		return;
	}
	if (!Init()) {
		return;
	}
	m_source = true;
}

void CLoopSource::DoStopCapture(void)
{
	if (!m_source) {
		return;
	}

	if (m_isAudioSource) DoStopAudio();
	else DoStopVideo();

	m_source = false;
}

bool CLoopSource::Init(void)
{
	CLiveConfig* parentConfig=m_pConfig->GetParentConfig();
	if (m_isAudioSource)
	{
		// for live capture we can match the source to the destination
		m_audioSrcSamplesPerFrame = 1024;
		m_audioDstSamplesPerFrame = 1024;
		
		InitAudio(true);
		SetAudioSrc(
				PCMAUDIOFRAME,
				parentConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS),
				parentConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

	}
	else
	{
		m_pConfig->m_videoNeedRgbToYuv=false;

		m_pConfig->CalculateVideoFrameSize();

		InitVideo(YUVVIDEOFRAME, true);
		SetVideoSrcSize(
				parentConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
				parentConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT),
				parentConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
				true);
	}
	return true;
}

void CLoopSource::ProcessVideo(CMediaFrame* pFrame)
{
	u_int8_t* m_videoMap;
	u_int8_t* pY;
	u_int8_t* pU;
	u_int8_t* pV;

	m_videoMap=(u_int8_t*)Malloc(m_videoSrcYUVSize);
	memcpy(m_videoMap, pFrame->GetData(), m_videoSrcYUVSize);

	pY =(u_int8_t*)m_videoMap;
	pU = pY + m_videoSrcYSize;
	pV = pU + m_videoSrcUVSize;

	ProcessVideoYUVFrame(
                         pY,
                         pU,
                         pV,
                         m_videoSrcWidth,
                         m_videoSrcWidth >> 1,
			 pFrame->GetTimestamp());
	free(m_videoMap);

	if (pFrame->RemoveReference())
	{
		delete pFrame;

	}
}

void CLoopSource::ProcessAudio(CMediaFrame* pFrame)
{
	u_int8_t* pcmData;
	pcmData=(u_int8_t*)Malloc(pFrame->GetDataLength());
	memcpy(pcmData, (u_int8_t*)pFrame->GetData(), (u_int32_t)pFrame->GetDataLength());
	ProcessAudioFrame(pcmData, (u_int32_t)pFrame->GetDataLength(), (u_int64_t)pFrame->GetTimestamp(), false);
	free(pcmData);
	if (pFrame->RemoveReference())
	  delete pFrame;
}


