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
	File:		ReflectorStream.h

	Contains:	This object supports reflecting an RTP multicast stream to N
				RTPStreams. It spaces out the packet send times in order to
				maximize the randomness of the sending pattern and smooth
				the stream.
					
	

*/

#ifndef _REFLECTOR_STREAM_H_
#define _REFLECTOR_STREAM_H_

#include "QTSS.h"

#include "IdleTask.h"
#include "SourceInfo.h"

#include "UDPSocket.h"
#include "UDPSocketPool.h"
#include "UDPDemuxer.h"
#include "EventContext.h"
#include "SequenceNumberMap.h"

#include "OSMutex.h"
#include "OSQueue.h"
#include "OSRef.h"

#include "RTCPSRPacket.h"
#include "ReflectorOutput.h"

//This will add some printfs that are useful for checking the thinning
#define REFLECTOR_THINNING_DEBUGGING 0 


class ReflectorPacket;
class ReflectorSender;
class ReflectorStream;


class ReflectorPacket
{
	public:
		void	SetPacketData(char *data, UInt32 len) {	Assert(kMaxReflectorPacketSize > len); if (len > 0) memcpy(this->fPacketPtr.Ptr,data,len); this->fPacketPtr.Len = len;}
		UInt32  GetSSRC(Bool16 isRTCP) 
		{
			if (fPacketPtr.Ptr == NULL || fPacketPtr.Len < 8)
				return 0;		
						
			UInt32* theSsrcPtr = (UInt32*)fPacketPtr.Ptr;
			if (isRTCP)// RTCP 
				return ntohl(theSsrcPtr[1]); 
					
			if (fPacketPtr.Len < 12)
				return 0;
			
			return ntohl(theSsrcPtr[2]);  // RTP SSRC
		}

	//private: //temp put back later
	
		ReflectorPacket() : fQueueElem(this), fPacketPtr(fPacketData, 0), fNeededByOutput(true) {}
		~ReflectorPacket() {}

		enum
		{
			kMaxReflectorPacketSize = 2048	//UInt32
		};

		UInt32 		fBucketsSeenThisPacket;
		SInt64 		fTimeArrived;
		OSQueueElem	fQueueElem;
		char		fPacketData[kMaxReflectorPacketSize];
		StrPtrLen	fPacketPtr;
		
		Bool16		fNeededByOutput; // is this packet still needed for output?
				
		friend class ReflectorSender;
		friend class ReflectorSocket;
};


//Custom UDP socket classes for doing reflector packet retrieval, socket management
class ReflectorSocket : public IdleTask, public UDPSocket
{
	public:

		ReflectorSocket();
		virtual ~ReflectorSocket();
		void	AddBroadcasterSession(QTSS_ClientSessionObject inSession) { OSMutexLocker locker(this->GetDemuxer()->GetMutex()); fBroadcasterClientSession = inSession; }
		void 	RemoveBroadcasterSession(QTSS_ClientSessionObject inSession){	OSMutexLocker locker(this->GetDemuxer()->GetMutex()); if (inSession == fBroadcasterClientSession) fBroadcasterClientSession = NULL; }
		void	AddSender(ReflectorSender* inSender);
		void	RemoveSender(ReflectorSender* inStreamElem);
		Bool16	HasSender() { return (this->GetDemuxer()->GetHashTable()->GetNumEntries() > 0); }
		Bool16  ProcessPacket(const SInt64& inMilliseconds,ReflectorPacket* thePacket,UInt32 theRemoteAddr,UInt16 theRemotePort);
		ReflectorPacket* 	GetPacket();
		virtual SInt64		Run();
		void 	SetSSRCFilter(Bool16 state, UInt32 timeoutSecs) { fFilterSSRCs = state; fTimeoutSecs = timeoutSecs;}
	private:
		
		//virtual SInt64		Run();
		void 	GetIncomingData(const SInt64& inMilliseconds);
		void	FilterInvalidSSRCs(ReflectorPacket* thePacket,Bool16 isRTCP);

		//Number of packets to allocate when the socket is first created
		enum
		{
			kNumPreallocatedPackets = 20,	//UInt32
			kRefreshBroadcastSessionIntervalMilliSecs = 10000,
			kSSRCTimeOut = 30000 // milliseconds before clearing the SSRC if no new ssrcs have come in
		};
		QTSS_ClientSessionObject	fBroadcasterClientSession;
		SInt64 						fLastBroadcasterTimeOutRefresh;	
		// Queue of available ReflectorPackets
		OSQueue	fFreeQueue;
		// Queue of senders
		OSQueue fSenderQueue;
		SInt64	fSleepTime;
		
		UInt32	fValidSSRC;
		SInt64	fLastValidSSRCTime;
		Bool16	fFilterSSRCs;
		UInt32  fTimeoutSecs;
};


class ReflectorSocketPool : public UDPSocketPool
{
	public:
	
		ReflectorSocketPool() {}
		virtual ~ReflectorSocketPool() {}
		
		virtual UDPSocketPair*	ConstructUDPSocketPair();
		virtual void			DestructUDPSocketPair(UDPSocketPair *inPair);
};

class ReflectorSender : public UDPDemuxerTask
{
	ReflectorSender(ReflectorStream* inStream, UInt32 inWriteFlag);
	virtual ~ReflectorSender();
	
	//Used for adjusting sequence numbers in light of thinning
	UInt16		GetPacketSeqNumber(const StrPtrLen& inPacket);
	void		SetPacketSeqNumber(const StrPtrLen& inPacket, UInt16 inSeqNumber);
	Bool16 		PacketShouldBeThinned(QTSS_RTPStreamObject inStream, const StrPtrLen& inPacket);

	//We want to make sure that ReflectPackets only gets invoked when there
	//is actually work to do, because it is an expensive function
	Bool16		ShouldReflectNow(const SInt64& inCurrentTime, SInt64* ioWakeupTime);
	
	//This function gets data from the multicast source and reflects.
	//Returns the time at which it next needs to be invoked
	void		ReflectPackets(SInt64* ioWakeupTime, OSQueue* inFreeQueue);

	ReflectorStream* 	fStream;
	UInt32				fWriteFlag;
	
	OSQueue			fPacketQueue;
	OSQueueElem*	fFirstNewPacketInQueue;

	//these serve as an optimization, keeping track of when this
	//sender needs to run so it doesn't run unnecessarily
	Bool16		fHasNewPackets;
	SInt64		fNextTimeToRun;
			
	//how often to send RRs to the source
	enum
	{
		kRRInterval = 5000		//SInt64 (every 5 seconds)
	};

	SInt64		fLastRRTime;
	OSQueueElem	fSocketQueueElem;
	
	friend class ReflectorSocket;
	friend class ReflectorStream;
};

class ReflectorStream
{
	public:
	
		enum
		{
			// A ReflectorStream is uniquely identified by the
			// destination IP address & destination port of the broadcast.
			// This ID simply contains that information.
			//
			// A unicast broadcast can also be identified by source IP address. If
			// you are attempting to demux by source IP, this ID will not guarentee
			// uniqueness and special care should be used.
			kStreamIDSize = sizeof(UInt32) + sizeof(UInt16)
		};
		
		// Uses a StreamInfo to generate a unique ID
		static void	GenerateSourceID(SourceInfo::StreamInfo* inInfo, char* ioBuffer);
	
		ReflectorStream(SourceInfo::StreamInfo* inInfo);
		~ReflectorStream();
		
		//
		// SETUP
		//
		// Call Register from the Register role, as this object has some QTSS API
		// attributes to setup
		static void	Register();
		static void Initialize(QTSS_ModulePrefsObject inPrefs);
		
		//
		// MODIFIERS
		
		// Call this to initialize the reflector sockets. Uses the QTSS_RTSPRequestObject
		// if provided to report any errors that occur 
		// Passes the QTSS_ClientSessionObject to the socket so the socket can update the session if needed.
		QTSS_Error BindSockets(QTSS_StandardRTSP_Params* inParams, UInt32 inReflectorSessionFlags, Bool16 filterState, UInt32 timeout);
		
		// This stream reflects packets from the broadcast to specific ReflectorOutputs.
		// You attach outputs to ReflectorStreams this way. You can force the ReflectorStream
		// to put this output into a certain bucket by passing in a certain bucket index.
		// Pass in -1 if you don't care. AddOutput returns the bucket index this output was
		// placed into, or -1 on an error.
		
		SInt32	AddOutput(ReflectorOutput* inOutput, SInt32 putInThisBucket);
		
		// Removes the specified output from this ReflectorStream.
		void 	RemoveOutput(ReflectorOutput* inOutput); // Removes this output from all tracks
		
		void  TearDownAllOutputs(); // causes a tear down and then a remove

		// If the incoming data is RTSP interleaved, packets for this stream are identified
		// by channel numbers
		void					SetRTPChannelNum(SInt16 inChannel) { fRTPChannel = inChannel; }
		void					SetRTCPChannelNum(SInt16 inChannel) { fRTCPChannel = inChannel; }
		void 					PushPacket(char *packet, UInt32 packetLen, Bool16 isRTCP);
		
		//
		// ACCESSORS
		
		OSRef*					GetRef()			{ return &fRef; }
		UInt32					GetBitRate()		{ return fCurrentBitRate; }
		SourceInfo::StreamInfo*	GetStreamInfo()		{ return &fStreamInfo; }
		OSMutex*				GetMutex()			{ return &fBucketMutex; }
		void*					GetStreamCookie()	{ return this; }
		SInt16					GetRTPChannel()		{ return fRTPChannel; }
		SInt16					GetRTCPChannel()	{ return fRTCPChannel; }
		UDPSocketPair*			GetSocketPair()		{ return fSockets;}

	private:
	
		//Sends an RTCP receiver report to the broadcast source
		void	SendReceiverReport();
		void 	AllocateBucketArray(UInt32 inNumBuckets);
		SInt32 	FindBucket();
		
		// Unique ID & OSRef. ReflectorStreams can be mapped & shared
		OSRef				fRef;
		char				fSourceIDBuf[kStreamIDSize];
		
		// Reflector sockets, retrieved from the socket pool
		UDPSocketPair*		fSockets;

		ReflectorSender		fRTPSender;
		ReflectorSender		fRTCPSender;
		SequenceNumberMap	fSequenceNumberMap; //for removing duplicate packets
		
		// All the necessary info about this stream
		SourceInfo::StreamInfo	fStreamInfo;
		
		enum
		{
			kReceiverReportSize = 16,				//UInt32
			kAppSize = 36,							//UInt32
			kMinNumBuckets = 16,					//UInt32
			kBitRateAvgIntervalInMilSecs = 30000 // time between bitrate averages
		};
	
		// BUCKET ARRAY
		
		//ReflectorOutputs are kept in a 2-dimensional array, "Buckets"
		typedef ReflectorOutput** Bucket;
		Bucket*		fOutputArray;

		UInt32		fNumBuckets;		//Number of buckets currently
		UInt32		fNumElements;		//Number of reflector outputs in the array
		
		//Bucket array can't be modified while we are sending packets.
		OSMutex		fBucketMutex;
		
		// RTCP RR information
		
		char		fReceiverReportBuffer[kReceiverReportSize + kAppSize +
										RTCPSRPacket::kMaxCNameLen];
		UInt32*		fEyeLocation;//place in the buffer to write the eye information
		UInt32		fReceiverReportSize;
		
		// This is the destination address & port for RTCP
		// receiver reports.
		UInt32		fDestRTCPAddr;
		UInt16		fDestRTCPPort;
		
	
		// Used for calculating average bit rate
		UInt32				fCurrentBitRate;
		SInt64				fLastBitRateSample;
		unsigned int		fBytesSentInThisInterval;// unsigned long because we need to atomic_add it

		// If incoming data is RTSP interleaved
		SInt16				fRTPChannel; //These will be -1 if not set to anything
		SInt16				fRTCPChannel;
		
		static UInt32 		sDelayInMsec;
		static UInt32 		sBucketSize;

		friend class ReflectorSocket;
		friend class ReflectorSender;
};



#endif //_REFLECTOR_SESSION_H_

