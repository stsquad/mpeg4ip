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
	File:		RTPPacketResender.h

	Contains:	RTPPacketResender class to buffer and track re-transmits of RTP packets.
	
	the ctor copies the packet data, sets a timer for the packet's age limit and
	another timer for it's possible re-transmission.
	A duration timer is started to measure the RTT based on the client's ack.
	
*/

#ifndef __RTP_PACKET_RESENDER_H__
#define __RTP_PACKET_RESENDER_H__

#include "PLDoubleLinkedList.h"

#include "RTPBandwidthTracker.h"
#include "DssStopwatch.h"
#include "UDPSocket.h"
#include "OSMemory.h"
#include "OSBufferPool.h"

#define RTP_PACKET_RESENDER_DEBUGGING 1

class MyAckListLog;

class RTPResenderEntry
{
	public:
		
		void*				fPacketData;
		UInt32				fPacketSize;
		SInt64				fExpireTime;
		SInt64				fAddedTime;
		SInt64				fOrigRetransTimeout;
		UInt32				fNumResends;
#if RTP_PACKET_RESENDER_DEBUGGING
		UInt16				fSeqNum;
		UInt32				fPacketArraySizeWhenAdded;
#endif
};


class RTPPacketResender
{
	public:
		
		RTPPacketResender();
		~RTPPacketResender();
		
		//
		// These must be called before using the object
		void				SetDestination(UDPSocket* inOutputSocket, UInt32 inDestAddr, UInt16 inDestPort);
		void				SetBandwidthTracker(RTPBandwidthTracker* inTracker) { fBandwidthTracker = inTracker; }
		
		//
		// AddPacket adds a new packet to the resend queue. This will not send the packet.
		// AddPacket itself is not thread safe.
		void 				AddPacket( void * rtpPacket, UInt32 packetSize, SInt32 ageLimitInMsec );
		
		//
		// Acks a packet. Also not thread safe.
		void 				AckPacket( UInt16 sequenceNumber, SInt64& inCurTimeInMsec );

		//
		// Resends outstanding packets in the queue. Guess what. Not thread safe.
		void 				ResendDueEntries();

		//
		// ACCESSORS
		Bool16				IsFlowControlled()		{ return fBandwidthTracker->IsFlowControlled(); }
		SInt32				GetMaxPacketsInList()	{ return fMaxPacketsInList; }
		SInt32				GetNumPacketsInList()	{ return fPacketsInList; }
		
		static UInt32		GetNumRetransmitBuffers() { return sBufferPool.GetTotalNumBuffers(); }
		static UInt32		GetWastedBufferBytes() { return sNumWastedBytes; }

#if RTP_PACKET_RESENDER_DEBUGGING
		void				SetDebugInfo(UInt32 trackID, UInt16 remoteRTCPPort, UInt32 curPacketDelay);
		void				SetLog( StrPtrLen *logname );
		UInt32 				SpillGuts(UInt32 inBytesSentThisInterval);
		void 				LogClose(SInt64 inTimeSpentInFlowControl);

#else
		void				SetLog( StrPtrLen */*logname*/) {}
#endif
		
	private:
	
		// Tracking the capacity of the network
		RTPBandwidthTracker* fBandwidthTracker;
		
		// Who to send to
		UDPSocket*			fSocket;
		UInt32				fDestAddr;
		UInt16				fDestPort;

		UInt32				fMaxPacketsInList;
		UInt32				fPacketsInList;
		UInt32				fNumResends;				// how many total retransmitted packets
		UInt32				fNumExpired;				// how many total packets dropped
		UInt32				fNumAcksForMissingPackets;	// how many acks received in the case where the packet was not in the list
		UInt32				fNumSent;					// how many packets sent

#if RTP_PACKET_RESENDER_DEBUGGING
		void				logprintf( const char * format, ... );
		MyAckListLog		*fLogger;
		
		UInt32				fTrackID;
		UInt16				fRemoteRTCPPort;
		UInt32				fCurrentPacketDelay;
		DssDurationTimer	fInfoDisplayTimer;
#endif
		
		RTPResenderEntry*	fPacketArray;
		UInt16				fStartSeqNum;
		UInt32				fPacketArraySize;
		UInt32				fPacketArrayMask;
		UInt16				fHighestSeqNum;
		
		RTPResenderEntry*	GetEntryByIndex(UInt16 inIndex);
		RTPResenderEntry*	GetEntryBySeqNum(UInt16 inSeqNum);

		RTPResenderEntry*	GetEmptyEntry(UInt16 inSeqNum);
		void ReallocatePacketArray();
		void RemovePacket(RTPResenderEntry* inEntry, UInt16 inSeqNum);

		static OSBufferPool	sBufferPool;
		static unsigned int	sNumWastedBytes;
		
		void 			UpdateCongestionWindow(SInt32 bytesToOpenBy );
};

#endif //__RTP_PACKET_RESENDER_H__