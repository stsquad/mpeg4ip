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
	File:		RTSPSessionInterface.h

	Contains:	Presents an API for session-wide resources for modules to use.
				Implements the RTSP Session dictionary for QTSS API.
	
	
*/

#ifndef __RTSPSESSIONINTERFACE_H__
#define __RTSPSESSIONINTERFACE_H__

#include "RTSPRequestStream.h"
#include "RTSPResponseStream.h"
#include "Task.h"
#include "QTSS.h"
#include "QTSSDictionary.h"
#include "atomic.h"

class RTSPSessionInterface : public QTSSDictionary, public Task
{
public:

	//Initialize must be called right off the bat to initialize dictionary resources
	static void		Initialize();
	static void		SetBase64Decoding(Bool16 newVal) { sDoBase64Decoding = newVal; }
	
	RTSPSessionInterface();
	virtual ~RTSPSessionInterface();
	
	//Is this session alive? If this returns false, clean up and begone as
	//fast as possible
	Bool16 IsLiveSession() 		{ return fSocket.IsConnected() && fLiveSession; }
	
	// Allows clients to refresh the timeout
	void RefreshTimeout()		{ fTimeoutTask.RefreshTimeout(); }
	
	// In order to facilitate sending out of band data on the RTSP connection,
	// other objects need to have direct pointer access to this object. But,
	// because this object is a task object it can go away at any time. If # of
	// object holders is > 0, the RTSPSession will NEVER go away. However,
	// the object managing the session should be aware that if IsLiveSession returns
	// false it may be wise to relinquish control of the session
	void IncrementObjectHolderCount() { (void)atomic_add(&fObjectHolders, 1); }
	void DecrementObjectHolderCount();
	
	// If RTP data is interleaved into the RTSP connection, we need to associate
	// a unique channel number with each rtp stream. This function allocates
	// a new channel number, unique wrt this RTSP session.
	unsigned char		GetChannelNumber() 	{ return fCurChannelNum++; }
	
	//Two main things are persistent through the course of a session, not
	//associated with any one request. The RequestStream (which can be used for
	//getting data from the client), and the socket. OOps, and the ResponseStream
	RTSPRequestStream*	GetInputStream() 	{ return &fInputStream; }
	RTSPResponseStream* GetOutputStream()	{ return &fOutputStream; }
	TCPSocket*			GetSocket() 		{ return &fSocket; }
	OSMutex*			GetSessionMutex()	{ return &fSessionMutex; }
	
	UInt32				GetSessionID()		{ return fSessionID; }
	
	// Request Body Length
	// This object can enforce a length of the request body to prevent callers
	// of Read() from overrunning the request body and going into the next request.
	// -1 is an unknown request body length. If the body length is unknown,
	// this object will do no length enforcement. 
	void				SetRequestBodyLength(SInt32 inLength) 	{ fRequestBodyLen = inLength; }
	SInt32				GetRemainingReqBodyLen() 				{ return fRequestBodyLen; }
	
	// QTSS STREAM FUNCTIONS
	
	// Allows non-buffered writes to the client. These will flow control.
	
	// THE FIRST ENTRY OF THE IOVEC MUST BE BLANK!!!
	virtual QTSS_Error WriteV(iovec* inVec, UInt32 inNumVectors, UInt32 inTotalLength, UInt32* outLenWritten);
	virtual QTSS_Error Write(void* inBuffer, UInt32 inLength, UInt32* outLenWritten, UInt32 inFlags);
	virtual QTSS_Error Read(void* ioBuffer, UInt32 inLength, UInt32* outLenRead);
	virtual QTSS_Error RequestEvent(QTSS_EventType inEventMask);

	// performs RTP over RTSP
	QTSS_Error 	InterleavedWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, unsigned char channel);
	void		InterleaveSetup();

	enum
	{
		kMaxUserNameLen 		= 32,
		kMaxUserPasswordLen 	= 32,
		kMaxUserRealmLen	 	= 64
	};

protected:
	enum
	{
		kFirstRTSPSessionID 	= 1,	//UInt32
	};

	//Each RTSP session has a unique number that identifies it.

	char				fUserNameBuf[kMaxUserNameLen];
	char				fUserPasswordBuf[kMaxUserPasswordLen];
	char				fUserRealmBuf[kMaxUserRealmLen];

	TimeoutTask			fTimeoutTask;//allows the session to be timed out
	
	RTSPRequestStream 	fInputStream;
	RTSPResponseStream 	fOutputStream;
	
	// Any RTP session sending interleaved data on this RTSP session must
	// be prevented from writing while an RTSP request is in progress
	OSMutex				fSessionMutex;
	
	// for coalescing small interleaved writes into a single TCP frame
	enum
	{
		  kTCPCoalesceBufferSize = 1450	//1450 is the max data space in an TCP segment over ent
		, kTCPCoalesceDirectWriteSize = 0 // if > this # bytes bypass coalescing and make a direct write
		, kInteleaveHeaderSize = 4  // '$ '+ 1 byte ch ID + 2 bytes length
	};
	char*		fTCPCoalesceBuffer;
	SInt32		fNumInCoalesceBuffer;


	//+rt  socket we get from "accept()"
	TCPSocket			fSocket;
	TCPSocket*			fOutputSocketP;
	TCPSocket*			fInputSocketP;	// <-- usually same as fSocketP, unless we're HTTP Proxying
	
	void		SnarfInputSocket( RTSPSessionInterface* fromRTSPSession );
	
	// What session type are we?
	QTSS_RTSPSessionType	fSessionType;
	
	Bool16				fLiveSession;
	unsigned int		fObjectHolders;
	unsigned char		fCurChannelNum;
	
	QTSS_StreamRef		fStreamRef;

	UInt32				fSessionID;
	UInt32				fLocalAddr;
	UInt32				fRemoteAddr;
	SInt32				fRequestBodyLen;
	
	static unsigned int	sSessionIDCounter;
	static Bool16			sDoBase64Decoding;
	
	//Dictionary support
	
	// Param retrieval function
	static Bool16		SetupParams(QTSS_FunctionParams* funcParamsPtr);
	
	static QTSSAttrInfoDict::AttrInfo	sAttributes[];
};
#endif // __RTSPSESSIONINTERFACE_H__

