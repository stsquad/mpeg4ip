/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
 * contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License.  Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 *
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
	File:		RelayOutput.h

	Contains:	An implementation of the ReflectorOutput abstract base class,
				that just writes data out to UDP sockets.

*/

#ifndef __RELAY_OUTPUT_H__
#define __RELAY_OUTPUT_H__

#include "ReflectorOutput.h"
#include "ReflectorSession.h"
#include "SourceInfo.h"

#include "OSQueue.h"
#include "OSMutex.h"

class RelayOutput : public ReflectorOutput
{
	public:
	
		RelayOutput(SourceInfo* inInfo, UInt32 inWhichOutput, ReflectorSession* inSession);
		virtual ~RelayOutput();
		
		// Returns true if this output matches one of the Outputs in the SourceInfo.
		// Also marks the proper SourceInfo::OutputInfo "fAlreadySetup" flag as true 
		Bool16	Equal(SourceInfo* inInfo);
		
		// Call this to setup this object's output socket
		OS_Error BindSocket();
		
		// Writes the packet directly to a UDP socket
		virtual QTSS_Error	WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec,  SInt64* timeToSendThisPacketAgain );
		
		//
		// ACCESSORS
		
		ReflectorSession* 	GetReflectorSession() { return fReflectorSession; }
		StrPtrLen*			GetOutputInfoHTML()	{ return &fOutputInfoHTML; }

		UInt32				GetCurPacketsPerSecond() { return fPacketsPerSecond; }
		UInt32				GetCurBitsPerSecond()	 { return fBitsPerSecond; }
		UInt64&				GetTotalPacketsSent()	 { return fTotalPacketsSent; }
		UInt64&				GetTotalBytesSent()		 { return fTotalBytesSent; }
		
		// Use these functions to iterate over all RelayOutputs
		static OSMutex*	GetQueueMutex() { return &sQueueMutex; }
		static OSQueue*	GetOutputQueue(){ return &sRelayOutputQueue; }
		void TearDown() {};

	private:
	
		enum
		{
			kMaxHTMLSize = 255, // Note, this may be too short and we don't protect!
			kStatsIntervalInMilSecs = 10000 // Update "current" statistics every 10 seconds
		};
	
		ReflectorSession* fReflectorSession;

		// Relay streams all share this one socket for writing.
		UDPSocket 	fOutputSocket;
		UInt32		fNumStreams;
		SourceInfo::OutputInfo fOutputInfo;
		void**		fStreamCookieArray;//Each stream has a cookie
		
		OSQueueElem	fQueueElem;

		char		fHTMLBuf[kMaxHTMLSize];
		StrPtrLen	fOutputInfoHTML;
		ResizeableStringFormatter fFormatter;
		
		// Statistics
		UInt32		fPacketsPerSecond;
		UInt32		fBitsPerSecond;
		
		SInt64		fLastUpdateTime;
		UInt64		fTotalPacketsSent;
		UInt64		fTotalBytesSent;
		UInt64		fLastPackets;
		UInt64		fLastBytes;

		// Queue of all current RelayReflectorOutput objects, for use in the		
		static OSQueue	sRelayOutputQueue;
		static OSMutex	sQueueMutex;
};
		
		
#endif //__RELAY_OUTPUT_H__
