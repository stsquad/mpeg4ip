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
#include "loop_feeder_sink.h"
#include "media_flow.h"
#include "mp4live_common.h"
#include <unistd.h>

int CLoopFeederSink::ThreadMain(void)
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
				uint32_t dontcare;
				DoWriteFrame((CMediaFrame*)pMsg->get_message(dontcare));
				break;
			}

			delete pMsg;
		}
	}

	return -1;
}

void CLoopFeederSink::DoStartSink()
{
	//CONFIGS
	//FILE1
	if (strlen(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE1))>0)
	{
		pConfigAux[0] = InitializeConfigVariables();
		if (ReadConfigFile(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE1), pConfigAux[0])<0)
		{
			pConfigAux[0]=NULL;
		} else pConfigAux[0]->Update();
	} else pConfigAux[0]=NULL;

	//FILE2
	if (strlen(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE2))>0)
	{
		pConfigAux[1] = InitializeConfigVariables();
		if (ReadConfigFile(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE2), pConfigAux[1])<0)
		{
			pConfigAux[1]=NULL;
		}else pConfigAux[1]->Update();
	} else pConfigAux[1]=NULL;

	//FILE3
	if (strlen(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE3))>0)
	{
		pConfigAux[2] = InitializeConfigVariables();
		if (ReadConfigFile(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE3), pConfigAux[2])<0)
		{
			pConfigAux[2]=NULL;
		}else pConfigAux[2]->Update();
	} else pConfigAux[2]=NULL;

	//FILE4
	if (strlen(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE4))>0)
	{
		pConfigAux[3] = InitializeConfigVariables();
		if (ReadConfigFile(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE4), pConfigAux[3])<0)
		{
			pConfigAux[3]=NULL;
		}else pConfigAux[3]->Update();
	} else pConfigAux[3]=NULL;

	//FILE5
	if (strlen(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE5))>0)
	{
		pConfigAux[4] = InitializeConfigVariables();
		if (ReadConfigFile(m_pConfig->GetStringValue(CONFIG_FEEDER_SINK_FILE5), pConfigAux[4])<0)
		{
			pConfigAux[4]=NULL;
		}else pConfigAux[4]->Update();
	}else pConfigAux[4]=NULL;
	
	pConfigAux[5]=NULL;
	
	
	int i=0;
	while(pConfigAux[i]!=NULL)
	{
		//the threads sometimes crash when they start
		//this delay avoid these crashes
		usleep(500000);
		
		pConfigAux[i]->SetParentConfig(m_pConfig);
		pFlow[i] = new CAVMediaFlow(pConfigAux[i]);
		pFlow[i]->Start();
		
		if(pConfigAux[i]->GetBoolValue(CONFIG_VIDEO_ENABLE))
		{
		  pInputVideo[i]=
		    (CLoopSource *)pFlow[i]->GetVideoSource();
		}
		else pInputVideo[i]=NULL;
		
		if(pConfigAux[i]->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		  pInputAudio[i]=(CLoopSource*)pFlow[i]->GetAudioSource();
		}
		else pInputAudio[i]=NULL;
		
		i++;
	}
}

void CLoopFeederSink::DoStopSink()
{
	
	for(int i=0;i<MAX_INPUTS;i++)
	{
		if(pConfigAux[i]!=NULL)
		{
			pFlow[i]->Stop();
			delete pFlow[i];
		}
		else return;
	}
	
}


void CLoopFeederSink::DoWriteFrame(CMediaFrame* pFrame)
{
	int i=0;	
	while(pConfigAux[i]!=NULL)
	{
		if(pFrame->GetType()==RAWPCMAUDIOFRAME)
		{
			if (pInputAudio[i]!=NULL) {
				pFrame->AddReference();
				pInputAudio[i]->EnqueueFrame(pFrame);
			}
		} else if (pFrame->GetType()==RECONSTRUCTYUVVIDEOFRAME)
		{
			if (pInputVideo[i]!=NULL) {
				pFrame->AddReference();
				pInputVideo[i]->EnqueueFrame(pFrame);
			}
		}
		i++;
	}
	if (pFrame->RemoveReference()) delete pFrame;
}

