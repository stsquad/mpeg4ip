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
	File:		RTCPAckPacket.cpp

	Contains:  	RTCPAckPacket de-packetizing class

	
*/


#include "RTCPAckPacket.h"
#include "RTCPPacket.h"
#include "MyAssert.h"
#include <stdio.h>


Bool16 RTCPAckPacket::ParseAckPacket(UInt8* inPacketBuffer, UInt32 inPacketLen)
{
	fRTCPAckBuffer = inPacketBuffer;

	//
	// Check whether this is an ack packet or not.
	if ((inPacketLen < kAckMaskOffset) || (!this->IsAckPacketType()))
		return false;
	
	Assert(inPacketLen == (UInt32)((this->GetPacketLength() * 4)) + RTCPPacket::kRTCPHeaderSizeInBytes);
	fAckMaskSize = inPacketLen - kAckMaskOffset;
	return true;
}

Bool16 RTCPAckPacket::IsAckPacketType()
{
	// While we are moving to a new type, check for both
	UInt32 theAppType = ntohl(*(UInt32*)&fRTCPAckBuffer[kAppPacketTypeOffset]);
	return this->IsAckType(theAppType);
}


