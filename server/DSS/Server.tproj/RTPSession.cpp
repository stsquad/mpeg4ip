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
	File:		RTPSession.cpp

	Contains:	Implementation of RTPSession class. 
					
	Change History (most recent first):

	
	
*/

#include "RTPSession.h"

#include "RTSPProtocol.h" 
#include "QTSServerInterface.h"
#include "QTSS.h"

#include "OS.h"
#include "OSMemory.h"

#include <errno.h>

#define RTPSESSION_DEBUGGING 0

RTPSession::RTPSession() :
 	fModule(NULL),
	fCurrentModuleIndex(0),
	fCurrentState(kStart),
	fClosingReason(qtssCliSesCloseClientTeardown),
	fCurrentModule(0),
	fModuleDoingAsyncStuff(false),
	fHasAnRTPStream(false)

{
#if DEBUG
	fActivateCalled = false;
#endif


	fModuleState.curModule = NULL;
	fModuleState.curTask = this;
	fModuleState.curRole = 0;
}

RTPSession::~RTPSession()
{
	// Delete all the streams
	RTPStream** theStream = NULL;
	UInt32 theLen = 0;
	
	for (int x = 0; this->GetValuePtr(qtssCliSesStreamObjects, x, (void**)&theStream, &theLen) == QTSS_NoErr; x++)
	{
		Assert(theStream != NULL);
		Assert(theLen == sizeof(RTPStream*));
		
		delete *theStream;
	}

	QTSServerInterface::GetServer()->AlterCurrentRTPSessionCount(-1);
	//we better not be in the RTPSessionMap anymore!
#if DEBUG
	Assert(!fRTPMapElem.IsInTable());
	OSRef* theRef = QTSServerInterface::GetServer()->GetRTPSessionMap()->Resolve(&fRTSPSessionID);
	Assert(theRef == NULL);
#endif
}

QTSS_Error	RTPSession::Activate(const StrPtrLen& inSessionID)
{
	//Set the session ID for this session
	Assert(inSessionID.Len <= QTSS_MAX_SESSION_ID_LENGTH);
	::memcpy(fRTSPSessionIDBuf, inSessionID.Ptr, inSessionID.Len);
	fRTSPSessionIDBuf[inSessionID.Len] = '\0';
	fRTSPSessionID.Len = inSessionID.Len;
	
	fRTPMapElem.Set(fRTSPSessionID, this);
	
	//Activate puts the session into the RTPSession Map
	OSRefTable* sessionTable = QTSServerInterface::GetServer()->GetRTPSessionMap();
	Assert(sessionTable != NULL);
	QTSS_Error err = sessionTable->Register(&fRTPMapElem);
	if (err == EPERM)
		return err;
	
#if DEBUG
	fActivateCalled = true;
#endif
	QTSServerInterface::GetServer()->IncrementTotalRTPSessions();
	Assert(err == QTSS_NoErr);
	return QTSS_NoErr;
}

QTSS_Error RTPSession::AddStream(RTSPRequestInterface* request, RTPStream** outStream,
										QTSS_AddStreamFlags inFlags)
{
	Assert(outStream != NULL);

	// Create a new SSRC for this stream. This should just be a random number unique
	// to all the streams in the session
	UInt32 theSSRC = 0;
	while (theSSRC == 0)
	{
		theSSRC = (SInt32)::rand();

		RTPStream** theStream = NULL;
		UInt32 theLen = 0;
	
		for (int x = 0; this->GetValuePtr(qtssCliSesStreamObjects, x, (void**)&theStream, &theLen) == QTSS_NoErr; x++)
		{
			Assert(theStream != NULL);
			Assert(theLen == sizeof(RTPStream*));
			
			if ((*theStream)->GetSSRC() == theSSRC)
				theSSRC = 0;
		}
	}

	*outStream = NEW RTPStream(theSSRC, this);
	
	QTSS_Error theErr = (*outStream)->Setup(request, inFlags);
	if (theErr != QTSS_NoErr)
		// If we couldn't setup the stream, make sure not to leak memory!
		delete *outStream;
	else
	{
		// If the stream init succeeded, then put it into the array of setup streams
		theErr = this->SetValue(qtssCliSesStreamObjects, this->GetNumValues(qtssCliSesStreamObjects),
													outStream, sizeof(RTPStream*), QTSSDictionary::kDontObeyReadOnly);
		Assert(theErr == QTSS_NoErr);
		fHasAnRTPStream = true;
	}
	return theErr;
}

QTSS_Error	RTPSession::Play(RTSPRequestInterface* request, QTSS_PlayFlags inFlags)
{
	//first setup the play associated session interface variables
	Assert(request != NULL);
	
	if (fModule == NULL)
		return QTSS_RequestFailed;//Can't play if there are no associated streams
	
	// Always reset the speed to 1. It is up to a module to override this behavior
	fSpeed = 1;

	fStartTime = request->GetStartTime();
	fStopTime = request->GetStopTime();
	
	//the client doesn't necessarily specify this information in a play,
	//if it doesn't, fall back on some defaults.
	if (fStartTime == -1)
		fStartTime = 0;
		
	//what time is this play being issued at?
	fNextSendPacketsTime = fPlayTime = OS::Milliseconds() + kPlayDelayTimeInMilSecs; //don't play until we've sent the play response
	if (fIsFirstPlay)
		fFirstPlayTime = fPlayTime - kPlayDelayTimeInMilSecs;
	fAdjustedPlayTime = fPlayTime - ((SInt64)(fStartTime * 1000));

	//for RTCP SRs, we also need to store the play time in NTP
	fNTPPlayTime = OS::TimeMilli_To_1900Fixed64Secs(fPlayTime);
	
	//we are definitely playing now, so schedule the object!
	fState = qtssPlayingState;
	fIsFirstPlay = false;
	fPlayFlags = inFlags;
	
	//
	// Go through all the streams, setting their thinning params
	RTPStream** theStream = NULL;
	UInt32 theLen = 0;
	
	for (int x = 0; this->GetValuePtr(qtssCliSesStreamObjects, x, (void**)&theStream, &theLen) == QTSS_NoErr; x++)
	{
		Assert(theStream != NULL);
		Assert(theLen == sizeof(RTPStream*));
		(*theStream)->SetThinningParams();
	}
	
	// Set the size of the RTSPSession's send buffer to an appropriate max size
	// based on the bitrate of the movie. This has 2 benefits:
	// 1) Each socket normally defaults to 32 K. A smaller buffer prevents the
	// system from getting buffer starved if lots of clients get flow-controlled
	//
	// 2) We may need to scale up buffer sizes for high-bandwidth movies in order
	// to maximize thruput, and we may need to scale down buffer sizes for low-bandwidth
	// movies to prevent us from buffering lots of data that the client can't use
	
	// If we don't know any better, assume maximum buffer size.
	QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();
	UInt32 theBufferSize = thePrefs->GetMaxTCPBufferSizeInBytes();
	
#if RTPSESSION_DEBUGGING
	printf("RTPSession GetMovieAvgBitrate %li\n",(SInt32)this->GetMovieAvgBitrate() );
#endif

	if (this->GetMovieAvgBitrate() > 0)
	{
		// We have a bit rate... use it.
		Float32 realBufferSize = (Float32)this->GetMovieAvgBitrate() * thePrefs->GetTCPSecondsToBuffer();
		theBufferSize = (UInt32)realBufferSize;
		theBufferSize >>= 3; // Divide by 8 to convert from bits to bytes
		
		// Round down to the next lowest power of 2.
		theBufferSize = this->PowerOf2Floor(theBufferSize);
		
		// This is how much data we should buffer based on the scaling factor... if it is
		// lower than the min, raise to min
		if (theBufferSize < thePrefs->GetMinTCPBufferSizeInBytes())
			theBufferSize = thePrefs->GetMinTCPBufferSizeInBytes();
			
		// Same deal for max buffer size
		if (theBufferSize > thePrefs->GetMaxTCPBufferSizeInBytes())
			theBufferSize = thePrefs->GetMaxTCPBufferSizeInBytes();
	}
	
	Assert(fRTSPSession != NULL); // can this ever happen?
	if (fRTSPSession != NULL)
		fRTSPSession->GetSocket()->SetSocketBufSize(theBufferSize);
	
#if RTPSESSION_DEBUGGING
	printf("RTPSession %ld: In Play, about to call Signal\n",(SInt32)this);
#endif
	this->Signal(Task::kStartEvent);
	
	return QTSS_NoErr;
}

UInt32 RTPSession::PowerOf2Floor(UInt32 inNumToFloor)
{
	UInt32 retVal = 0x10000000;
	while (retVal > 0)
	{
		if (retVal & inNumToFloor)
			return retVal;
		else
			retVal >>= 1;
	}
	return retVal;
}

void RTPSession::Teardown()
{
	// To proffer a quick death of the RTSP session, let's disassociate
	// ourselves with it right now.
	
	// Note that this function relies on the session mutex being grabbed, because
	// this fRTSPSession pointer could otherwise be being used simultaneously by
	// an RTP stream.
	if (fRTSPSession != NULL)
		fRTSPSession->DecrementObjectHolderCount();
	fRTSPSession = NULL;
	fState = qtssPausedState;
	this->Signal(Task::kKillEvent);
}

void RTPSession::SendPlayResponse(RTSPRequestInterface* request, UInt32 inFlags)
{
	QTSS_RTSPHeader theHeader = qtssRTPInfoHeader;
	
	//
	// If there was a Speed header in the request, add one to the response
	if (request->GetHeaderDictionary()->GetValue(qtssSpeedHeader)->Len > 0)
	{
		char speedBuf[32];
		::sprintf(speedBuf, "%10.5f", fSpeed);
		StrPtrLen speedBufPtr(speedBuf);
		request->AppendHeader(qtssSpeedHeader, &speedBufPtr);
	}
	
	RTPStream** theStream = NULL;
	UInt32 theLen = 0;
	
	for (int x = 0; this->GetValuePtr(qtssCliSesStreamObjects, x, (void**)&theStream, &theLen) == QTSS_NoErr; x++)
	{
		Assert(theStream != NULL);
		Assert(theLen == sizeof(RTPStream*));
		(*theStream)->AppendRTPInfo(theHeader, request, inFlags);
		theHeader = qtssSameAsLastHeader;
	}
	request->SendHeader();
}

void	RTPSession::SendDescribeResponse(RTSPRequestInterface* inRequest)
{
	if (inRequest->GetStatus() == qtssRedirectNotModified)
	{
		(void)inRequest->SendHeader();
		return;
	}
	
	// write date and expires
	inRequest->AppendDateAndExpires();
	
	//write content type header
	static StrPtrLen sContentType("application/sdp");
	inRequest->AppendHeader(qtssContentTypeHeader, &sContentType);
	
	// write x-Accept-Retransmit header
	static StrPtrLen sRetransmitProtocolName("our-retransmit");
	inRequest->AppendHeader(qtssXAcceptRetransmitHeader, &sRetransmitProtocolName);
	
	//write content base header
	
	inRequest->AppendContentBaseHeader(inRequest->GetValue(qtssRTSPReqAbsoluteURL));
	
	//I believe the only error that can happen is if the client has disconnected.
	//if that's the case, just ignore it, hopefully the calling module will detect
	//this and return control back to the server ASAP 
	(void)inRequest->SendHeader();
}

void	RTPSession::SendAnnounceResponse(RTSPRequestInterface* inRequest)
{
	//
	// Currently, no need to do anything special for an announce response
	(void)inRequest->SendHeader();
}


SInt64 RTPSession::Run()
{
#if DEBUG
	Assert(fActivateCalled);
#endif
	EventFlags events = this->GetEvents();
	QTSS_RoleParams theParams;
	theParams.clientSessionClosingParams.inClientSession = this;	//every single role being invoked now has this
													//as the first parameter
	
#if RTPSESSION_DEBUGGING
	printf("RTPSession %ld: In Run. Events %ld\n",(SInt32)this, (SInt32)events);
#endif
	// Some callbacks look for this struct in the thread object
	OSThread::GetCurrent()->SetThreadData(&fModuleState);

	//if we have been instructed to go away, then let's delete ourselves
	if ((events & Task::kKillEvent) || (events & Task::kTimeoutEvent) || (fModuleDoingAsyncStuff))
	{
		if (!fModuleDoingAsyncStuff)
	{
		if (events & Task::kTimeoutEvent)
			fClosingReason = qtssCliSesCloseTimeout;
			
		//deletion is a bit complicated. For one thing, it must happen from within
		//the Run function to ensure that we aren't getting events when we are deleting
		//ourselves. We also need to make sure that we aren't getting RTSP requests
		//(or, more accurately, that the stream object isn't being used by any other
		//threads). We do this by first removing the session from the session map.
		
#if RTPSESSION_DEBUGGING
		printf("RTPSession %ld: about to be killed. Eventmask = %ld\n",(SInt32)this, (SInt32)events);
#endif
		// We cannot block waiting to UnRegister, because we have to
		// give the RTSPSessionTask a chance to release the RTPSession.
		OSRefTable* sessionTable = QTSServerInterface::GetServer()->GetRTPSessionMap();
		Assert(sessionTable != NULL);
		if (!sessionTable->TryUnRegister(&fRTPMapElem))
		{
			this->Signal(Task::kKillEvent);// So that we get back to this place in the code
			return kCantGetMutexIdleTime;
		}
		
			// The ClientSessionClosing role is allowed to do async stuff
			fModuleState.curTask = this;
			fModuleDoingAsyncStuff = true; 	// So that we know to jump back to the
			fCurrentModule = 0;				// right place in the code
		
			// Set the reason parameter 
			theParams.clientSessionClosingParams.inReason = fClosingReason;
			
			// If RTCP packets are being generated internally for this stream, 
			// Send a BYE now.
			RTPStream** theStream = NULL;
			UInt32 theLen = 0;
			
			if (this->GetPlayFlags() & qtssPlayFlagsSendRTCP)
			{
				SInt64 byePacketTime = OS::Milliseconds();
				for (int x = 0; this->GetValuePtr(qtssCliSesStreamObjects, x, (void**)&theStream, &theLen) == QTSS_NoErr; x++)
					(*theStream)->SendRTCPSR(byePacketTime, true);
			}
		}
		
		//at this point, we know no one is using this session, so invoke the
		//session cleanup role. We don't need to grab the session mutex before
		//invoking modules here, because the session is unregistered and
		//therefore there's no way another thread could get involved anyway

		UInt32 numModules = QTSServerInterface::GetNumModulesInRole(QTSSModule::kClientSessionClosingRole);
		{
			for (; fCurrentModule < numModules; fCurrentModule++)
			{  
				fModuleState.eventRequested = false;
				fModuleState.idleTime = 0;
				QTSSModule* theModule = QTSServerInterface::GetModule(QTSSModule::kClientSessionClosingRole, fCurrentModule);
				(void)theModule->CallDispatch(QTSS_ClientSessionClosing_Role, &theParams);

				// If this module has requested an event, return and wait for the event to transpire
				if (fModuleState.eventRequested == true)
					return fModuleState.idleTime; // If the module has requested idle time...
			}
		}
		
		return -1;//doing this will cause the destructor to get called.
	}
	
	//if the stream is currently paused, just return without doing anything.
	//We'll get woken up again when a play is issued
	if ((fState == qtssPausedState) || (fModule == NULL))
		return 0;
		
	//Make sure to grab the session mutex here, to protect the module against
	//RTSP requests coming in while it's sending packets
	{
		OSMutexLocker locker(&fSessionMutex);

		//just make sure we haven't been scheduled before our scheduled play
		//time. If so, reschedule ourselves for the proper time. (if client
		//sends a play while we are already playing, this may occur)
		theParams.rtpSendPacketsParams.inCurrentTime = OS::Milliseconds();
		if (fNextSendPacketsTime > theParams.rtpSendPacketsParams.inCurrentTime)
		{
			RTPStream** retransStream = NULL;
			UInt32 retransStreamLen = 0;

			//
			// Send retransmits if we need to
			for (int streamIter = 0; this->GetValuePtr(qtssCliSesStreamObjects, streamIter, (void**)&retransStream, &retransStreamLen) == QTSS_NoErr; streamIter++)
			{
				//printf("RTPSession: Calling ResendDueEntries\n");
				(*retransStream)->SendRetransmits();
			}
			
			theParams.rtpSendPacketsParams.outNextPacketTime = fNextSendPacketsTime - theParams.rtpSendPacketsParams.inCurrentTime;
		}
		else
		{			
	#if RTPSESSION_DEBUGGING
			printf("RTPSession %ld: about to call SendPackets\n",(SInt32)this);
	#endif
			theParams.rtpSendPacketsParams.outNextPacketTime = 0;
			// Async event registration is definitely allowed from this role.
			fModuleState.eventRequested = false;
			Assert(fModule != NULL);
			(void)fModule->CallDispatch(QTSS_RTPSendPackets_Role, &theParams);
	#if RTPSESSION_DEBUGGING
			printf("RTPSession %ld: back from sendPackets, nextPacketTime = %"_64BITARG_"d\n",(SInt32)this, theParams.rtpSendPacketsParams.outNextPacketTime);
	#endif
			//make sure not to get deleted accidently!
			if (theParams.rtpSendPacketsParams.outNextPacketTime < 0)
				theParams.rtpSendPacketsParams.outNextPacketTime = 0;
			fNextSendPacketsTime = theParams.rtpSendPacketsParams.inCurrentTime + theParams.rtpSendPacketsParams.outNextPacketTime;
		}
	}
	
	//
	// Make sure the duration between calls to Run() isn't greater than the
	// max retransmit delay interval.
	UInt32 theRetransDelayInMsec = QTSServerInterface::GetServer()->GetPrefs()->GetMaxRetransmitDelayInMsec();
	if (theParams.rtpSendPacketsParams.outNextPacketTime > theRetransDelayInMsec)
		theParams.rtpSendPacketsParams.outNextPacketTime = theRetransDelayInMsec;
	
	Assert(theParams.rtpSendPacketsParams.outNextPacketTime >= 0);//we'd better not get deleted accidently!
	return theParams.rtpSendPacketsParams.outNextPacketTime;
}

