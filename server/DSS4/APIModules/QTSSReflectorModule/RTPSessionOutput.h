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
	File:		RTSPReflectorOutput.h

	Contains:	Derived from ReflectorOutput, this implements the WritePacket
				method in terms of the QTSS API (that is, it writes to a client
				using the QTSS_RTPSessionObject
					


*/

#ifndef __RTSP_REFLECTOR_OUTPUT_H__
#define __RTSP_REFLECTOR_OUTPUT_H__

#include "ReflectorOutput.h"
#include "ReflectorSession.h"
#include "QTSS.h"

class RTPSessionOutput : public ReflectorOutput
{
	public:
	
		// Adds some dictionary attributes
		static void Register();
		
		RTPSessionOutput(QTSS_ClientSessionObject inRTPSession, ReflectorSession* inReflectorSession,
							QTSS_Object serverPrefs, QTSS_AttributeID inCookieAddrID);
		virtual ~RTPSessionOutput() {}
		
		ReflectorSession* GetReflectorSession() { return fReflectorSession; }
		
		// This writes the packet out to the proper QTSS_RTPStreamObject.
		// If this function returns QTSS_WouldBlock, timeToSendThisPacketAgain will
		// be set to # of msec in which the packet can be sent, or -1 if unknown
		virtual QTSS_Error	WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec, SInt64* timeToSendThisPacketAgain );
		virtual void TearDown();
		
	private:
	
		QTSS_ClientSessionObject fClientSession;
		ReflectorSession* 		fReflectorSession;
		QTSS_AttributeID		fCookieAttrID;
		
		UInt16 GetPacketSeqNumber(StrPtrLen* inPacket);
		void SetPacketSeqNumber(StrPtrLen* inPacket, UInt16 inSeqNumber);
		Bool16 PacketShouldBeThinned(QTSS_RTPStreamObject inStream, StrPtrLen* inPacket);
};

#endif //__RTSP_REFLECTOR_OUTPUT_H__
