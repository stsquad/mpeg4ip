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
	File:		RTPPacketResender.cpp

	Contains:	RTPPacketResender class to buffer and track re-transmits of RTP packets.
	
	
*/

#include <stdio.h>

#include "RTPPacketResender.h"
#include "RTPStream.h"
#include "atomic.h"
#include "OSMutex.h"

#if RTP_PACKET_RESENDER_DEBUGGING
#include "QTSSRollingLog.h"
#include <stdarg.h>

class MyAckListLog : public QTSSRollingLog
{
	public:
	
		MyAckListLog(char * logFName) : QTSSRollingLog() { ::strcpy( fLogFName, logFName ); }
		virtual ~MyAckListLog() {}
	
		virtual char* GetLogName() 
		{ 
			char *logFileNameStr = NEW char[80];
			
			::strcpy( logFileNameStr, fLogFName );
			return logFileNameStr; 
		}
		
		virtual char* GetLogDir()
		{ 
			char *logDirStr = NEW char[80];
			
#ifdef __MacOSX__
			::strcpy( logDirStr, "/Library/QuickTimeStreaming/Logs" );
#else
			::strcpy( logDirStr, "/var/streaming/logs" );
#endif
			return logDirStr; 
		}
		
		virtual UInt32 GetRollIntervalInDays() 	{ return 0; }
		virtual UInt32 GetMaxLogBytes()			{ return 0; }
		
	char	fLogFName[128];
	
};
#endif

static const UInt32 kPacketArrayIncreaseInterval = 32;// must be multiple of 2
static const UInt32 kInitialPacketArraySize = 64;// must be multiple of kPacketArrayIncreaseInterval (Turns out this is as big as we typically need)
//static const UInt32 kMaxPacketArraySize = 512;// must be multiple of kPacketArrayIncreaseInterval it would have to be a 3 mbit or more

static const UInt32 kMaxDataBufferSize = 1600;
OSBufferPool RTPPacketResender::sBufferPool(kMaxDataBufferSize);
unsigned int	RTPPacketResender::sNumWastedBytes = 0;

#if 1 // john's code
RTPPacketResender::RTPPacketResender()
:	fBandwidthTracker(NULL),
	fSocket(NULL),
	fDestAddr(0),
	fDestPort(0),
	fMaxPacketsInList(0),
	fPacketsInList(0),
	fNumResends(0),
	fNumExpired(0),
	fNumAcksForMissingPackets(0),
	fNumSent(0),
	fPacketArray(NULL),
	fPacketArraySize(kInitialPacketArraySize),
	fPacketArrayMask(0),
	fHighestSeqNum(0),
	fLastUsed(0),
	fPacketQMutex()
{
	fPacketArray = (RTPResenderEntry*) NEW char[sizeof(RTPResenderEntry) * fPacketArraySize];
	::memset(fPacketArray,0,sizeof(RTPResenderEntry) * fPacketArraySize);

}

RTPPacketResender::~RTPPacketResender()
{
	for (UInt32 x = 0; x < fPacketArraySize; x++)
	{
		if (fPacketArray[x].fPacketSize > 0)
			atomic_sub(&sNumWastedBytes, kMaxDataBufferSize - fPacketArray[x].fPacketSize);
		if (fPacketArray[x].fPacketData != NULL)
		{
			if (fPacketArray[x].fIsSpecialBuffer)
				delete [] (char*)fPacketArray[x].fPacketData;
			else
				sBufferPool.Put(fPacketArray[x].fPacketData);
		}
	}
			
	delete [] fPacketArray;
	

}

#if RTP_PACKET_RESENDER_DEBUGGING
void RTPPacketResender::logprintf( const char * format, ... )
{
	/*
		WARNING - the logger is not multiple task thread safe.
		its OK when we run just one thread for all of the
		sending tasks though.
		
		each logger for a given session will open up access
		to the same log file.  with one thread we're serialized
		on writing to the file, so it works. 
	*/
	
	va_list argptr;
	char	buff[1024];
	

	va_start( argptr, format );
	
	vsprintf( buff, format, argptr );
	
	va_end(argptr);

	if ( fLogger )
	{
		fLogger->WriteToLog(buff, false);
		printf( buff );
	}
}


void RTPPacketResender::SetDebugInfo(UInt32 trackID, UInt16 remoteRTCPPort, UInt32 curPacketDelay)
{
	fTrackID = trackID;
	fRemoteRTCPPort = remoteRTCPPort;
	fCurrentPacketDelay = curPacketDelay;
}

void RTPPacketResender::SetLog( StrPtrLen *logname )
{
	/*
		WARNING - see logprintf()
	*/
	
	char	logFName[128];
	
	memcpy( logFName, logname->Ptr, logname->Len );
	logFName[logname->Len] = 0;
	
	if ( fLogger )
		delete fLogger;

	fLogger = new MyAckListLog( logFName );
	
	fLogger->EnableLog();
}

void RTPPacketResender::LogClose(SInt64 inTimeSpentInFlowControl)
{
	this->logprintf("Flow control duration msec: %"_64BITARG_"d. Max outstanding packets: %d\n", inTimeSpentInFlowControl, this->GetMaxPacketsInList());
	
}


UInt32 RTPPacketResender::SpillGuts(UInt32 inBytesSentThisInterval)
{
	if (fInfoDisplayTimer.DurationInMilliseconds() > 1000 )
	{
		//fDisplayCount++;
		
		// spill our guts on the state of KRR
		char *isFlowed = "open";
		
		if ( fBandwidthTracker->IsFlowControlled() )
			isFlowed = "flowed";
		
		SInt64	kiloBitperSecond = (( (SInt64)inBytesSentThisInterval * (SInt64)1000 * (SInt64)8 ) / fInfoDisplayTimer.DurationInMilliseconds() ) / (SInt64)1024;
		
		//fStreamCumDuration += fInfoDisplayTimer.DurationInMilliseconds();
		fInfoDisplayTimer.Reset();

		//this->logprintf( "\n[%li] info for track %li, cur bytes %li, cur kbit/s %li\n", /*(long)fStreamCumDuration,*/ fTrackID, (long)inBytesSentThisInterval, (long)kiloBitperSecond);
		this->logprintf( "\nx info for track %li, cur bytes %li, cur kbit/s %li\n", /*(long)fStreamCumDuration,*/ fTrackID, (long)inBytesSentThisInterval, (long)kiloBitperSecond);
		this->logprintf( "stream is %s, bytes pending ack %li, cwnd %li, ssth %li, wind %li \n", isFlowed, fBandwidthTracker->BytesInList(), fBandwidthTracker->CongestionWindow(), fBandwidthTracker->SlowStartThreshold(), fBandwidthTracker->ClientWindowSize() );
		this->logprintf( "stats- resends:  %li, expired:  %li, dupe acks: %li, sent: %li\n", fNumResends, fNumExpired, fNumAcksForMissingPackets, fNumSent );
		this->logprintf( "delays- cur:  %li, srtt: %li , dev: %li, rto: %li, bw: %li\n\n", fCurrentPacketDelay, fBandwidthTracker->RunningAverageMSecs(), fBandwidthTracker->RunningMeanDevationMSecs(), fBandwidthTracker->CurRetransmitTimeout(), fBandwidthTracker->GetCurrentBandwidthInBps());
		
		
		inBytesSentThisInterval = 0;
	}
	return inBytesSentThisInterval;
}

#endif


void RTPPacketResender::SetDestination(UDPSocket* inOutputSocket, UInt32 inDestAddr, UInt16 inDestPort)
{
	fSocket = inOutputSocket;
	fDestAddr = inDestAddr;
	fDestPort = inDestPort;
}

RTPResenderEntry*	RTPPacketResender::GetEmptyEntry(UInt16 inSeqNum, UInt32 inPacketSize)
{
	
	RTPResenderEntry* theEntry = NULL;
	
	for (UInt32 packetIndex = 0 ;packetIndex < fPacketsInList; packetIndex++) // see if packet is already in the array
	{	if (inSeqNum == fPacketArray[packetIndex].fSeqNum)
		{	return NULL;
		}
	}

	if (fPacketsInList == fPacketArraySize) // allocate a new array
	{
		fPacketArraySize += kPacketArrayIncreaseInterval;
		RTPResenderEntry* tempArray = (RTPResenderEntry*) NEW char[sizeof(RTPResenderEntry) * fPacketArraySize];
		::memset(tempArray,0,sizeof(RTPResenderEntry) * fPacketArraySize);
		::memcpy(tempArray,fPacketArray,sizeof(RTPResenderEntry) * fPacketsInList);
		delete [] fPacketArray;
		fPacketArray = tempArray;
		//printf("NewArray size=%ld packetsInList=%ld\n",fPacketArraySize, fPacketsInList);
	}

	if (fPacketsInList <  fPacketArraySize) // have an open spot
	{	theEntry = &fPacketArray[fPacketsInList];
		fPacketsInList++;
		
		if (fPacketsInList < fPacketArraySize)
			fLastUsed = fPacketsInList;
		else
			fLastUsed = fPacketArraySize;
	}
	else 
	{
		// nothing open so re-use 
		if (fLastUsed < fPacketArraySize - 1)
			fLastUsed ++;
		else
			fLastUsed = 0;
			
		//printf("array is full = %lu reusing index=%lu\n",fPacketsInList,fLastUsed); 
		theEntry = &fPacketArray[fLastUsed];
		RemovePacket(fLastUsed, false); // delete packet in place don't fill we will use the spot
	}
			
	//
	// Check to see if this packet is too big for the buffer. If it is, then
	// we need to specially allocate a special buffer
	if (inPacketSize > kMaxDataBufferSize)
	{
		//sBufferPool.Put(theEntry->fPacketData);
		theEntry->fIsSpecialBuffer = true;
		theEntry->fPacketData = NEW char[inPacketSize];
	}
	else// It is not special, it's from the buffer pool
	{	theEntry->fIsSpecialBuffer = false;
		theEntry->fPacketData = sBufferPool.Get();
	}

	

	return theEntry;
}


void RTPPacketResender::ClearOutstandingPackets()
{	
	//OSMutexLocker packetQLocker(&fPacketQMutex);
	//for (UInt16 packetIndex = 0; packetIndex < fPacketArraySize; packetIndex++) //Testing purposes
	for (UInt16 packetIndex = 0; packetIndex < fPacketsInList; packetIndex++)
	{
		this->RemovePacket(packetIndex,false);// don't move packets delete in place
		Assert(fPacketArray[packetIndex].fPacketSize==0);
	}
	if (fBandwidthTracker != NULL)
		fBandwidthTracker->EmptyWindow(fBandwidthTracker->BytesInList()); //clean it out
	fPacketsInList = 0; // deleting in place doesn't decrement
	
	Assert(fPacketsInList == 0);
}

void RTPPacketResender::AddPacket( void * inRTPPacket, UInt32 packetSize, SInt32 ageLimit )
{
	//OSMutexLocker packetQLocker(&fPacketQMutex);
	// the caller needs to adjust the overall age limit by reducing it
	// by the current packet lateness.
	
	// we compute a re-transmit timeout based on the Karns RTT esmitate
	
	UInt16* theSeqNumP = (UInt16*)inRTPPacket;
	UInt16 theSeqNum = ntohs(theSeqNumP[1]);
	
	if ( ageLimit > 0 )
	{	
		RTPResenderEntry* theEntry = this->GetEmptyEntry(theSeqNum, packetSize);

		//
		// This may happen if this sequence number has already been added.
		// That may happen if we have repeat packets in the stream.
		if (theEntry == NULL || theEntry->fPacketSize > 0)
			return;
			
		//
		// Reset all the information in the RTPResenderEntry
		::memcpy(theEntry->fPacketData, inRTPPacket, packetSize);
		theEntry->fPacketSize = packetSize;
		theEntry->fAddedTime = OS::Milliseconds();
		theEntry->fOrigRetransTimeout = fBandwidthTracker->CurRetransmitTimeout();
		theEntry->fExpireTime = theEntry->fAddedTime + ageLimit;
		theEntry->fNumResends = 0;
		theEntry->fSeqNum = theSeqNum;
		
		//
		// Track the number of wasted bytes we have
		atomic_add(&sNumWastedBytes, kMaxDataBufferSize - packetSize);
		
		//PLDoubleLinkedListNode<RTPResenderEntry> * listNode = NEW PLDoubleLinkedListNode<RTPResenderEntry>( new RTPResenderEntry(inRTPPacket, packetSize, ageLimit, fRTTEstimator.CurRetransmitTimeout() ) );
		//fAckList.AddNodeToTail(listNode);
		fBandwidthTracker->FillWindow(packetSize);
	}
	else
	{
#if RTP_PACKET_RESENDER_DEBUGGING	
		this->logprintf( "packet too old to add: seq# %li, age limit %li, cur late %li, track id %li\n", (long)ntohs( *((UInt16*)(((char*)inRTPPacket)+2)) ), (long)ageLimit, fCurrentPacketDelay, fTrackID );
#endif
		fNumExpired++;
	}
	fNumSent++;
}

void RTPPacketResender::AckPacket( UInt16 inSeqNum, SInt64& inCurTimeInMsec )
{
	//OSMutexLocker packetQLocker(&fPacketQMutex);
	
	SInt32 foundIndex = -1;
	for (UInt32 packetIndex = 0 ;packetIndex < fPacketsInList; packetIndex++)
	{	if (inSeqNum == fPacketArray[packetIndex].fSeqNum)
		{	foundIndex = packetIndex;
			break;
		}
	}
	
	RTPResenderEntry* theEntry = NULL;
	if (foundIndex != -1)
		theEntry = &fPacketArray[foundIndex];


	if (theEntry == NULL || theEntry->fPacketSize == 0 )
	{	/* 	we got an ack for a packet that has already expired or
			for a packet whose re-transmit crossed with it's original ack
	
		*/
#if RTP_PACKET_RESENDER_DEBUGGING	
		this->logprintf( "acked packet not found: %li, track id %li, OS::MSecs %li\n"
		, (long)inSeqNum, fTrackID, (long)OS::Milliseconds()
		 );
#endif
		fNumAcksForMissingPackets++;
		//printf("Ack for missing packet: %d\n", inSeqNum);
		 
		 // hmm.. we -should not have- closed down the window in this case
		 // so reopen it a bit as we normally would.
		 // ?? who know's what it really was, just use kMaximumSegmentSize
	 	fBandwidthTracker->EmptyWindow( RTPBandwidthTracker::kMaximumSegmentSize, false );

		// when we don't add an estimate from re-transmitted segments we're actually *underestimating* 
		// both the variation and srtt since we're throwing away ALL estimates above the current RTO!
		// therefore it's difficult for us to rapidly adapt to increases in RTT, as well as RTT that
		// are higher than our original RTO estimate.
		
		// for duplicate acks, use 1.5x the cur RTO as the RTT sample
		//fRTTEstimator.AddToEstimate( fRTTEstimator.CurRetransmitTimeout() * 3 / 2 );
		/// this results in some very very big RTO's since the dupes come in batches of maybe 10 or more!
	}
	else
	{

#if RTP_PACKET_RESENDER_DEBUGGING
		Assert(inSeqNum == theEntry->fSeqNum);
		this->logprintf( "Ack for packet: %li, track id %li, OS::MSecs %qd\n"
		, (long)inSeqNum, fTrackID, OS::Milliseconds()
		 );
#endif		
		fBandwidthTracker->EmptyWindow(theEntry->fPacketSize);
		if ( theEntry->fNumResends == 0 )
		{
			// add RTT sample...		
			// only use rtt from packets acked after their initial send, do not use
			// estimates gatherered from re-trasnmitted packets.
			//fRTTEstimator.AddToEstimate( theEntry->fPacketRTTDuration.DurationInMilliseconds() );
			fBandwidthTracker->AddToRTTEstimate( inCurTimeInMsec - theEntry->fAddedTime );
		}
		else
		{
	#if RTP_PACKET_RESENDER_DEBUGGING
			this->logprintf( "re-tx'd packet acked.  ack num : %li, pack seq #: %li, num resends %li, track id %li, size %li, OS::MSecs %qd\n" \
			, (long)inSeqNum, (long)ntohs( *((UInt16*)(((char*)theEntry->fPacketData)+2)) ), (long)theEntry->fNumResends
			, (long)fTrackID, theEntry->fPacketSize, OS::Milliseconds() );
	#endif
		}
		this->RemovePacket(foundIndex);
	}
}

void RTPPacketResender::RemovePacket(UInt32 packetIndex, Bool16 reuseIndex)
{

	//OSMutexLocker packetQLocker(&fPacketQMutex);

	Assert(packetIndex < fPacketArraySize);
	if (packetIndex >= fPacketArraySize)
		return;
		
	if (fPacketsInList == 0)
		return;
		
	RTPResenderEntry* theEntry = &fPacketArray[packetIndex];
	if (theEntry->fPacketSize == 0)
		return;
		
	//
	// Track the number of wasted bytes we have
	atomic_sub(&sNumWastedBytes, kMaxDataBufferSize - theEntry->fPacketSize);
	Assert(theEntry->fPacketSize > 0);

	//
	// Update our list information
	Assert(fPacketsInList > 0);
	
	if (theEntry->fIsSpecialBuffer)
	{	delete [] (char*)theEntry->fPacketData;
	}
	else if (theEntry->fPacketData != NULL)
		sBufferPool.Put(theEntry->fPacketData);
		

	if (reuseIndex) // we are re-using the space so keep array contiguous
	{
		fPacketArray[packetIndex] = fPacketArray[fPacketsInList -1]; 
		::memset(&fPacketArray[fPacketsInList -1],0,sizeof(RTPResenderEntry));
		fPacketsInList--;

	}
	else	// the array is full
	{
		fBandwidthTracker->EmptyWindow( theEntry->fPacketSize, false ); // keep window available
		::memset(theEntry,0,sizeof(RTPResenderEntry));
	}
	
}

void RTPPacketResender::ResendDueEntries()
{
	if (fPacketsInList <= 0)
		return;
		
	//OSMutexLocker packetQLocker(&fPacketQMutex);
	//
	SInt32 numResends = 0;
	RTPResenderEntry* theEntry = NULL; 
	SInt64 curTime = OS::Milliseconds();
	for (SInt32 packetIndex = fPacketsInList -1; packetIndex >= 0; packetIndex--) // walk backwards because remove packet moves array members forward
	{
		theEntry = &fPacketArray[packetIndex];
		
		if (theEntry->fPacketSize == 0) 
			continue;
			
		if (curTime > theEntry->fExpireTime)
		{
#if RTP_PACKET_RESENDER_DEBUGGING	
			unsigned char version;
			version = *((char*)theEntry->fPacketData);
			version &= 0x84; 	// grab most sig 2 bits
			version = version >> 6;	// shift by 6 bits
			this->logprintf( "expired:  seq number %li, track id %li (port: %li), vers # %li, pack seq # %li, size: %li, OS::Msecs: %qd\n", \
								(long)ntohs( *((UInt16*)(((char*)theEntry->fPacketData)+2)) ), fTrackID,  (long) ntohs(fDestPort), \
								(long)version, (long)ntohs( *((UInt16*)(((char*)theEntry->fPacketData)+2))), theEntry->fPacketSize, OS::Milliseconds() );
#endif
			//
			// This packet is expired
			fNumExpired++;
			//printf("Packet expired: %d\n", ((UInt16*)thePacket)[1]);
                        fBandwidthTracker->EmptyWindow(theEntry->fPacketSize);
			this->RemovePacket(packetIndex);
			continue;
		}
		
		if ((curTime - theEntry->fAddedTime) > fBandwidthTracker->CurRetransmitTimeout())
		{
			// Resend this packet
			fSocket->SendTo(fDestAddr, fDestPort, theEntry->fPacketData, theEntry->fPacketSize);
			//printf("Packet resent: %d\n", ((UInt16*)theEntry->fPacketData)[1]);

			theEntry->fNumResends++;
	#if RTP_PACKET_RESENDER_DEBUGGING	
			this->logprintf( "re-sent: %li RTO %li, track id %li (port %li), size: %li, OS::Ms %qd\n", (long)ntohs( *((UInt16*)(((char*)theEntry->fPacketData)+2)) ),  curTime - theEntry->fAddedTime, \
					fTrackID, (long) ntohs(fDestPort) \
					, theEntry->fPacketSize, OS::Milliseconds());
	#endif		

			fNumResends++;
			
			numResends ++;
			//printf("resend loop numResends=%ld packet theEntry->fNumResends=%ld stream fNumResends=\n",numResends,theEntry->fNumResends++, fNumResends);
						
			// ok -- lets try this.. add 1.5x of the INITIAL duration since the last send to the rto estimator
			// since we won't get an ack on this packet
			// this should keep us from exponentially increasing due o a one time increase
			// in the actuall rtt, only AddToEstimate on the first resend ( assume that it's a dupe )
			// if it's not a dupe, but rather an actual loss, the subseqnuent actuals wil bring down the average quickly
			
			if ( theEntry->fNumResends == 1 )
				fBandwidthTracker->AddToRTTEstimate( (theEntry->fOrigRetransTimeout  * 3) / 2 );
			
			theEntry->fAddedTime = curTime;
			fBandwidthTracker->AdjustWindowForRetransmit();
			continue;
		}
		
	}
}
void RTPPacketResender::RemovePacket(RTPResenderEntry* inEntry){ Assert(0); }

#endif

#if 0 // Denis's code
RTPPacketResender::RTPPacketResender()
:	fBandwidthTracker(NULL),
	fSocket(NULL),
	fDestAddr(0),
	fDestPort(0),
	fMaxPacketsInList(0),
	fPacketsInList(0),
	fNumResends(0),
	fNumExpired(0),
	fNumAcksForMissingPackets(0),
	fNumSent(0),
	fPacketArray(NULL),
	fStartSeqNum(0),
	fPacketArraySize(kInitialPacketArraySize),
	fPacketArrayMask(0),
	fHighestSeqNum(0)
{
	
#if RTP_PACKET_RESENDER_DEBUGGING
	fLogger = NULL;
#endif
	fPacketArray = (RTPResenderEntry*) NEW char[sizeof(RTPResenderEntry) * kInitialPacketArraySize];
	::memset(fPacketArray,0,sizeof(RTPResenderEntry) * kInitialPacketArraySize);
	fPacketArrayMask = kInitialPacketArraySize-1;
}

void RTPPacketResender::RemovePacket(UInt32 packetIndex) { Assert(0); }

RTPPacketResender::~RTPPacketResender()
{
	for (UInt32 x = 0; x < fPacketArraySize; x++)
	{
		if (fPacketArray[x].fPacketSize > 0)
			atomic_sub(&sNumWastedBytes, kMaxDataBufferSize - fPacketArray[x].fPacketSize);
		if (fPacketArray[x].fPacketData != NULL)
		{
			if (fPacketArray[x].fIsSpecialBuffer)
				delete [] (char*)fPacketArray[x].fPacketData;
			else
				sBufferPool.Put(fPacketArray[x].fPacketData);
		}
	}
			
	delete [] fPacketArray;
	
#if RTP_PACKET_RESENDER_DEBUGGING
	if ( fLogger )
		delete fLogger;
#endif
}

#if RTP_PACKET_RESENDER_DEBUGGING
void RTPPacketResender::logprintf( const char * format, ... )
{
	/*
		WARNING - the logger is not multiple task thread safe.
		its OK when we run just one thread for all of the
		sending tasks though.
		
		each logger for a given session will open up access
		to the same log file.  with one thread we're serialized
		on writing to the file, so it works. 
	*/
	
	va_list argptr;
	char	buff[1024];
	

	va_start( argptr, format );
	
	vsprintf( buff, format, argptr );
	
	va_end(argptr);

	if ( fLogger )
	{
		fLogger->WriteToLog(buff, false);
		printf( buff );
	}
}


void RTPPacketResender::SetDebugInfo(UInt32 trackID, UInt16 remoteRTCPPort, UInt32 curPacketDelay)
{
	fTrackID = trackID;
	fRemoteRTCPPort = remoteRTCPPort;
	fCurrentPacketDelay = curPacketDelay;
}

void RTPPacketResender::SetLog( StrPtrLen *logname )
{
	/*
		WARNING - see logprintf()
	*/
	
	char	logFName[128];
	
	memcpy( logFName, logname->Ptr, logname->Len );
	logFName[logname->Len] = 0;
	
	if ( fLogger )
		delete fLogger;

	fLogger = new MyAckListLog( logFName );
	
	fLogger->EnableLog();
}

void RTPPacketResender::LogClose(SInt64 inTimeSpentInFlowControl)
{
	this->logprintf("Flow control duration msec: %"_64BITARG_"d. Max outstanding packets: %d\n", inTimeSpentInFlowControl, this->GetMaxPacketsInList());
	
}


UInt32 RTPPacketResender::SpillGuts(UInt32 inBytesSentThisInterval)
{
	if (fInfoDisplayTimer.DurationInMilliseconds() > 1000 )
	{
		//fDisplayCount++;
		
		// spill our guts on the state of KRR
		char *isFlowed = "open";
		
		if ( fBandwidthTracker->IsFlowControlled() )
			isFlowed = "flowed";
		
		SInt64	kiloBitperSecond = (( (SInt64)inBytesSentThisInterval * (SInt64)1000 * (SInt64)8 ) / fInfoDisplayTimer.DurationInMilliseconds() ) / (SInt64)1024;
		
		//fStreamCumDuration += fInfoDisplayTimer.DurationInMilliseconds();
		fInfoDisplayTimer.Reset();

		//this->logprintf( "\n[%li] info for track %li, cur bytes %li, cur kbit/s %li\n", /*(long)fStreamCumDuration,*/ fTrackID, (long)inBytesSentThisInterval, (long)kiloBitperSecond);
		this->logprintf( "\nx info for track %li, cur bytes %li, cur kbit/s %li\n", /*(long)fStreamCumDuration,*/ fTrackID, (long)inBytesSentThisInterval, (long)kiloBitperSecond);
		this->logprintf( "stream is %s, bytes pending ack %li, cwnd %li, ssth %li, wind %li \n", isFlowed, fBandwidthTracker->BytesInList(), fBandwidthTracker->CongestionWindow(), fBandwidthTracker->SlowStartThreshold(), fBandwidthTracker->ClientWindowSize() );
		this->logprintf( "stats- resends:  %li, expired:  %li, dupe acks: %li, sent: %li\n", fNumResends, fNumExpired, fNumAcksForMissingPackets, fNumSent );
		this->logprintf( "delays- cur:  %li, srtt: %li , dev: %li, rto: %li, bw: %li\n\n", fCurrentPacketDelay, fBandwidthTracker->RunningAverageMSecs(), fBandwidthTracker->RunningMeanDevationMSecs(), fBandwidthTracker->CurRetransmitTimeout(), fBandwidthTracker->GetCurrentBandwidthInBps());
		
		
		inBytesSentThisInterval = 0;
	}
	return inBytesSentThisInterval;
}

#endif

void RTPPacketResender::SetDestination(UDPSocket* inOutputSocket, UInt32 inDestAddr, UInt16 inDestPort)
{
	fSocket = inOutputSocket;
	fDestAddr = inDestAddr;
	fDestPort = inDestPort;
}

RTPResenderEntry*	RTPPacketResender::GetEmptyEntry(UInt16 inSeqNum, UInt32 inPacketSize)
{
	if (fPacketsInList == 0)
		fHighestSeqNum = fStartSeqNum = inSeqNum;

	SInt16 theSeqNumDifference = inSeqNum - fStartSeqNum;
	if (theSeqNumDifference < 0)
	{
		//
		// We've gotten an out of order packet from a module,
		// and this new packet is actually before the first packet
		// in our window, so we need to adjust the fStartSeqNum to be
		// this seq num, and reallocate the array possibly
#if RTP_PACKET_RESENDER_DEBUGGING
		UInt32 loopCount1 = 0;
		printf("found a way out of order seq num\n");
#endif
		if ((fHighestSeqNum - inSeqNum) >= (SInt32)fPacketArraySize) // changed from while to keep from killing server
		//while ((fHighestSeqNum - inSeqNum) >= (SInt32)fPacketArraySize) 
		{
#if RTP_PACKET_RESENDER_DEBUGGING
			loopCount1++;
			Assert(loopCount1 < 2);
#endif
			this->ReallocatePacketArray();
		}
		
		fStartSeqNum = inSeqNum;
	}

#if RTP_PACKET_RESENDER_DEBUGGING
	UInt32 loopCount2 = 0;
#endif
	if (theSeqNumDifference >= (SInt32)fPacketArraySize) // change from while to keep from killing server
	//while (theSeqNumDifference >= (SInt32)fPacketArraySize)
	{
		//
		// If the difference between the highest and lowest
		// sequence numbers is bigger than our array, we have to
		// resize the array until all the packets in this range can fit.
#if RTP_PACKET_RESENDER_DEBUGGING
		loopCount2++;
		Assert(loopCount2 < 2);
#endif
		this->ReallocatePacketArray();
	}

	//
	// Readjust the highest seq num if necessary
	SInt16 isHighest = inSeqNum - fHighestSeqNum;
	if (isHighest > 0)
		fHighestSeqNum = inSeqNum;
	
#if RTP_PACKET_RESENDER_DEBUGGING
	SInt16 debugDifference = fHighestSeqNum - fStartSeqNum;
	Assert(debugDifference >= 0);
	Assert(debugDifference < (SInt32)fPacketArraySize);
#endif	

	RTPResenderEntry* theEntry = &fPacketArray[inSeqNum & fPacketArrayMask];

	if (theEntry->fPacketSize > 0) // to preven re-using existing packets
	{	//printf("reallocating current packet seq = %u new seq=%u\n",theEntry->fSeqNum, inSeqNum);
		//Assert(0);
		RemovePacket(theEntry);
	}

	//
	// If there was a special buffer, delete it
	if (theEntry->fIsSpecialBuffer)
	{
		delete [] (char*)theEntry->fPacketData;
		theEntry->fPacketData = NULL;
	}
	theEntry->fIsSpecialBuffer = false;
		
	//
	// This buffer, by default, is not special, it's from the buffer pool
	if (theEntry->fPacketData == NULL)
		theEntry->fPacketData = sBufferPool.Get();
		
	//
	// Check to see if this packet is too big for the buffer. If it is, then
	// we need to specially allocate a special buffer
	if (inPacketSize > kMaxDataBufferSize)
	{
		sBufferPool.Put(theEntry->fPacketData);
		theEntry->fIsSpecialBuffer = true;
		theEntry->fPacketData = NEW char[inPacketSize];
	}
		

	return theEntry;
}

void RTPPacketResender::ReallocatePacketArray()
{
	if (fPacketArraySize >= 256) // to keep from bloating memory
	{	//Assert(0);
		printf("max arraysize = %lu\n",fPacketArraySize);
		return;
	}
	//
	// Figure out how big the packet array needs to be
	UInt32 newArraySize = fPacketArraySize * 2;
	UInt32 newArrayMask = newArraySize - 1;
	//printf("RTPPacketResender::ReallocatePacketArray newArraySize=%lu\n",newArraySize);
	//
	// Create a new array
	RTPResenderEntry* newArray = (RTPResenderEntry*) NEW char[sizeof(RTPResenderEntry) * newArraySize];
	::memset(newArray,0,sizeof(RTPResenderEntry) * newArraySize); //fixes initialization bug

	//
	// Copy the old entries into the new array
	for (UInt16 y = fStartSeqNum; y != (fHighestSeqNum + 1); y++)
	{
		newArray[y & newArrayMask] = fPacketArray[y & fPacketArrayMask];
#if RTP_PACKET_RESENDER_DEBUGGING
		fPacketArray[y & fPacketArrayMask].fPacketData = NULL;
#endif
	}
	
	//
	// If there are any lingering buffers, make sure to copy those too.
	for (UInt16 z = fHighestSeqNum + 1; (z & fPacketArrayMask) != (fStartSeqNum & fPacketArrayMask); z++)
	{
		newArray[z & newArrayMask].fPacketData = fPacketArray[z & fPacketArrayMask].fPacketData;
#if RTP_PACKET_RESENDER_DEBUGGING
		Assert(newArray[z & newArrayMask].fPacketSize == 0);
		Assert(fPacketArray[z & fPacketArrayMask].fPacketSize == 0);
		fPacketArray[z & fPacketArrayMask].fPacketData = NULL;
#endif
	}
	
#if RTP_PACKET_RESENDER_DEBUGGING
	//
	// Make sure we aren't leaking any buffers
	for (UInt32 a = 0; a < fPacketArraySize; a++)
		Assert(fPacketArray[a].fPacketData == NULL);
#endif
		
	delete [] fPacketArray;

	fPacketArray = newArray;
	fPacketArraySize = newArraySize;
	fPacketArrayMask = newArrayMask;
	
	//Assert(fPacketArraySize < 2048);
}

void RTPPacketResender::ClearOutstandingPackets()
{
	if (fPacketsInList == 0)
		return;

	for (UInt16 curSeqNum = fStartSeqNum; curSeqNum <= fHighestSeqNum; curSeqNum++)
	{
		UInt32 packetIndex = curSeqNum & fPacketArrayMask;
		RTPResenderEntry* theEntry = &fPacketArray[packetIndex];
		if (theEntry->fPacketSize > 0)
			this->RemovePacket(theEntry);
	}
	Assert(fPacketsInList == 0);
}

void RTPPacketResender::AddPacket( void * inRTPPacket, UInt32 packetSize, SInt32 ageLimit )
{
	// the caller needs to adjust the overall age limit by reducing it
	// by the current packet lateness.
	
	// we compute a re-transmit timeout based on the Karns RTT esmitate
	
	UInt16* theSeqNumP = (UInt16*)inRTPPacket;
	UInt16 theSeqNum = ntohs(theSeqNumP[1]);
	
	if ( ageLimit > 0 )
	{	
		RTPResenderEntry* theEntry = this->GetEmptyEntry(theSeqNum, packetSize);

		//
		// This may happen if this sequence number has already been added.
		// That may happen if we have repeat packets in the stream.
		if (theEntry->fPacketSize > 0)
			return;
			
		//
		// Reset all the information in the RTPResenderEntry
		::memcpy(theEntry->fPacketData, inRTPPacket, packetSize);
		theEntry->fPacketSize = packetSize;
		theEntry->fAddedTime = OS::Milliseconds();
		theEntry->fOrigRetransTimeout = fBandwidthTracker->CurRetransmitTimeout();
		theEntry->fExpireTime = theEntry->fAddedTime + ageLimit;
		theEntry->fNumResends = 0;
		theEntry->fSeqNum = theSeqNum;
#if RTP_PACKET_RESENDER_DEBUGGING
		theEntry->fPacketArraySizeWhenAdded = fPacketArraySize;
#endif
		
		//
		// Track the number of wasted bytes we have
		atomic_add(&sNumWastedBytes, kMaxDataBufferSize - packetSize);
		
		//PLDoubleLinkedListNode<RTPResenderEntry> * listNode = NEW PLDoubleLinkedListNode<RTPResenderEntry>( new RTPResenderEntry(inRTPPacket, packetSize, ageLimit, fRTTEstimator.CurRetransmitTimeout() ) );
		//fAckList.AddNodeToTail(listNode);
		fBandwidthTracker->FillWindow(packetSize);
		fPacketsInList++;
		if (fPacketsInList > fMaxPacketsInList)
			fMaxPacketsInList = fPacketsInList;
	}
	else
	{
#if RTP_PACKET_RESENDER_DEBUGGING	
		this->logprintf( "packet too old to add: seq# %li, age limit %li, cur late %li, track id %li\n", (long)ntohs( *((UInt16*)(((char*)inRTPPacket)+2)) ), (long)ageLimit, fCurrentPacketDelay, fTrackID );
#endif
		fNumExpired++;
	}
	fNumSent++;
}

void RTPPacketResender::AckPacket( UInt16 inSeqNum, SInt64& inCurTimeInMsec )
{
	UInt32 packetIndex = inSeqNum & fPacketArrayMask;
	RTPResenderEntry* theEntry = &fPacketArray[packetIndex];

	//
	// If the expected sequence number is not the same as the sequence number
	// of this entry, ignore this ACK. If there is a bug in the Packet Array logic
	// of this class, returning here has the danger of not considering valid ACKs.
	// However, there is a legit situation where this can happen. If an ACK arrives
	// for a now dead connection, there is a risk that the ACK could be routed to a new
	// connection, in which case it will screw up the Packet Array. So, we really need
	// to discard all ACKs if the sequence numbers don't match up.
	if (inSeqNum != theEntry->fSeqNum)
		return;
		
	SInt16 theSeqNumDifference = inSeqNum - fStartSeqNum;
	if ((theSeqNumDifference < 0) || (inSeqNum >= (fStartSeqNum + fPacketArraySize)) ||
		(theEntry->fPacketSize == 0))
	{	/* 	we got an ack for a packet that has already expired or
			for a packet whose re-transmit crossed with it's original ack
	
		*/
		fNumAcksForMissingPackets++;
#if RTP_PACKET_RESENDER_DEBUGGING	
		this->logprintf( "acked packet not found: %li, track id %li, OS::MSecs %li\n"
		, (long)inSeqNum, fTrackID, (long)OS::Milliseconds()
		 );
#endif
		//printf("Ack for missing packet: %d\n", inSeqNum);
		 
		 // hmm.. we -should not have- closed down the window in this case
		 // so reopen it a bit as we normally would.
		 // ?? who know's what it really was, just use kMaximumSegmentSize
		 fBandwidthTracker->EmptyWindow( RTPBandwidthTracker::kMaximumSegmentSize, false );

		// when we don't add an estimate from re-transmitted segments we're actually *underestimating* 
		// both the variation and srtt since we're throwing away ALL estimates above the current RTO!
		// therefore it's difficult for us to rapidly adapt to increases in RTT, as well as RTT that
		// are higher than our original RTO estimate.
		
		// for duplicate acks, use 1.5x the cur RTO as the RTT sample
		//fRTTEstimator.AddToEstimate( fRTTEstimator.CurRetransmitTimeout() * 3 / 2 );
		/// this results in some very very big RTO's since the dupes come in batches of maybe 10 or more!
	}
	else
	{
#if RTP_PACKET_RESENDER_DEBUGGING
		Assert(inSeqNum == theEntry->fSeqNum);
		this->logprintf( "Ack for packet: %li, track id %li, OS::MSecs %qd\n"
		, (long)inSeqNum, fTrackID, OS::Milliseconds()
		 );
#endif

		if ( theEntry->fNumResends == 0 )
		{
			// add RTT sample...		
			// only use rtt from packets acked after their initial send, do not use
			// estimates gatherered from re-trasnmitted packets.
			//fRTTEstimator.AddToEstimate( theEntry->fPacketRTTDuration.DurationInMilliseconds() );
			fBandwidthTracker->AddToRTTEstimate( inCurTimeInMsec - theEntry->fAddedTime );
		}
		else
		{
	#if RTP_PACKET_RESENDER_DEBUGGING
			this->logprintf( "re-tx'd packet acked.  ack num : %li, pack seq #: %li, num resends %li, track id %li, size %li, OS::MSecs %qd\n" \
			, (long)inSeqNum, (long)ntohs( *((UInt16*)(((char*)theEntry->fPacketData)+2)) ), (long)theEntry->fNumResends
			, (long)fTrackID, theEntry->fPacketSize, OS::Milliseconds() );
	#endif
		}
		this->RemovePacket(theEntry);
	}
}

void RTPPacketResender::RemovePacket(RTPResenderEntry* inEntry)
{
	//
	// Track the number of wasted bytes we have
	atomic_sub(&sNumWastedBytes, kMaxDataBufferSize - inEntry->fPacketSize);

	Assert(inEntry->fPacketSize > 0);
	fPacketsInList--;
	fBandwidthTracker->EmptyWindow(inEntry->fPacketSize);
	inEntry->fPacketSize = 0;
	UInt16 fEndNum = fStartSeqNum + 128;
	//
	// Update our list information
	if ((fPacketsInList > 0) && (inEntry->fSeqNum == fStartSeqNum))
	{
		while (fPacketArray[fStartSeqNum & fPacketArrayMask].fPacketSize == 0)
		{	fStartSeqNum++;
			if ( fEndNum == fStartSeqNum) break;
		}
	}
}

void RTPPacketResender::ResendDueEntries()
{
	if (fPacketsInList == 0)
		return;
		
	//
	// Start at the lowest seq number, resend until we find one that hasn't been resent,
	// and whose retransmit time is in the future. At that point, we know all subsequent
	// packets have a retransmit time in the future (because we assume that they are added in order by time
	UInt16 curSeqNum = fStartSeqNum;
	UInt16 highestSeqPlusOne = fHighestSeqNum + 1;
	SInt64 curTime = OS::Milliseconds();
	
	while (true)
	{
		if (curSeqNum == highestSeqPlusOne)
			break;

		RTPResenderEntry* thePacket = &fPacketArray[curSeqNum & fPacketArrayMask];
		if (thePacket->fPacketSize == 0)
		{
			curSeqNum++;
			continue;
		}
		
		if (curTime > thePacket->fExpireTime)
		{
#if RTP_PACKET_RESENDER_DEBUGGING	
			unsigned char version;
			version = *((char*)thePacket->fPacketData);
			version &= 0x84; 	// grab most sig 2 bits
			version = version >> 6;	// shift by 6 bits
			this->logprintf( "expired:  seq number %li, track id %li (port: %li), vers # %li, pack seq # %li, size: %li, OS::Msecs: %qd\n", \
								(long)ntohs( *((UInt16*)(((char*)thePacket->fPacketData)+2)) ), fTrackID,  (long) ntohs(fDestPort), \
								(long)version, (long)ntohs( *((UInt16*)(((char*)thePacket->fPacketData)+2))), thePacket->fPacketSize, OS::Milliseconds() );
#endif
			//
			// This packet is expired
			fNumExpired++;
			//printf("Packet expired: %d\n", ((UInt16*)thePacket)[1]);

			this->RemovePacket(thePacket);
		}
		else if ((curTime - thePacket->fAddedTime) > fBandwidthTracker->CurRetransmitTimeout())
		{
			//
			// Resend this packet
			fSocket->SendTo(fDestAddr, fDestPort, thePacket->fPacketData, thePacket->fPacketSize);
			//printf("Packet resent: %d\n", ((UInt16*)thePacket)[1]);

			thePacket->fNumResends++;
			
	#if RTP_PACKET_RESENDER_DEBUGGING	
			this->logprintf( "re-sent: %li RTO %li, track id %li (port %li), size: %li, OS::Ms %qd\n", (long)ntohs( *((UInt16*)(((char*)thePacket->fPacketData)+2)) ),  curTime - thePacket->fAddedTime, \
					fTrackID, (long) ntohs(fDestPort) \
					, thePacket->fPacketSize, OS::Milliseconds());
	#endif		
			fNumResends++;
			
			
			// ok -- lets try this.. add 1.5x of the INITIAL duration since the last send to the rto estimator
			// since we won't get an ack on this packet
			// this should keep us from exponentially increasing due o a one time increase
			// in the actuall rtt, only AddToEstimate on the first resend ( assume that it's a dupe )
			// if it's not a dupe, but rather an actual loss, the subseqnuent actuals wil bring down the average quickly
			
			if ( thePacket->fNumResends == 1 )
				fBandwidthTracker->AddToRTTEstimate( (thePacket->fOrigRetransTimeout  * 3) / 2 );
			
			thePacket->fAddedTime = curTime;
			fBandwidthTracker->AdjustWindowForRetransmit();
		}
		else if (thePacket->fNumResends == 0)
			break;
			
		curSeqNum++;
	}
}
#endif


#if 0
// Old queue code
void RTPPacketResender::ResendEntryIfDue( PLDoubleLinkedListNode<RTPResenderEntry> *listNode, void * ackList /*RTPPacketResender*/)
{
	/*
		static function called by "ForEach" to send all pending
		packets for a given ack list
	*/

	RTPPacketResender *curAckList = (RTPPacketResender*)ackList;
	
	// thin the late packets first
	if ( !curAckList->IsPacketGoodToSend( listNode ) )
	{
#if RTP_PACKET_RESENDER_DEBUGGING	
		curAckList->logprintf( "thinned: %li, track id %li\n", (long)listNode->fElement->fSequenceNumber, curAckList->fTrackID );
#endif		
		curAckList->fAckList.RemoveNode(listNode);
		curAckList->fBytesInList -= listNode->fElement->fPacketSize;
		curAckList->UpdateCongestionWindow(listNode->fElement->fPacketSize);	
		
		delete listNode;
		
		
	} // then dump the one's who are stale
	else if ( listNode->fElement->fExpirationTimer.Expired() )
	{
#if RTP_PACKET_RESENDER_DEBUGGING	
		unsigned char version;
		version = *listNode->fElement->fPacketData;
		version &= 0x84; 	// grab most sig 2 bits
		version = version >> 6;	// shift by 6 bits
		curAckList->logprintf( "expired:  seq number %li, track id %li (port: %li), vers # %li, pack seq # %li, size: %li, OS::Msecs: %qd\n", \
							(long)listNode->fElement->fSequenceNumber, curAckList->fTrackID,  (long) ntohs(curAckList->fDestPort), \
							(long)version, (long)ntohs( *((UInt16*)(listNode->fElement->fPacketData+2))), listNode->fElement->fPacketSize, OS::Milliseconds() );
#endif
		curAckList->fNumExpired++;
		
		curAckList->fAckList.RemoveNode(listNode);
		curAckList->fBytesInList -= listNode->fElement->fPacketSize;
		curAckList->UpdateCongestionWindow(listNode->fElement->fPacketSize);	
		
		delete listNode;
	} // now resend those that are fresh and due.
	// try usign the same RTO for all in the queue... this lets us use a possibly better estimate for the packets out on the wire
	else if ( listNode->fElement->fLastResendDuration.DurationInMilliseconds() > curAckList->fRTTEstimator.CurRetransmitTimeout() )
	{
		curAckList->fSocket->SendTo(curAckList->fDestAddr, curAckList->fDestPort, listNode->fElement->fPacketData, listNode->fElement->fPacketSize);
		listNode->fElement->fNumResends++;
		
#if RTP_PACKET_RESENDER_DEBUGGING	
		curAckList->logprintf( "re-sent: %li RTO %li, track id %li (port %li), size: %li, OS::Ms %li\n", (long)listNode->fElement->fSequenceNumber,  (long)listNode->fElement->fResendTimer.MaxDuration(), \
				curAckList->fTrackID, (long) ntohs(curAckList->fDestPort) \
				, listNode->fElement->fPacketSize, (long)OS::Milliseconds());
#endif		
		curAckList->fNumResends++;
		
		
		// ok -- lets try this.. add 1.5x of the INITIAL duration since the last send to the rto estimator
		// since we won't get an ack on this packet
		// this should keep us from exponentially increasing due o a one time increase
		// in the actuall rtt, only AddToEstimate on the first resend ( assume that it's a dupe )
		// if it's not a dupe, but rather an actual loss, the subseqnuent actuals wil bring down the average quickly
		
		if ( listNode->fElement->fNumResends == 1 )
			curAckList->fRTTEstimator.AddToEstimate( listNode->fElement->fResendTimer.MaxDuration()  * 3 / 2 );
		
		listNode->fElement->fLastResendDuration.Reset();
		listNode->fElement->fResendTimer.ResetTo( curAckList->fRTTEstimator.CurRetransmitTimeout() );
		
		// slow start says that we should reduce the new ss threshold to 1/2
		// of where started getting errors ( the current congestion window size )
		
		
		// so, we get a burst of re-tx becuase our RTO was mis-estimated
		// it doesn't seem like we should lower the threshold for each one.
		// it seems like we should just lower it when we first enter
		// the re-transmit "state" 
		if ( !curAckList->fIsRetransmitting )
			curAckList->fSlowStartThreshold = curAckList->fCongestionWindow/2;
		
		// make sure that it is at least 1 packet
		if ( curAckList->fSlowStartThreshold < kMaximumSegmentSize )
			curAckList->fSlowStartThreshold = kMaximumSegmentSize;
	
		// start the full window segemnt counter over again.
		curAckList->fSlowStartByteCount = 0;
		
		
		// tcp resets to one (max segment size) mss, but i'm experimenting a bit
		// with not being so brutal.
		
		//curAckList->fCongestionWindow = kMaximumSegmentSize;
		
		if ( curAckList->fSlowStartThreshold < curAckList->fCongestionWindow )
			curAckList->fCongestionWindow = curAckList->fSlowStartThreshold/2;
		else
			curAckList->fCongestionWindow = curAckList->fCongestionWindow /2;
			
		if ( curAckList->fCongestionWindow < kMaximumSegmentSize )
			curAckList->fCongestionWindow = kMaximumSegmentSize;
		
	
		curAckList->fIsRetransmitting = true;
		
	}

}		

bool RTPPacketResender::CompareEntryToSequenceNumber( PLDoubleLinkedListNode<RTPResenderEntry> *node, void * sequenceNumber /*UInt16*/)
{
	
	if ( node->fElement->fSequenceNumber == *(UInt16*)sequenceNumber )
		return true;
	return false;
}		




Bool16 RTPPacketResender::IsPacketGoodToSend(PLDoubleLinkedListNode<RTPResenderEntry> * /*listNode*/ )
{
	// our thinning wants to give re-transmit priority to more valuable packets
	// when we begin to fall behind.  we fall behind when our immediate bandwidth
	// requirements are higher than the available bandwidth this may be because
	// of a spike in the movie, flow control pressure from congestion, or
	// a spike created by retransmissions due to lost packets.
	
	// the thinning may be distorted away from optimal if the RTT increase
	// significantly from the original projection.  This will result in a reduced
	// number of actual re-transmit attempts across the board.


	/*
		right now we can't thin because we done't know enough about the 
		packet types we've been asked to write.
		
		we realy need to have write called with a small amount of opaque meta
		data that we can use to call back to the module and ask -it-
		if a given packet should be thinned or not.
		
		all the data needed to make the thinning decision lives in the sending module
		-current lateness, packet types, type dependencies and perhaps more.
		
		so for now, we continue to re-tx packets until they expire which basically forces
		the thinning on to future packets as this generates flow control back pressure.
	*/
	return true;
	
	/*
	SInt32	maxPacketAge;
	
	
	if ( listNode->fElement->fPacket->fPacketType == kPacketIsKeyFrame )
	{
		maxPacketAge = 3 * listNode->fElement->fResendTimer.MaxDuration();
	}
	else if ( listNode->fElement->fPacket->fElement->fPacketType == kPacketIsPFrame )
	{
		// 1.5 x RTO
		maxPacketAge = 3 * listNode->fElement->fResendTimer.MaxDuration()/2;
	}
	else if ( listNode->fElement->fPacket->fPacketType == kPacketIsBFrame )
	{
		maxPacketAge = 1 * listNode->fElement->fResendTimer.MaxDuration();
	}
	else	// audio and other packets like text, are kept until they expires
		return true;
	
	// others are weighted by a number of retransmits to give a maximum age
	// which is compared to the current lateness in sending fresh packets.
	if ( maxPacketAge > fCurrentLateness )
		return true;
	
	return false;
	*/
}
#endif
