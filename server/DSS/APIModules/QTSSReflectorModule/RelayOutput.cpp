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
	File:		RelayOutput.cpp

	Contains:	Implementation of object described in .h file
					
	

*/

#include "RelayOutput.h"

#include "OSMemory.h"
#include "SocketUtils.h"

// STATIC DATA
OSQueue	RelayOutput::sRelayOutputQueue;
OSMutex	RelayOutput::sQueueMutex;


RelayOutput::RelayOutput(SourceInfo* inInfo, UInt32 inWhichOutput, ReflectorSession* inReflectorSession)
:	fReflectorSession(inReflectorSession),
	fOutputSocket(NULL, Socket::kNonBlockingSocketType),
	fNumStreams(inInfo->GetNumStreams()),
	fOutputInfo(*inInfo->GetOutputInfo(inWhichOutput)),
	fQueueElem(this),
	fFormatter(&fHTMLBuf[0], kMaxHTMLSize),

	fPacketsPerSecond(0),
	fBitsPerSecond(0),
	fLastUpdateTime(0),
	fTotalPacketsSent(0),
	fTotalBytesSent(0),
	fLastPackets(0),
	fLastBytes(0)
{
	Assert(fNumStreams > 0);
	fStreamCookieArray = NEW void*[fNumStreams];
	fOutputInfo.fPortArray = NEW UInt16[fNumStreams];//copy constructor doesn't do this

	// create a bookmark for each stream we'll reflect
	this->InititializeBookmarks( inReflectorSession->GetNumStreams() );
	
	// Copy out all the track IDs for each stream
	for (UInt32 x = 0; x < fNumStreams; x++)
		fStreamCookieArray[x] = inReflectorSession->GetStreamCookie(inInfo->GetStreamInfo(x)->fTrackID);
		
	// Copy the contents of the output port array
	::memcpy(fOutputInfo.fPortArray, inInfo->GetOutputInfo(inWhichOutput)->fPortArray, fNumStreams * sizeof(UInt16));
	
	// Write the Output HTML
	// Looks like: Relaying to: 229.49.52.102, Ports: 16898 16900 Time to live: 15
	static StrPtrLen sHTMLStart("Relaying to: ");
	static StrPtrLen sPorts(", Ports: ");
	static StrPtrLen sTimeToLive(" Time to live: ");
	static StrPtrLen sHTMLEnd("<BR>");
		
	// First, format the destination addr as a dotted decimal string
	char theIPAddrBuf[20];
	StrPtrLen theIPAddr(theIPAddrBuf, 20);
	struct in_addr theAddr;
	theAddr.s_addr = htonl(fOutputInfo.fDestAddr);
	SocketUtils::ConvertAddrToString(theAddr, &theIPAddr);

	// Begin writing the HTML
	fFormatter.Put(sHTMLStart);
	fFormatter.Put(theIPAddr);
	fFormatter.Put(sPorts);
	
	for (UInt32 y = 0; y < fNumStreams; y++)
	{
		// Write all the destination ports
		fFormatter.Put(fOutputInfo.fPortArray[y]);
		fFormatter.PutSpace();
	}
	
	if (SocketUtils::IsMulticastIPAddr(inInfo->GetOutputInfo(inWhichOutput)->fDestAddr))
	{
		// Put the time to live if this is a multicast destination
		fFormatter.Put(sTimeToLive);
		fFormatter.Put(fOutputInfo.fTimeToLive);
	}
	fFormatter.Put(sHTMLEnd);
	
	// Setup the StrPtrLen to point to the right stuff
	fOutputInfoHTML.Ptr = fFormatter.GetBufPtr();
	fOutputInfoHTML.Len = fFormatter.GetCurrentOffset();
	
	OSMutexLocker locker(&sQueueMutex);
	sRelayOutputQueue.EnQueue(&fQueueElem);
}

RelayOutput::~RelayOutput()
{
	OSMutexLocker locker(&sQueueMutex);
	sRelayOutputQueue.Remove(&fQueueElem);
	
	delete [] fStreamCookieArray;
}


OS_Error RelayOutput::BindSocket()
{
	OS_Error theErr = fOutputSocket.Open();
	if (theErr != OS_NoErr)
		return theErr;
	
	// We don't care what local port we bind to
	theErr = fOutputSocket.Bind(fOutputInfo.fLocalAddr, 0);
	if (theErr != OS_NoErr)
		return theErr;
		
	// Set the ttl to be the proper value
	return fOutputSocket.SetTtl(fOutputInfo.fTimeToLive);
}

Bool16	RelayOutput::Equal(SourceInfo* inInfo)
{
	// First check if the Source Info matches this ReflectorSession
	if (!fReflectorSession->Equal(inInfo))
		return false;
	for (UInt32 x = 0; x < inInfo->GetNumOutputs(); x++)
	{
		if (inInfo->GetOutputInfo(x)->Equal(fOutputInfo))
		{
			// This is a rather special purpose function... here we set this
			// flag marking this particular output as a duplicate, because
			// we know it is equal to this object.
			// (This is used in QTSSReflectorModule.cpp:RereadRelayPrefs)
			inInfo->GetOutputInfo(x)->fAlreadySetup = true;
			return true;
		}
	}
	return false;
}

QTSS_Error	RelayOutput::WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 /*packetLatenessInMSec*/ )
{

	// we don't use packetLateness becuase relays don't need to worry about TCP  flow control induced transmit delay
	
	// Look for the matching streamID
	for (UInt32 x = 0; x < fNumStreams; x++)
	{
		if (inStreamCookie == fStreamCookieArray[x])
		{
			UInt16 theDestPort = fOutputInfo.fPortArray[x];
			Assert((theDestPort & 1) == 0);//this should always be an RTP port (even)
			if (inFlags & qtssWriteFlagsIsRTCP)
				theDestPort++;
			
			(void)fOutputSocket.SendTo(fOutputInfo.fDestAddr, theDestPort, 
											inPacket->Ptr, inPacket->Len);

			// Update our totals
			fTotalPacketsSent++;
			fTotalBytesSent += inPacket->Len;
			
			break;
		}
	}

	// If it is time to recalculate statistics, do so
	SInt64 curTime = OS::Milliseconds();
	if ((fLastUpdateTime + kStatsIntervalInMilSecs) < curTime)
	{
		// Update packets per second
		Float64 packetsPerSec = (Float64)((SInt64)fTotalPacketsSent - (SInt64)fLastPackets);
		packetsPerSec *= 1000;
		packetsPerSec /= (Float64)(curTime - fLastUpdateTime);
		fPacketsPerSecond = (UInt32)packetsPerSec;
		
		// Update bits per second. Win32 doesn't implement UInt64 -> Float64.
		Float64 bitsPerSec = (Float64)((SInt64)fTotalBytesSent - (SInt64)fLastBytes);
		bitsPerSec *= 1000 * 8;//convert from seconds to milsecs, bytes to bits
		bitsPerSec /= (Float64)(curTime - fLastUpdateTime);
		fBitsPerSecond = (UInt32)bitsPerSec;

		fLastUpdateTime = curTime;
		fLastPackets = fTotalPacketsSent;
		fLastBytes = fTotalBytesSent;
	}
	
	return QTSS_NoErr;
}

