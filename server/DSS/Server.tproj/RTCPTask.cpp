/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999 Apple Computer, Inc.  All Rights Reserved.
 * The contents of this file constitute Original Code as defined in and are 
 * subject to the Apple Public Source License Version 1.1 (the "License").  
 * You may not use this file except in compliance with the License.  Please 
 * obtain a copy of the License at http://www.apple.com/publicsource and 
 * read it before using this file.
 * 
 * This Original Code and all software distributed under the License are 
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the License for 
 * the specific language governing rights and limitations under the 
 * License.
 * 
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		RTCPTask.cpp

	Contains:	Implementation of class defined in RTCPTask.h
					
	
	
	

*/

#include "RTCPTask.h"
#include "QTSServerInterface.h"
#include "UDPSocketPool.h"
#include "RTPStream.h"

SInt64 RTCPTask::Run()
{
	const UInt32 kMaxRTCPPacketSize = 2048;
	char thePacketBuffer[kMaxRTCPPacketSize];
	StrPtrLen thePacket(thePacketBuffer, 0);
	
	//This task goes through all the UDPSockets in the RTPSocketPool, checking to see
	//if they have data. If they do, it demuxes the packets and sends the packet onto
	//the proper RTP session.
	(void)this->GetEvents();
	
	//Must be done atomically wrt the socket pool.
	QTSServerInterface* theServer = QTSServerInterface::GetServer();
	
	OSMutexLocker locker(theServer->GetSocketPool()->GetMutex());
	for (OSQueueIter iter(theServer->GetSocketPool()->GetSocketQueue());
			!iter.IsDone(); iter.Next())
	{
		UInt32 theRemoteAddr = 0;
		UInt16 theRemotePort = 0;

		UDPSocketPair* thePair = (UDPSocketPair*)iter.GetCurrent()->GetEnclosingObject();
		Assert(thePair != NULL);
		
		for (UInt32 x = 0; x < 2; x++)
		{
			UDPSocket* theSocket = NULL;
			if (x == 0)
				theSocket = thePair->GetSocketA();
			else
				theSocket = thePair->GetSocketB();
				
			UDPDemuxer* theDemuxer = theSocket->GetDemuxer();
			if (theDemuxer != NULL)
				theDemuxer->GetMutex()->Lock();
			
			//get all the outstanding packets for this socket
			while (true)
			{
				thePacket.Len = 0;
				theSocket->RecvFrom(&theRemoteAddr, &theRemotePort, thePacket.Ptr,	
								kMaxRTCPPacketSize, &thePacket.Len);
				if (thePacket.Len == 0)
					break;//no more packets on this socket!
					
				//if this socket has a demuxer, find the target RTPStream
				if (theDemuxer != NULL)
				{
					RTPStream* theStream = (RTPStream*)theDemuxer->GetTask(theRemoteAddr, theRemotePort);
					if (theStream != NULL)
						theStream->ProcessPacket(&thePacket);
				}
			}
			if (theDemuxer != NULL)
				theDemuxer->GetMutex()->Unlock();
		}
	}
	return QTSServerInterface::GetServer()->GetPrefs()->GetRTCPPollIntervalInMsec();
}
