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

	Contains:	API interface for objects to use to get access to attributes,
				data items, whatever, specific to RTP sessions (see RTPSession.h
				for more details on what that object is). This object
				implements the RTP Session Dictionary.

	

*/


#ifndef _RTPSESSIONINTERFACE_H_
#define _RTPSESSIONINTERFACE_H_

#include "QTSSDictionary.h"

#include "RTCPSRPacket.h"
#include "RTSPSessionInterface.h"
#include "TimeoutTask.h"
#include "Task.h"
#include "RTPBandwidthTracker.h"

#include "OSMutex.h"
#include "atomic.h"

class RTPSessionInterface : public QTSSDictionary, public Task
{
	public:
	
		// Initializes dictionary resources
		static void	Initialize();
		
		//
		// CONSTRUCTOR / DESTRUCTOR
		
		RTPSessionInterface();
		virtual ~RTPSessionInterface()
			{ if (fRTSPSession != NULL) fRTSPSession->DecrementObjectHolderCount(); }

		//Timeouts. This allows clients to refresh the timeout on this session
		void	RefreshTimeout()  		{ fTimeoutTask.RefreshTimeout(); }
		
		//
		// ACCESSORS
		
		Bool16	IsFirstPlay()			{ return fIsFirstPlay; }
		SInt64	GetFirstPlayTime()		{ return fFirstPlayTime; }
		//Time (msec) most recent play was issued
		SInt64	GetPlayTime()			{ return fPlayTime; }
		SInt64	GetNTPPlayTime()		{ return fNTPPlayTime; }
		SInt64	GetSessionCreateTime()	{ return fSessionCreateTime; }
		//Time (msec) most recent play, adjusted for start time of the movie
		//ex: PlayTime() == 20,000. Client said start 10 sec into the movie,
		//so AdjustedPlayTime() == 10,000
		SInt64	GetAdjustedPlayTime()	{ return fAdjustedPlayTime; }
		QTSS_PlayFlags GetPlayFlags()	{ return fPlayFlags; }
		RTCPSRPacket*	GetSRPacket()		{ return &fRTCPSRPacket; }
		OSMutex*		GetSessionMutex()	{ return &fSessionMutex; }
		UInt32			GetPacketsSent()	{ return fPacketsSent; }
		UInt32			GetBytesSent()	{ return fBytesSent; }
		StrPtrLen*	GetSessionID()		{ return &fRTSPSessionID; }
		OSRef*		GetRef()			{ return &fRTPMapElem; }
		RTSPSessionInterface* GetRTSPSession() { return fRTSPSession; }
		UInt32		GetMovieAvgBitrate(){ return fMovieAverageBitRate; }
		QTSS_CliSesTeardownReason GetTeardownReason() { return fTeardownReason; }
		QTSS_RTPSessionState	GetSessionState() { return fState; }
		void	SetUniqueID(UInt32 theID)	{fUniqueID = theID;}
		RTPBandwidthTracker* GetBandwidthTracker() { return &fTracker; }
		
		//
		// STATISTICS UPDATING
		
		//The session tracks the total number of bytes sent during the session.
		//Streams can update that value by calling this function
		void			UpdateBytesSent(UInt32 bytesSent)
										{ (void)atomic_add(&fBytesSent, bytesSent); }
						
		//The session tracks the total number of packets sent during the session.
		//Streams can update that value by calling this function				
		void  			UpdatePacketsSent(UInt32 packetsSent)  
										{ (void)atomic_add(&fPacketsSent, packetsSent); }
		
		//
		// RTSP RESPONSES

		// This function appends a session header to the SETUP response, and
		// checks to see if it is a 304 Not Modified. If it is, it sends the entire
		// response and returns an error
		QTSS_Error DoSessionSetupResponse(RTSPRequestInterface* inRequest);
		
		//
		// RTSP SESSION
								
		// This object has a current RTSP session. This may change over the
		// life of the RTSPSession, so update it. It keeps an RTSP session in
		// case interleaved data or commands need to be sent back to the client. 
		void			UpdateRTSPSession(RTSPSessionInterface* inNewRTSPSession);
				
		// let's RTSP Session pass along it's query string
		void			SetQueryString( StrPtrLen* queryString );
		
	protected:
	
		// These variables are setup by the derived RTPSession object when
		// Play and Pause get called

		//Some stream related information that is shared amongst all streams
		Bool16		fIsFirstPlay;
		SInt64		fFirstPlayTime;//in milliseconds
		SInt64		fPlayTime;
		SInt64		fAdjustedPlayTime;
		SInt64		fNTPPlayTime;
		SInt64		fNextSendPacketsTime;
		
		//keeps track of whether we are playing or not
		QTSS_RTPSessionState fState;
		
		// If we are playing, this are the play flags that were set on play
		QTSS_PlayFlags	fPlayFlags;
		
		//
		// Play speed of this session
		Float32		fSpeed;
		
		//
		//Start and stop times for this play spurt
		Float64		fStartTime;
		Float64		fStopTime;
		
		//Session mutex. This mutex should be grabbed before invoking the module
		//responsible for managing this session. This allows the module to be
		//non-preemptive-safe with respect to a session
		OSMutex		fSessionMutex;

		//Stores the session ID
		OSRef			 	fRTPMapElem;
		StrPtrLen			fRTSPSessionID;
		char				fRTSPSessionIDBuf[QTSS_MAX_SESSION_ID_LENGTH + 4];
		
		// In order to facilitate sending data over the RTSP channel from
		// an RTP session, we store a pointer to the RTSP session used in
		// the last RTSP request.
		RTSPSessionInterface* fRTSPSession;

	private:
	
		static Bool16 PacketLossPercent(QTSS_FunctionParams* funcParamsPtr);
		static Bool16 TimeConnected(QTSS_FunctionParams* funcParamsPtr);
		static Bool16 BitRate(QTSS_FunctionParams* funcParamsPtr);

		// One of the RTP session attributes is an iterated list of all streams.
		// As an optimization, we preallocate a "large" buffer of stream pointers,
		// even though we don't know how many streams we need at first.
		enum
		{
			kStreamBufSize 				= 4,
			kUserAgentBufSize 			= 256,
			kPresentationURLSize		= 256,
			kFullRequestURLBufferSize	= 256,
			kRequestHostNameBufferSize	= 80,
			
			kIPAddrStrBufSize 			= 20,
			kLocalDNSBufSize 			= 80,
			
		};
		
		void* 		fStreamBuffer[kStreamBufSize];

		
		// theses are dictionary items picked up by the RTSPSession
		// but we need to store copies of them for logging purposes.
		char		fUserAgentBuffer[kUserAgentBufSize];
		char		fPresentationURL[kPresentationURLSize];			// eg /foo/bar.mov
		char		fFullRequestURL[kFullRequestURLBufferSize];		// eg rtsp://yourdomain.com/foo/bar.mov
		char		fRequestHostName[kRequestHostNameBufferSize];	// eg yourdomain.com
	
		char		fRTSPSessRemoteAddrStr[kIPAddrStrBufSize];
		char		fRTSPSessLocalDNS[kLocalDNSBufSize];
		char		fRTSPSessLocalAddrStr[kIPAddrStrBufSize];
		
		char		fUserNameBuf[RTSPSessionInterface::kMaxUserNameLen];
		char		fUserPasswordBuf[RTSPSessionInterface::kMaxUserPasswordLen];
		char		fUserRealmBuf[RTSPSessionInterface::kMaxUserRealmLen];
		UInt32		fLastRTSPReqRealStatusCode;

		//for timing out this session
		TimeoutTask	fTimeoutTask;
		UInt32		fTimeout;
		
		// Time when this session got created
		SInt64		fSessionCreateTime;

		//Packet priority levels. Each stream has a current level, and
		//the module that owns this session sets what the number of levels is.
		UInt32		fNumQualityLevels;
		
		//Statistics
		unsigned int fBytesSent;
		unsigned int fPacketsSent;	
		Float32 fPacketLossPercent;
		SInt64 fTimeConnected;
		SInt64 fLastBitRateTime;
		UInt32 fLastBitsSent;
		// Movie size & movie duration. It may not be so good to associate these
		// statistics with the movie, for a session MAY have multiple movies...
		// however, the 1 movie assumption is in too many subsystems at this point
		Float64		fMovieDuration;
		UInt64		fMovieSizeInBytes;
		UInt32		fMovieAverageBitRate;
		UInt32		fMovieCurrentBitRate;
		QTSS_CliSesTeardownReason fTeardownReason;
		// So the streams can send sender reports
		RTCPSRPacket		fRTCPSRPacket;
		UInt32		fUniqueID;
		
		RTPBandwidthTracker	fTracker;
		
		// Built in dictionary attributes
		static QTSSAttrInfoDict::AttrInfo	sAttributes[];
		static unsigned int	sRTPSessionIDCounter;
};

#endif //_RTPSESSIONINTERFACE_H_
