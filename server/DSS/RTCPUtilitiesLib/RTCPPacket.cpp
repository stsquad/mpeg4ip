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
	File:		RTCPPacket.cpp

	Contains:  	RTCPReceiverPacket de-packetizing classes
	
*/


#include "RTCPPacket.h"

#include <stdio.h>


//returns true if successful, false otherwise
Bool16 RTCPPacket::ParsePacket(UInt8* inPacketBuffer, UInt32 inPacketLen)
{
	if (inPacketLen < kRTCPPacketSizeInBytes)
		return false;
	fReceiverPacketBuffer = inPacketBuffer;

	//the length of this packet can be no less than the advertised length (which is
	//in 32-bit words, so we must multiply) plus the size of the header (4 bytes)
	if (inPacketLen < (UInt32)((this->GetPacketLength() * 4) + kRTCPHeaderSizeInBytes))
		return false;
	
	//do some basic validation on the packet
	if (this->GetVersion() != kSupportedRTCPVersion)
		return false;
		
	return true;
}

#ifdef DEBUG_RTCP_PACKETS
void RTCPReceiverPacket::Dump()//Override
{
	RTCPPacket::Dump();
	
	for (int i = 0;i<this->GetReportCount(); i++)
	{
		printf( "	[%d] rptSrcID==%lu, fracLost==%d, totLost==%d, highSeq#==%lu\n"
				"        jit==%lu, lastSRTime==%lu, lastSRDelay==%lu \n",
	                         i,
	                         this->GetReportSourceID(i),
	                         this->GetFractionLostPackets(i),
	                         this->GetTotalLostPackets(i),
	                         this->GetHighestSeqNumReceived(i),
	                         this->GetJitter(i),
	                         this->GetLastSenderReportTime(i),
	                         this->GetLastSenderReportDelay(i) );
	}


}
#endif


Bool16 RTCPReceiverPacket::ParseReceiverReport(UInt8* inPacketBuffer, UInt32 inPacketLength)
{
	Bool16 ok = this->ParsePacket(inPacketBuffer, inPacketLength);
	if (!ok)
		return false;
	
	fRTCPReceiverReportArray = inPacketBuffer + kRTCPPacketSizeInBytes;
	
	//this is the maximum number of reports there could possibly be
	int theNumReports = (inPacketLength - kRTCPPacketSizeInBytes) / kReportBlockOffsetSizeInBytes;

	//if the number of receiver reports is greater than the theoretical limit, return an error.
	if (this->GetReportCount() > theNumReports)
		return false;
		
	return true;
}

UInt32 RTCPReceiverPacket::GetCumulativeFractionLostPackets()
{
	float avgFractionLost = 0;
	for (short i = 0; i < this->GetReportCount(); i++)
	{
		avgFractionLost += this->GetFractionLostPackets(i);
		avgFractionLost /= (i+1);
	}
	
	return (UInt32)avgFractionLost;
}


UInt32 RTCPReceiverPacket::GetCumulativeJitter()
{
	float avgJitter = 0;
	for (short i = 0; i < this->GetReportCount(); i++)
	{
		avgJitter += this->GetJitter(i);
		avgJitter /= (i + 1);
	}
	
	return (UInt32)avgJitter;
}


UInt32 RTCPReceiverPacket::GetCumulativeTotalLostPackets()
{
	UInt32 totalLostPackets = 0;
	for (short i = 0; i < this->GetReportCount(); i++)
	{
		totalLostPackets += this->GetTotalLostPackets(i);
	}
	
	return totalLostPackets;
}



#ifdef DEBUG_RTCP_PACKETS
void RTCPPacket::Dump()
{
	
	printf("\n--\n");
	
	printf( "vers==%d, pad==%d, ReportCount==%d, type==%d, length==%d, sourceID==%ld\n",
			 this->GetVersion(),
             (int)this->GetHasPadding(),
             this->GetReportCount(),
             (int)this->GetPacketType(),
             (int)this->GetPacketLength(),
             this->GetPacketSourceID() );
}
#endif


