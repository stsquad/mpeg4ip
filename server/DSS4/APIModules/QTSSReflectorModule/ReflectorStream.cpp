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
	File:		ReflectorStream.cpp

	Contains:	Implementation of object defined in ReflectorStream.h. 
					
	

*/

#include "ReflectorStream.h"
#include "QTSSModuleUtils.h"
#include "OSMemory.h"
#include "SocketUtils.h"
#include "atomic.h"
#include "RTCPPacket.h"
#include "ReflectorSession.h"


#if DEBUG
#define REFLECTOR_STREAM_DEBUGGING 0
#else
#define REFLECTOR_STREAM_DEBUGGING 0
#endif


static ReflectorSocketPool	sSocketPool;

// ATTRIBUTES

static QTSS_AttributeID 		sCantBindReflectorSocketErr = qtssIllegalAttrID;
static QTSS_AttributeID 		sCantJoinMulticastGroupErr 	= qtssIllegalAttrID;

// PREFS

static UInt32 				sDefaultDelayInMsec 		= 100;
static UInt32 				sDefaultBucketSize 			= 16;
static UInt32 				sOverBufferInMsec			= 0; //must be 0 for now. 
UInt32 				ReflectorStream::sDelayInMsec 		= 73;//  100 causes problems
UInt32 				ReflectorStream::sBucketSize		= 16;

void ReflectorStream::Register()
{
	// Add text messages attributes
	static char*		sCantBindReflectorSocket= "QTSSReflectorModuleCantBindReflectorSocket";
	static char*		sCantJoinMulticastGroup	= "QTSSReflectorModuleCantJoinMulticastGroup";
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sCantBindReflectorSocket, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sCantBindReflectorSocket, &sCantBindReflectorSocketErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sCantJoinMulticastGroup, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sCantJoinMulticastGroup, &sCantJoinMulticastGroupErr);
}

void ReflectorStream::Initialize(QTSS_ModulePrefsObject inPrefs)
{
//	too slow to use
// QTSSModuleUtils::GetAttribute(inPrefs, "reflector_delay", qtssAttrDataTypeUInt32,
//								&sDelayInMsec, &sDefaultDelayInMsec, sizeof(sDelayInMsec));
								
//	QTSSModuleUtils::GetAttribute(inPrefs, "reflector_bucket_size", 	qtssAttrDataTypeUInt32,
//								&sBucketSize, &sDefaultBucketSize, 	sizeof(sBucketSize));
}

void	ReflectorStream::GenerateSourceID(SourceInfo::StreamInfo* inInfo, char* ioBuffer)
{
	
	::memcpy(ioBuffer, &inInfo->fSrcIPAddr, sizeof(inInfo->fSrcIPAddr));
	::memcpy(&ioBuffer[sizeof(inInfo->fSrcIPAddr)], &inInfo->fPort, sizeof(inInfo->fPort));
}


ReflectorStream::ReflectorStream(SourceInfo::StreamInfo* inInfo)
: 	fSockets(NULL),
	fRTPSender(this, qtssWriteFlagsIsRTP),
	fRTCPSender(this, qtssWriteFlagsIsRTCP),
	fOutputArray(NULL),
	fNumBuckets(kMinNumBuckets),
	fNumElements(0),
	fBucketMutex(),
	
	fDestRTCPAddr(0),
	fDestRTCPPort(0),
	
	fCurrentBitRate(0),
	fLastBitRateSample(OS::Milliseconds()), // don't calculate our first bit rate until kBitRateAvgIntervalInMilSecs has passed!
	fBytesSentInThisInterval(0),
	
	fRTPChannel(-1),
	fRTCPChannel(-1)
{
	fStreamInfo.Copy(*inInfo);
	
	// ALLOCATE BUCKET ARRAY
	this->AllocateBucketArray(fNumBuckets);

	// WRITE RTCP PACKET
	
	//write as much of the RTCP RR as is possible right now (most of it never changes)
	UInt32 theSsrc = (UInt32)::rand();
	char theTempCName[RTCPSRPacket::kMaxCNameLen];
	UInt32 cNameLen = RTCPSRPacket::GetACName(theTempCName);
	
	//write the RR (just header + ssrc)
	UInt32* theRRWriter = (UInt32*)&fReceiverReportBuffer[0];
	*theRRWriter = htonl(0x80c90001);
	theRRWriter++;
	*theRRWriter = htonl(theSsrc);
	theRRWriter++;

	//SDES length is the length of the CName, plus 2 32bit words, minus 1
	*theRRWriter = htonl(0x81ca0000 + (cNameLen >> 2) + 1);
	theRRWriter++;
	*theRRWriter = htonl(theSsrc);
	theRRWriter++;
	::memcpy(theRRWriter, theTempCName, cNameLen);
	theRRWriter += cNameLen >> 2;
	
	//APP packet format, QTSS specific stuff
	*theRRWriter = htonl(0x80cc0008);
	theRRWriter++;
	*theRRWriter = htonl(theSsrc);
	theRRWriter++;
	*theRRWriter = FOUR_CHARS_TO_INT('Q','T','S','S');
	theRRWriter++;
	*theRRWriter = htonl(0);
	theRRWriter++;
	*theRRWriter = htonl(0x00000004);
	theRRWriter++;
	*theRRWriter = htonl(0x6579000c);
	theRRWriter++;
	
	fEyeLocation = theRRWriter;
	fReceiverReportSize = kReceiverReportSize + kAppSize + cNameLen;
	
	// If the source is a multicast, we should send our receiver reports
	// to the multicast address
	if (SocketUtils::IsMulticastIPAddr(fStreamInfo.fDestIPAddr))
	{
		fDestRTCPAddr = fStreamInfo.fDestIPAddr;
		fDestRTCPPort = fStreamInfo.fPort + 1;
	}
}


ReflectorStream::~ReflectorStream()
{
	Assert(fNumElements == 0);

	if (fSockets != NULL)
	{
		//first things first, let's take this stream off the socket's queue
		//of streams. This will basically ensure that no reflecting activity
		//can happen on this stream.
		((ReflectorSocket*)fSockets->GetSocketA())->RemoveSender(&fRTPSender);
		((ReflectorSocket*)fSockets->GetSocketB())->RemoveSender(&fRTCPSender);
		
		//leave the multicast group. Because this socket is shared amongst several
		//potential multicasts, we don't want to remain a member of a stale multicast
		if (SocketUtils::IsMulticastIPAddr(fStreamInfo.fDestIPAddr))
		{
			fSockets->GetSocketA()->LeaveMulticast(fStreamInfo.fDestIPAddr);
			fSockets->GetSocketB()->LeaveMulticast(fStreamInfo.fDestIPAddr);
		}
		//now release the socket pair
		sSocketPool.ReleaseUDPSocketPair(fSockets);
	}
        
        //printf("Deleting stream %x\n", this);

	//delete every client Bucket
	for (UInt32 y = 0; y < fNumBuckets; y++)
		delete [] fOutputArray[y];
	delete [] fOutputArray;
}

void ReflectorStream::AllocateBucketArray(UInt32 inNumBuckets)
{
	Bucket* oldArray = fOutputArray;
	//allocate the 2-dimensional array
	fOutputArray = NEW Bucket[inNumBuckets];
	for (UInt32 x = 0; x < inNumBuckets; x++)
	{
		fOutputArray[x] = NEW ReflectorOutput*[sBucketSize];
		::memset(fOutputArray[x], 0, sizeof(ReflectorOutput*) * sBucketSize);
	}
	
	//copy over the old information if there was an old array
	if (oldArray != NULL)
	{
		Assert(inNumBuckets > fNumBuckets);
		for (UInt32 y = 0; y < fNumBuckets; y++)
		{
			::memcpy(fOutputArray[y],oldArray[y], sBucketSize * sizeof(ReflectorOutput*));
			delete [] oldArray[y];
		}
		delete [] oldArray;
	}
	fNumBuckets = inNumBuckets;
}


SInt32 ReflectorStream::AddOutput(ReflectorOutput* inOutput, SInt32 putInThisBucket)
{
	OSMutexLocker locker(&fBucketMutex);
	
#if DEBUG
	// We should never be adding an output twice to a stream
	for (UInt32 dOne = 0; dOne < fNumBuckets; dOne++)
		for (UInt32 dTwo = 0; dTwo < sBucketSize; dTwo++)
			Assert(fOutputArray[dOne][dTwo] != inOutput);
#endif

	// If caller didn't specify a bucket, find a bucket
	if (putInThisBucket < 0)
		putInThisBucket = this->FindBucket();
		
	Assert(putInThisBucket >= 0);
	
	if (fNumBuckets <= (UInt32)putInThisBucket)
		this->AllocateBucketArray(putInThisBucket * 2);

	for(UInt32 y = 0; y < sBucketSize; y++)
	{
		if (fOutputArray[putInThisBucket][y] == NULL)
		{
			fOutputArray[putInThisBucket][y] = inOutput;
#if REFLECTOR_STREAM_DEBUGGING 
			printf("Adding new output (0x%lx) to bucket %ld, index %ld,\nnum buckets %li bucketSize: %li \n",(long)inOutput, putInThisBucket, y, (long)fNumBuckets, (long)sBucketSize);
#endif
			fNumElements++;
			return putInThisBucket;
		}
	}
	// There was no empty spot in the specified bucket. Return an error
	return -1;		
}

SInt32 ReflectorStream::FindBucket()
{
	// If we need more buckets, allocate them.
	if (fNumElements == (sBucketSize * fNumBuckets))
		this->AllocateBucketArray(fNumBuckets * 2);
	
	//find the first open spot in the array
	for (SInt32 putInThisBucket = 0; (UInt32)putInThisBucket < fNumBuckets; putInThisBucket++)
	{
		for(UInt32 y = 0; y < sBucketSize; y++)
			if (fOutputArray[putInThisBucket][y] == NULL)
				return putInThisBucket;
	}
	Assert(0);
	return 0;
}

void  ReflectorStream::RemoveOutput(ReflectorOutput* inOutput)
{
	OSMutexLocker locker(&fBucketMutex);
	Assert(fNumElements > 0);
	
	//look at all the indexes in the array
	for (UInt32 x = 0; x < fNumBuckets; x++)
	{
		for (UInt32 y = 0; y < sBucketSize; y++)
		{
			//The array may have blank spaces!
			if (fOutputArray[x][y] == inOutput)
			{
				fOutputArray[x][y] = NULL;//just clear out the pointer
				
#if REFLECTOR_STREAM_DEBUGGING  
				printf("Removing output %x from bucket %ld, index %ld\n",inOutput,x,y);
#endif
				fNumElements--;
				return;				
			}
		}
	}
	Assert(0);
}

void  ReflectorStream::TearDownAllOutputs()
{

	OSMutexLocker locker(&fBucketMutex);
	
	//look at all the indexes in the array
	for (UInt32 x = 0; x < fNumBuckets; x++)
	{
		for (UInt32 y = 0; y < sBucketSize; y++)
		{	ReflectorOutput* theOutputPtr= fOutputArray[x][y];
			//The array may have blank spaces!
			if (theOutputPtr != NULL)
			{	theOutputPtr->TearDown();
#if REFLECTOR_STREAM_DEBUGGING  
				printf("TearDownAllOutputs Removing output from bucket %ld, index %ld\n",x,y);
#endif
			}
		}
	}
}


QTSS_Error ReflectorStream::BindSockets(QTSS_StandardRTSP_Params* inParams, UInt32 inReflectorSessionFlags, Bool16 filterState, UInt32 timeout)
{
	// If the incoming data is RTSP interleaved, we don't need to do anything here
	if (inReflectorSessionFlags & ReflectorSession::kIsPushSession)
		fStreamInfo.fSetupToReceive = true;
			
	QTSS_RTSPRequestObject inRequest = NULL;
	if (inParams != NULL)
		inRequest = inParams->inRTSPRequest;
	
			// Set the transport Type a Broadcaster
	QTSS_RTPTransportType transportType = qtssRTPTransportTypeUDP;
	if (inParams != NULL)
	{	UInt32 theLen = sizeof(transportType);
		(void) QTSS_GetValue(inParams->inRTSPRequest, qtssRTSPReqTransportType, 0, (void*)&transportType, &theLen);
	}
	
	// get a pair of sockets. The socket must be bound on INADDR_ANY because we don't know
	// which interface has access to this broadcast. If there is a source IP address
	// specified by the source info, we can use that to demultiplex separate broadcasts on
	// the same port. If the src IP addr is 0, we cannot do this and must dedicate 1 port per
	// broadcast
	fSockets = sSocketPool.GetUDPSocketPair(INADDR_ANY, fStreamInfo.fPort, fStreamInfo.fSrcIPAddr, 0);
	if ((fSockets == NULL) && fStreamInfo.fSetupToReceive)
	{
		fStreamInfo.fPort = 0;
		fSockets = sSocketPool.GetUDPSocketPair(INADDR_ANY, fStreamInfo.fPort, fStreamInfo.fSrcIPAddr, 0);
	}
	if (fSockets == NULL)
		return QTSSModuleUtils::SendErrorResponse(inRequest, qtssServerInternal,
													sCantBindReflectorSocketErr);

	// If we know the source IP address of this broadcast, we can demux incoming traffic
	// on the same port by that source IP address. If we don't know the source IP addr,
	// it is impossible for us to demux, and therefore we shouldn't allow multiple
	// broadcasts on the same port.
 	if (((ReflectorSocket*)fSockets->GetSocketA())->HasSender() && (fStreamInfo.fSrcIPAddr == 0))
		return QTSSModuleUtils::SendErrorResponse(inRequest, qtssServerInternal,
													sCantBindReflectorSocketErr);
	
	//also put this stream onto the socket's queue of streams
	((ReflectorSocket*)fSockets->GetSocketA())->AddSender(&fRTPSender);
	((ReflectorSocket*)fSockets->GetSocketB())->AddSender(&fRTCPSender);

	// A broadcaster is setting up a UDP session so let the sockets update the session
	if (fStreamInfo.fSetupToReceive &&  qtssRTPTransportTypeUDP == transportType && inParams != NULL)
	{	((ReflectorSocket*)fSockets->GetSocketA())->AddBroadcasterSession(inParams->inClientSession);
		((ReflectorSocket*)fSockets->GetSocketB())->AddBroadcasterSession(inParams->inClientSession);
	}
	
	((ReflectorSocket*)fSockets->GetSocketA())->SetSSRCFilter(filterState, timeout);
	((ReflectorSocket*)fSockets->GetSocketB())->SetSSRCFilter(filterState, timeout);

	
	//If the broadcaster is sending RTP directly to us, we don't
	//need to join a multicast group because we're not using multicast
	if (SocketUtils::IsMulticastIPAddr(fStreamInfo.fDestIPAddr))
	{
		QTSS_Error err = fSockets->GetSocketA()->JoinMulticast(fStreamInfo.fDestIPAddr);
		if (err == QTSS_NoErr)
			err = fSockets->GetSocketB()->JoinMulticast(fStreamInfo.fDestIPAddr);
		// If we get an error when setting the TTL, this isn't too important (TTL on
		// these sockets is only useful for RTCP RRs.
		if (err == QTSS_NoErr)
			(void)fSockets->GetSocketA()->SetTtl(fStreamInfo.fTimeToLive);
		if (err == QTSS_NoErr)
			(void)fSockets->GetSocketB()->SetTtl(fStreamInfo.fTimeToLive);
		if (err != QTSS_NoErr)
			return QTSSModuleUtils::SendErrorResponse(inRequest, qtssServerInternal,
														sCantJoinMulticastGroupErr);
	}
	
	// If the port is 0, update the port to be the actual port value
	fStreamInfo.fPort = fSockets->GetSocketA()->GetLocalPort();

	//finally, register these sockets for events
	fSockets->GetSocketA()->RequestEvent(EV_RE);
	fSockets->GetSocketB()->RequestEvent(EV_RE);
			
	// Copy the source ID and setup the ref
	StrPtrLen theSourceID(fSourceIDBuf, kStreamIDSize);
	ReflectorStream::GenerateSourceID(&fStreamInfo, fSourceIDBuf);
	fRef.Set(theSourceID, this);
	return QTSS_NoErr;
}

void ReflectorStream::SendReceiverReport()
{
	// Check to see if our destination RTCP addr & port are setup. They may
	// not be if the source is unicast and we haven't gotten any incoming packets yet
	if (fDestRTCPAddr == 0)
		return;
	
	UInt32 theEyeCount = fNumElements;
	
	//write the current eye count information based on the array
	//this is not really pre-emptive safe, but who cares... it's just statistics
	UInt32* theEyeWriter = fEyeLocation;
	*theEyeWriter = htonl(theEyeCount) & 0x7fffffff;//no idea why we do this!
	theEyeWriter++;
	*theEyeWriter = htonl(theEyeCount) & 0x7fffffff;
	theEyeWriter++;
	*theEyeWriter = htonl(0) & 0x7fffffff;
	
	//send the packet to the multicast RTCP addr & port for this stream
	(void)fSockets->GetSocketB()->SendTo(fDestRTCPAddr, fDestRTCPPort, fReceiverReportBuffer, fReceiverReportSize);
}

void ReflectorStream::PushPacket(char *packet, UInt32 packetLen, Bool16 isRTCP)
{

	if (packetLen > 0)
	{	
		ReflectorPacket* thePacket = NULL;
		if (isRTCP)
		{	//printf("ReflectorStream::PushPacket RTCP packetlen = %lu\n",packetLen);
			thePacket = ((ReflectorSocket*)fSockets->GetSocketB())->GetPacket();
			if (thePacket == NULL)
			{	//printf("ReflectorStream::PushPacket RTCP GetPacket() is NULL\n");
				return;
			}
			
			OSMutexLocker locker( ((ReflectorSocket*)(fSockets->GetSocketB()) )->GetDemuxer()->GetMutex());
			thePacket->SetPacketData(packet, packetLen);
			((ReflectorSocket*)fSockets->GetSocketB())->ProcessPacket(OS::Milliseconds(),thePacket,0,0);
			//((ReflectorSocket*)fSockets->GetSocketB())->Run();
			((ReflectorSocket*)fSockets->GetSocketB())->Signal(Task::kIdleEvent);
		}
		else
		{	//printf("ReflectorStream::PushPacket RTP packetlen = %lu\n",packetLen);
			thePacket =  ((ReflectorSocket*)fSockets->GetSocketA())->GetPacket();
			if (thePacket == NULL)
			{	//printf("ReflectorStream::PushPacket GetPacket() is NULL\n");
				return;
			}
	
			OSMutexLocker locker(((ReflectorSocket*)(fSockets->GetSocketA()))->GetDemuxer()->GetMutex());
			thePacket->SetPacketData(packet, packetLen);
			 ((ReflectorSocket*)fSockets->GetSocketA())->ProcessPacket(OS::Milliseconds(),thePacket,0,0);
			//((ReflectorSocket*)fSockets->GetSocketA())->Run();
			((ReflectorSocket*)fSockets->GetSocketA())->Signal(Task::kIdleEvent);
		}
	}
}




ReflectorSender::ReflectorSender(ReflectorStream* inStream, UInt32 inWriteFlag)
: 	fStream(inStream),
	fWriteFlag(inWriteFlag),
	fFirstNewPacketInQueue(NULL),
	fHasNewPackets(false),
	fNextTimeToRun(0),
	fLastRRTime(0),
	fSocketQueueElem(this)
{}

ReflectorSender::~ReflectorSender()
{
	//dequeue and delete every buffer
	while (fPacketQueue.GetLength() > 0)
	{
		ReflectorPacket* packet = (ReflectorPacket*)fPacketQueue.DeQueue()->GetEnclosingObject();
		delete packet;
	}
}


Bool16 ReflectorSender::ShouldReflectNow(const SInt64& inCurrentTime, SInt64* ioWakeupTime)
{
	Assert(ioWakeupTime != NULL);
	//check to make sure there actually is work to do for this stream.
	if ((!fHasNewPackets) && ((fNextTimeToRun == 0) || (inCurrentTime < fNextTimeToRun)))
	{
		//We don't need to do work right now, but
		//this stream must still communicate when it needs to be woken up next
		SInt64 theWakeupTime = fNextTimeToRun - inCurrentTime;
		if ((fNextTimeToRun > 0) && (theWakeupTime < *ioWakeupTime))
			*ioWakeupTime = theWakeupTime;
		return false;
	}
	return true;	
}



#if REFLECTOR_STREAM_DEBUGGING
static UInt16 DGetPacketSeqNumber(StrPtrLen* inPacket)
{
	if (inPacket->Len < 4)
		return 0;
	
	//The RTP seq number is the second short of the packet
	UInt16* seqNumPtr = (UInt16*)inPacket->Ptr;
	return ntohs(seqNumPtr[1]);
}



#endif



/***********************************************************************************************
/	ReflectorSender::ReflectPackets
/	
/	There are n ReflectorSender's for n output streams per presentation.
/	
/	Each sender is associated with an array of ReflectorOutput's.  Each
/	output represents a client connection.  Each output has # RTPStream's. 
/	
/	When we write a packet to the ReflectorOutput he matches it's payload
/	to one of his streams and sends it there.
/	
/	To smooth the bandwitdth (server, not user) requirements of the reflected streams, the Sender
/	groups the ReflectorOutput's into buckets.  The input streams are reflected to
/	each bucket progressively later in time.  So rather than send a single packet
/	to say 1000 clients all at once, we send it to just the first 16, then then next 16 
/	100 ms later and so on.
/
/
/	intputs 	ioWakeupTime - relative time to call us again in MSec
/				inFreeQueue - queue of free packets.
*/

void ReflectorSender::ReflectPackets(SInt64* ioWakeupTime, OSQueue* inFreeQueue)
{
	//printf("ReflectorSender::ReflectPackets %qd %qd\n",*ioWakeupTime,fNextTimeToRun);
#if DEBUG
	Assert(ioWakeupTime != NULL);
#endif
	#if REFLECTOR_STREAM_DEBUGGING > 2
	Bool16	printQueueLenOnExit = false;
	#endif	

	SInt64 currentTime = OS::Milliseconds();

	//make sure to reset these state variables
	fHasNewPackets = false;	
	fNextTimeToRun = 10000;	// init to 10 secs
	
	//determine if we need to send a receiver report to the multicast source
	if ((fWriteFlag == qtssWriteFlagsIsRTCP) && (currentTime > (fLastRRTime + kRRInterval)))
	{
		fLastRRTime = currentTime;
		fStream->SendReceiverReport();
		#if REFLECTOR_STREAM_DEBUGGING > 2
		printQueueLenOnExit = true;
		printf( "fPacketQueue len %li\n", (long)fPacketQueue.GetLength() );
		#endif	
	}
	
	//the rest of this function must be atomic wrt the ReflectorSession, because
	//it involves iterating through the RTPSession array, which isn't thread safe
	OSMutexLocker locker(&fStream->fBucketMutex);
	
	// Check to see if we should update the session's bitrate average
	if ((fStream->fLastBitRateSample + ReflectorStream::kBitRateAvgIntervalInMilSecs) < currentTime)
	{
		unsigned int intervalBytes = fStream->fBytesSentInThisInterval;
		(void)atomic_sub(&fStream->fBytesSentInThisInterval, intervalBytes);
		
		// Multiply by 1000 to convert from milliseconds to seconds, and by 8 to convert from bytes to bits
		Float32 bps = (Float32)(intervalBytes * 8) / (Float32)(currentTime - fStream->fLastBitRateSample);
		bps *= 1000;
		fStream->fCurrentBitRate = (UInt32)bps;
		
		// Don't check again for awhile!
		fStream->fLastBitRateSample = currentTime;
	}

	for (UInt32 bucketIndex = 0; bucketIndex < fStream->fNumBuckets; bucketIndex++)
	{	
		for (UInt32 bucketMemberIndex = 0; bucketMemberIndex < fStream->sBucketSize; bucketMemberIndex++)
		{	 
			ReflectorOutput* theOutput = fStream->fOutputArray[bucketIndex][bucketMemberIndex];
		
			
			if (theOutput != NULL)
			{	
				SInt32			availBookmarksPosition = -1;	// -1 == invalid position
				OSQueueElem*	packetElem = NULL;				
				UInt32			curBookmark = 0;
				
				Assert( curBookmark < theOutput->fNumBookmarks );		
				
				// see if we've bookmarked a held packet for this Sender in this Output
				while ( curBookmark < theOutput->fNumBookmarks )
				{					
					OSQueueElem* 	bookmarkedElem = theOutput->fBookmarkedPacketsElemsArray[curBookmark];
					
					if ( bookmarkedElem )	// there may be holes in this array
					{							
						if ( bookmarkedElem->IsMember( fPacketQueue ) )	
						{	
							// this packet was previously bookmarked for this specific queue
							// remove if from the bookmark list and use it
							// to jump ahead into the Sender's over all packet queue						
							theOutput->fBookmarkedPacketsElemsArray[curBookmark] = NULL;
							availBookmarksPosition = curBookmark;
							packetElem = bookmarkedElem;
							break;
						}
						
					}
					else
					{
						availBookmarksPosition = curBookmark;
					}
					
					curBookmark++;
						
				}
				
				Assert( availBookmarksPosition != -1 );		
				
				#if REFLECTOR_STREAM_DEBUGGING > 1
				if ( packetElem )	// show 'em what we got johnny
				{	ReflectorPacket* 	thePacket = (ReflectorPacket*)packetElem->GetEnclosingObject();
					printf("Bookmarked packet time: %li, packetSeq %i\n", (long)thePacket->fTimeArrived, DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
				}
				#endif
				
				// the output did not have a bookmarked packet if it's own
				// so show it the first new packet we have in this sender.
				// ( since TCP flow control may delay the sending of packets, this may not
				// be the same as the first packet in the queue
				if ( packetElem  == NULL )
				{	
					packetElem = fFirstNewPacketInQueue;
						
					#if REFLECTOR_STREAM_DEBUGGING > 1
					if ( packetElem )	// show 'em what we got johnny
					{	
						ReflectorPacket* 	thePacket = (ReflectorPacket*)packetElem->GetEnclosingObject();
						printf("1st NEW packet from Sender sess 0x%lx time: %li, packetSeq %i\n", (long)theOutput, (long)thePacket->fTimeArrived, DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
					}
					else
						printf("no new packets\n" );
					#endif
				}
				
				OSQueueIter qIter(&fPacketQueue, packetElem);  // starts from beginning if packetElem == NULL, else from packetElem
				
				Bool16			dodBookmarkPacket = false;
				
				while ( !qIter.IsDone() )
				{					
					packetElem = qIter.GetCurrent();
					
					ReflectorPacket* 	thePacket = (ReflectorPacket*)packetElem->GetEnclosingObject();
					QTSS_Error			err = QTSS_NoErr;
					
					#if REFLECTOR_STREAM_DEBUGGING > 2
					printf("packet time: %li, packetSeq %i\n", (long)thePacket->fTimeArrived, DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
					#endif
					
					// once we see a packet we cant' send, we need to stop trying
					// during this pass mark remaining as still needed
					if ( !dodBookmarkPacket )
					{
						SInt64  packetLateness =  currentTime - thePacket->fTimeArrived - (ReflectorStream::sDelayInMsec * (SInt64)bucketIndex);
						packetLateness -= sOverBufferInMsec;// Increasing over 500 causes problems. This represents time to buffer/delay packets. Required by reliable UDP for over-buffer support.
						// packetLateness measures how late this packet it after being corrected for the bucket delay
						
						#if REFLECTOR_STREAM_DEBUGGING > 2
						printf("packetLateness %li, seq# %li\n", (long)packetLateness, (long) DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
						#endif
						
						SInt64 timeToSendPacket = -1;
						err = theOutput->WritePacket(&thePacket->fPacketPtr, fStream, fWriteFlag, packetLateness, &timeToSendPacket );
					
						if ( err == QTSS_WouldBlock )
						{	
							#if REFLECTOR_STREAM_DEBUGGING > 2
							printf("EAGAIN bookmark: %li, packetSeq %i\n", (long)packetLateness, DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
							#endif
							// tag it and bookmark it
							thePacket->fNeededByOutput = true;
							
							Assert( availBookmarksPosition != -1 );
							if ( availBookmarksPosition != -1 )
								theOutput->fBookmarkedPacketsElemsArray[availBookmarksPosition] =  packetElem;

							dodBookmarkPacket = true;
							
							// call us again in # ms to retry on an EAGAIN
							if ((timeToSendPacket > 0) && (fNextTimeToRun > timeToSendPacket ))
								fNextTimeToRun = timeToSendPacket;
							if ( timeToSendPacket == -1 )
							//	fNextTimeToRun = 100;
								fNextTimeToRun = 37; // fixes OSX (cpu spike) and smaller is better to avoid synch problems
						}
					}
					else
					{	
						if ( thePacket->fNeededByOutput == true )	// optimization: if the packet is already marked, another Output has been through this already
							break;
						thePacket->fNeededByOutput = true;
					}
					
					qIter.Next();
				} 
				
			}
		}
	}
	
	// reset our first new packet bookmark
	fFirstNewPacketInQueue = NULL;

	// iterate one more through the senders queue to clear out
	// the unneeded packets
	OSQueueIter removeIter(&fPacketQueue);
	while ( !removeIter.IsDone() )
	{
		OSQueueElem* elem = removeIter.GetCurrent();
		Assert( elem );

		//at this point, move onto the next queue element, because we may be altering
		//the queue itself in the code below
		removeIter.Next();

		ReflectorPacket* thePacket = (ReflectorPacket*)elem->GetEnclosingObject();		
		Assert( thePacket );
		
		if ( thePacket->fNeededByOutput == false )
		{	
			thePacket->fNeededByOutput = true;
			fPacketQueue.Remove( elem );
			inFreeQueue->EnQueue( elem );
			
		}
		else	// reset for next call to ReflectPackets
		{
			thePacket->fNeededByOutput = false;
		}
	}
	
	//Don't forget that the caller also wants to know when we next want to run
	if (*ioWakeupTime == 0)
		*ioWakeupTime = fNextTimeToRun;
	else if ((fNextTimeToRun > 0) && (*ioWakeupTime > fNextTimeToRun))
		*ioWakeupTime = fNextTimeToRun;
	// exit with fNextTimeToRun in real time, not relative time.
	fNextTimeToRun += currentTime;
	
	#if REFLECTOR_STREAM_DEBUGGING > 2
	if ( printQueueLenOnExit )
		printf( "EXIT fPacketQueue len %li\n", (long)fPacketQueue.GetLength() );
	#endif
}



UDPSocketPair* ReflectorSocketPool::ConstructUDPSocketPair()
{
	return NEW UDPSocketPair
		(NEW ReflectorSocket(), NEW ReflectorSocket());
}

void ReflectorSocketPool::DestructUDPSocketPair(UDPSocketPair *inPair)
{
	//The socket's run function may be executing RIGHT NOW! So we can't
	//just delete the thing, we need to send the sockets kill events.
	//Assert(inPair->GetSocketA()->GetLocalPort() > 0);
	if (inPair->GetSocketA()->GetLocalPort() > 0)
	{
		((ReflectorSocket*)inPair->GetSocketA())->Signal(Task::kKillEvent);
		((ReflectorSocket*)inPair->GetSocketB())->Signal(Task::kKillEvent);
	}
	delete inPair;
}

ReflectorSocket::ReflectorSocket()
: 	IdleTask(), 
	UDPSocket(this, Socket::kNonBlockingSocketType | UDPSocket::kWantsDemuxer), 
	fBroadcasterClientSession(NULL), 
	fLastBroadcasterTimeOutRefresh(0), 
	fSleepTime(0),
	fValidSSRC(0),
	fLastValidSSRCTime(0),
	fFilterSSRCs(true),
	fTimeoutSecs(30)
{
	//construct all the preallocated packets
	for (UInt32 numPackets = 0; numPackets < kNumPreallocatedPackets; numPackets++)
	{
		//If the local port # of this socket is odd, then all the packets
		//used for this socket are rtcp packets.
		ReflectorPacket* packet = NEW ReflectorPacket();
		fFreeQueue.EnQueue(&packet->fQueueElem);//put this packet onto the free queue
	}
}

ReflectorSocket::~ReflectorSocket()
{
	//printf("ReflectorSocket::~ReflectorSocket\n");
	while (fFreeQueue.GetLength() > 0)
	{
		ReflectorPacket* packet = (ReflectorPacket*)fFreeQueue.DeQueue()->GetEnclosingObject();
		delete packet;
	}
}

void	ReflectorSocket::AddSender(ReflectorSender* inSender)
{
	OSMutexLocker locker(this->GetDemuxer()->GetMutex());
	QTSS_Error err = this->GetDemuxer()->RegisterTask(inSender->fStream->fStreamInfo.fSrcIPAddr, 0, inSender);
	Assert(err == QTSS_NoErr);
	fSenderQueue.EnQueue(&inSender->fSocketQueueElem);
}

void	ReflectorSocket::RemoveSender(ReflectorSender* inSender)
{
	OSMutexLocker locker(this->GetDemuxer()->GetMutex());
	fSenderQueue.Remove(&inSender->fSocketQueueElem);
	QTSS_Error err = this->GetDemuxer()->UnregisterTask(inSender->fStream->fStreamInfo.fSrcIPAddr, 0, inSender);
	Assert(err == QTSS_NoErr);
}

SInt64 ReflectorSocket::Run()
{
	//We want to make sure we can't get idle events WHILE we are inside
	//this function. That will cause us to run the queues unnecessarily
	//and just get all confused.
	this->CancelTimeout();
	
	Task::EventFlags theEvents = this->GetEvents();
	//if we have been told to delete ourselves, do so.
	if (theEvents & Task::kKillEvent)
		return -1;

	OSMutexLocker locker(this->GetDemuxer()->GetMutex());
	SInt64 theMilliseconds = OS::Milliseconds();
	
	//Only check for data on the socket if we've actually been notified to that effect
	if (theEvents & Task::kReadEvent)
		this->GetIncomingData(theMilliseconds);

#if DEBUG
	//make sure that we haven't gotten here prematurely! This wouldn't mess
	//anything up, but it would waste CPU.
	if (theEvents & Task::kIdleEvent)
	{
		SInt32 temp = (SInt32)(fSleepTime - theMilliseconds);
		char tempBuf[20];
		::sprintf(tempBuf,"%ld",temp);
		WarnV(fSleepTime <= theMilliseconds, tempBuf);
	}
#endif
	
	fSleepTime = 0;
	//Now that we've gotten all available packets, have the streams reflect
	for (OSQueueIter iter2(&fSenderQueue); !iter2.IsDone(); iter2.Next())
	{
		ReflectorSender* theSender2 = (ReflectorSender*)iter2.GetCurrent()->GetEnclosingObject();
		if (theSender2->ShouldReflectNow(theMilliseconds, &fSleepTime))
			theSender2->ReflectPackets(&fSleepTime, &fFreeQueue);
	}
	
#if DEBUG
	theMilliseconds = OS::Milliseconds();
#endif
	
	//For smoothing purposes, the streams can mark when they want to wakeup.
	if (fSleepTime > 0)
		this->SetIdleTimer(fSleepTime);
#if DEBUG
	//The debugging check above expects real time.
	fSleepTime += theMilliseconds;
#endif
		
	return 0;
}


void ReflectorSocket::FilterInvalidSSRCs(ReflectorPacket* thePacket,Bool16 isRTCP)
{	// assume the first SSRC we see is valid and all others are to be ignored.
	if ( thePacket->fPacketPtr.Len > 0) do 
	{
		SInt64 currentTime = OS::Milliseconds() / 1000;
		if (0 == fValidSSRC)
		{	fValidSSRC = thePacket->GetSSRC(isRTCP); // SSRC of 0 is allowed
			fLastValidSSRCTime = currentTime;
			//printf("socket=%lu FIRST PACKET fValidSSRC=%lu \n", (long unsigned) this,fValidSSRC);
			break;
		}
	
		UInt32 packetSSRC = thePacket->GetSSRC(isRTCP);
		if (packetSSRC != 0)
		{	
			if (packetSSRC == fValidSSRC)
			{	fLastValidSSRCTime = currentTime;
				//printf("socket=%lu good packet\n", (long unsigned) this );
				break;
			}
			
			//printf("socket=%lu bad packet packetSSRC= %lu fValidSSRC=%lu \n", (long unsigned) this,packetSSRC,fValidSSRC);
			thePacket->fPacketPtr.Len = 0; // ignore this packet wrong SSRC
		}
		
		// this executes whenever an invalid SSRC is found -- maybe the original stream ended and a new one is now active
		if ( (fLastValidSSRCTime + fTimeoutSecs) < currentTime) // fValidSSRC timed out --no packets with this SSRC seen for awhile
		{	fValidSSRC = 0; // reset the valid SSRC with the next packet's SSRC
			//printf("RESET fValidSSRC\n");
		}

	}while (false);
}

Bool16 ReflectorSocket::ProcessPacket(const SInt64& inMilliseconds,ReflectorPacket* thePacket,UInt32 theRemoteAddr,UInt16 theRemotePort)
{	
	Bool16 done = false; // stop when result is true
	if (thePacket != NULL) do
	{
		if (fBroadcasterClientSession != NULL) // alway refresh timeout even if we are filtering.
		{	if ( (inMilliseconds - fLastBroadcasterTimeOutRefresh) > kRefreshBroadcastSessionIntervalMilliSecs)
			{	QTSS_RefreshTimeOut(fBroadcasterClientSession);
				fLastBroadcasterTimeOutRefresh = inMilliseconds;
			}
		}

		// Only reflect one SSRC stream at a time.
		// Pass the packet and whether it is an RTCP or RTP packet based on the port number.
		if (fFilterSSRCs)
			this->FilterInvalidSSRCs(thePacket,GetLocalPort() & 1);// thePacket->fPacketPtr.Len is set to 0 for invalid SSRCs.
	
		if (thePacket->fPacketPtr.Len == 0)
		{
			//put the packet back on the free queue, because we didn't actually
			//get any data here.
			fFreeQueue.EnQueue(&thePacket->fQueueElem);
			this->RequestEvent(EV_RE);
			done = true;
			//printf("ReflectorSocket::ProcessPacket no more packets on this socket!\n");
			break;//no more packets on this socket!
		}
		
		if (GetLocalPort() & 1)
		{
			//if this is a new RTCP packet, check to see if it is a sender report.
			//We should only reflect sender reports. Because RTCP packets can't have both
			//an SR & an RR, and because the SR & the RR must be the first packet in a
			//compound RTCP packet, all we have to do to determine this is look at the
			//packet type of the first packet in the compound packet.
			RTCPPacket theRTCPPacket;
			if ((!theRTCPPacket.ParsePacket((UInt8*)thePacket->fPacketPtr.Ptr, thePacket->fPacketPtr.Len)) ||
				(theRTCPPacket.GetPacketType() != RTCPSRPacket::kSRPacketType))
			{
				//pretend as if we never got this packet
				fFreeQueue.EnQueue(&thePacket->fQueueElem);
				done = true;
				break;
			}
		}
				
		// Find the appropriate ReflectorSender for this packet.
		ReflectorSender* theSender = (ReflectorSender*)this->GetDemuxer()->GetTask(theRemoteAddr, 0);
		// If there is a generic sender for this socket, use it.
		if (theSender == NULL)
			theSender = (ReflectorSender*)this->GetDemuxer()->GetTask(0, 0);
		
		if (theSender == NULL)
		{	
			//UInt16* theSeqNumberP = (UInt16*)thePacket->fPacketPtr.Ptr;
			//printf("ReflectorSocket::ProcessPacket no sender found for packet! sequence number=%d\n",ntohs(theSeqNumberP[1]));
			fFreeQueue.EnQueue(&thePacket->fQueueElem); // don't process the packet
			done = true;
			break;
		}
			
		Assert(theSender != NULL); // at this point we have a sender
			
		if (!(this->GetLocalPort() & 1))
		{
			// don't check for duplicate packets, they may be needed to keep in sync.
			// Because this is an RTP packet make sure to atomic add this because
			// multiple sockets can be adding to this variable simultaneously
			(void)atomic_add(&theSender->fStream->fBytesSentInThisInterval, thePacket->fPacketPtr.Len);
		}

		const UInt32 maxQSize = 4000;
		if (theSender->fPacketQueue.GetLength() < maxQSize) //don't grow memory too big
		{	
			// Check to see if we need to set the remote RTCP address
			// for this stream. This will be necessary if the source is unicast.
			if ((theRemoteAddr != 0) && (theSender->fStream->fDestRTCPAddr == 0))
			{
				// If the source is multicast, this shouldn't be necessary
				Assert(!SocketUtils::IsMulticastIPAddr(theSender->fStream->fStreamInfo.fDestIPAddr));
				Assert(theRemotePort != 0);
				theSender->fStream->fDestRTCPAddr = theRemoteAddr;
				theSender->fStream->fDestRTCPPort = theRemotePort;
				
				// RTCPs are always on odd ports, so check to see if this port is an
				// RTP port, and if so, just add 1.
				if (!(theRemotePort & 1))
					theSender->fStream->fDestRTCPPort++;				
			}

			thePacket->fBucketsSeenThisPacket = 0;
			thePacket->fTimeArrived = inMilliseconds;
			theSender->fPacketQueue.EnQueue(&thePacket->fQueueElem);			
			if ( theSender->fFirstNewPacketInQueue == NULL )
				theSender->fFirstNewPacketInQueue = &thePacket->fQueueElem;					
			theSender->fHasNewPackets = true;
			
			//printf("ReflectorSocket::GetIncomingData has packet from time=%qd src addr=%lu src port=%u packetlen=%lu\n",inMilliseconds, theRemoteAddr,theRemotePort,thePacket->fPacketPtr.Len);
		}
		else
		{	char outMessage[256];
			sprintf(outMessage,"Packet Queue for port=%d hit max qSize=%lu", theRemotePort,maxQSize);
			WarnV(false, outMessage); 

			//UInt16* theSeqNumberP = (UInt16*)thePacket->fPacketPtr.Ptr;
			//printf("ReflectorSocket::ProcessPacket the reflector fPacketQueue is full drop packet sequence number=%d\n",ntohs(theSeqNumberP[1]));
			fFreeQueue.EnQueue(&thePacket->fQueueElem);
			done = true;
			break;
		}


	} while(false);
	
	return done;
}


void ReflectorSocket::GetIncomingData(const SInt64& inMilliseconds)
{
	OSMutexLocker locker(this->GetDemuxer()->GetMutex());
	UInt32 theRemoteAddr = 0;
	UInt16 theRemotePort = 0;
	
	//get all the outstanding packets for this socket
	while (true)
	{
		//get a packet off the free queue.
		ReflectorPacket* thePacket = this->GetPacket();

		thePacket->fPacketPtr.Len = 0;
		(void)this->RecvFrom(&theRemoteAddr, &theRemotePort, thePacket->fPacketPtr.Ptr,
							ReflectorPacket::kMaxReflectorPacketSize, &thePacket->fPacketPtr.Len);

		if (this->ProcessPacket(inMilliseconds,thePacket,theRemoteAddr, theRemotePort))
			break;
			
		//printf("ReflectorSocket::GetIncomingData \n");
	}

}




ReflectorPacket* ReflectorSocket::GetPacket()
{
	OSMutexLocker locker(this->GetDemuxer()->GetMutex());
	if (fFreeQueue.GetLength() == 0)
		//if the port number of this socket is odd, this packet is an RTCP packet.
		return NEW ReflectorPacket();
	else
		return (ReflectorPacket*)fFreeQueue.DeQueue()->GetEnclosingObject();
}
