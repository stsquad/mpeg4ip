/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
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

#pragma mark __REFLECTOR_STREAM__

static ReflectorSocketPool	sSocketPool;

// ATTRIBUTES

static QTSS_AttributeID 		sCantBindReflectorSocketErr = qtssIllegalAttrID;
static QTSS_AttributeID 		sCantJoinMulticastGroupErr 	= qtssIllegalAttrID;

// PREFS

static UInt32 				sDefaultDelayInMsec 		= 100;
static UInt32 				sDefaultBucketSize 			= 16;

UInt32 				ReflectorStream::sDelayInMsec 		= 100;
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
	QTSSModuleUtils::GetPref(inPrefs, "reflector_delay", qtssAttrDataTypeUInt32,
								&sDelayInMsec, &sDefaultDelayInMsec, sizeof(sDelayInMsec));
	QTSSModuleUtils::GetPref(inPrefs, "reflector_bucket_size", 	qtssAttrDataTypeUInt32,
								&sBucketSize, &sDefaultBucketSize, 	sizeof(sBucketSize));
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
	fStreamInfo(*inInfo),	//copy construct the streamInfo
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
#if REFLECTOR_SESSION_DEBUGGING
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
				
#if REFLECTOR_SESSION_DEBUGGING
				printf("Removing output from bucket %ld, index %ld\n",x,y);
#endif
				fNumElements--;
				return;				
			}
		}
	}
	Assert(0);
}



QTSS_Error ReflectorStream::BindSockets(QTSS_RTSPRequestObject inRequest)
{
	// If the incoming data is RTSP interleaved, we don't need to do anything here
	if (fStreamInfo.fIsTCP)
		return QTSS_NoErr;
		
	// get a pair of sockets. The socket must be bound on INADDR_ANY because we don't know
	// which interface has access to this broadcast. If there is a source IP address
	// specified by the source info, we can use that to demultiplex separate broadcasts on
	// the same port. If the src IP addr is 0, we cannot do this and must dedicate 1 port per
	// broadcast
	fSockets = sSocketPool.GetUDPSocketPair(INADDR_ANY, fStreamInfo.fPort, fStreamInfo.fSrcIPAddr, 0);
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
	
	//also put this stream onto the socket's queue of streams
	((ReflectorSocket*)fSockets->GetSocketA())->AddSender(&fRTPSender);
	((ReflectorSocket*)fSockets->GetSocketB())->AddSender(&fRTCPSender);

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


#pragma mark __REFLECTOR_SENDER__


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



#if REFLECTOR_SESSION_DEBUGGING
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
#if DEBUG
	Assert(ioWakeupTime != NULL);
#endif

	#if REFLECTOR_SESSION_DEBUGGING > 2
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
		#if REFLECTOR_SESSION_DEBUGGING > 2
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
				
				#if REFLECTOR_SESSION_DEBUGGING > 1
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
						
					#if REFLECTOR_SESSION_DEBUGGING
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
					
					#if REFLECTOR_SESSION_DEBUGGING > 2
					printf("packet time: %li, packetSeq %i\n", (long)thePacket->fTimeArrived, DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
					#endif
					
					// once we see a packet we cant' send, we need to stop trying
					// during this pass mark remaining as still needed
					if ( !dodBookmarkPacket )
					{
						SInt64  packetLateness =  currentTime - thePacket->fTimeArrived - (ReflectorStream::sDelayInMsec * (SInt64)bucketIndex);
						SInt64	whenToSend; 
					
						whenToSend = this->ThePacketIsToEarlyForTheBucket( thePacket, bucketIndex, currentTime );	// inline function
						
						if ( whenToSend != 0 )
						{	
							#if REFLECTOR_SESSION_DEBUGGING > 2
							printf("packet held: too early packetLateness %li\n" , (long)packetLateness);
							#endif	
							
							// tag it and bookmark it
							thePacket->fNeededByOutput = true;
							
							Assert( availBookmarksPosition != -1 );							
							if ( availBookmarksPosition != -1 )
								theOutput->fBookmarkedPacketsElemsArray[availBookmarksPosition] =  packetElem;
							
							dodBookmarkPacket = true;
							
							// adjust our next run time to match the
							// packet that needs to go soonest
							if ( fNextTimeToRun > whenToSend )
								fNextTimeToRun = whenToSend;
						}				
						else
						{
							// packetLateness measures how late this packet it after being corrected for the bucket delay
							
							#if REFLECTOR_SESSION_DEBUGGING > 2
							printf("packetLateness %li, seq# %li\n", (long)packetLateness, (long) DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
							#endif
						
							err = theOutput->WritePacket(&thePacket->fPacketPtr, fStream, fWriteFlag, packetLateness );
						
							if ( err == EAGAIN )
							{	
								#if REFLECTOR_SESSION_DEBUGGING > 2
								printf("EAGAIN bookmark: %li, packetSeq %i\n", (long)packetLateness, DGetPacketSeqNumber( &thePacket->fPacketPtr ) );			
								#endif
								// tag it and bookmark it
								thePacket->fNeededByOutput = true;
								
								Assert( availBookmarksPosition != -1 );
								if ( availBookmarksPosition != -1 )
									theOutput->fBookmarkedPacketsElemsArray[availBookmarksPosition] =  packetElem;

								dodBookmarkPacket = true;
								
								// call us again in # ms to retry on an EAGAIN
								if ( fNextTimeToRun > 10 )
									fNextTimeToRun = 10;
							}
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
	
	#if REFLECTOR_SESSION_DEBUGGING > 2
	if ( printQueueLenOnExit )
		printf( "EXIT fPacketQueue len %li\n", (long)fPacketQueue.GetLength() );
	#endif
}


#pragma mark __REFLECTOR_SOCKET__

UDPSocketPair* ReflectorSocketPool::ConstructUDPSocketPair()
{
	return NEW UDPSocketPair
		(NEW ReflectorSocket(), NEW ReflectorSocket());
}

void ReflectorSocketPool::DestructUDPSocketPair(UDPSocketPair *inPair)
{
	//The socket's run function may be executing RIGHT NOW! So we can't
	//just delete the thing, we need to send the sockets kill events.
	Assert(inPair->GetSocketA()->GetLocalPort() > 0);
	((ReflectorSocket*)inPair->GetSocketA())->Signal(Task::kKillEvent);
	((ReflectorSocket*)inPair->GetSocketB())->Signal(Task::kKillEvent);
	delete inPair;
}

ReflectorSocket::ReflectorSocket()
: IdleTask(), UDPSocket(this, Socket::kNonBlockingSocketType | UDPSocket::kWantsDemuxer), fSleepTime(0)
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

void ReflectorSocket::GetIncomingData(const SInt64& inMilliseconds)
{
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
		if (thePacket->fPacketPtr.Len == 0)
		{
			//put the packet back on the free queue, because we didn't actually
			//get any data here.
			fFreeQueue.EnQueue(&thePacket->fQueueElem);
			this->RequestEvent(EV_RE);
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
				continue;
			}
		}
	
		// Find the appropriate ReflectorSender for this packet.
		ReflectorSender* theSender = (ReflectorSender*)this->GetDemuxer()->GetTask(theRemoteAddr, 0);
		// If there is a generic sender for this socket, use it.
		if (theSender == NULL)
			theSender = (ReflectorSender*)this->GetDemuxer()->GetTask(0, 0);
		
		if (theSender != NULL)
		{
			//Before putting this on the sender's packet queue, make sure this isn't
			//a duplicate of a previous packet. Only do this for RTP packets, using
			//the RTP sequence number
			if (!(this->GetLocalPort() & 1))
			{
				UInt16* theSeqNumberP = (UInt16*)thePacket->fPacketPtr.Ptr;
				
				//AddSequenceNumber returns true if we've seen this sequence number before
				if (theSender->fStream->fSequenceNumberMap.AddSequenceNumber(ntohs(theSeqNumberP[1])))
					theSender = NULL;
				else
					// Because this is an RTP packet, and it's not a duplicate, lets update the
					// ReflectorSession's current byte count. Make sure to atomic add this because
					// multiple sockets can be adding to this variable simultaneously
					(void)atomic_add(&theSender->fStream->fBytesSentInThisInterval, thePacket->fPacketPtr.Len);
			}
		}
		if (theSender != NULL)
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
		}
		//if none of the Reflector senders wanted this packet, make sure not to leak it!
		else
			fFreeQueue.EnQueue(&thePacket->fQueueElem);
	}
}


ReflectorPacket* ReflectorSocket::GetPacket()
{
	if (fFreeQueue.GetLength() == 0)
		//if the port number of this socket is odd, this packet is an RTCP packet.
		return NEW ReflectorPacket();
	else
		return (ReflectorPacket*)fFreeQueue.DeQueue()->GetEnclosingObject();
}
